// ItunesLibraryParser
// Allows JSON parsing of XML Itunes Library file
// Copyright (C) 2021 Guillaume Vara <guillaume.vara@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Any graphical resources available within the source code may
// use a different license and copyright : please refer to their metadata
// for further details. Graphical resources without explicit references to a
// different license and copyright still refer to this GPL.

#pragma once

#include <string.h>

#include <array>
#include <string_view>

#include "SIMD/Search.hpp"
#include "config/TracksScan.hpp"

// raw XML values of the wanted fields of a track; a missing field has a null data()
using TrackFields = std::array<std::string_view, FieldType::Index::Count>;

namespace FieldType {

static constexpr const std::size_t notAField = Index::Count;

// index of the field matching an XML <key> name, or "notAField"
inline std::size_t indexOf(const std::string_view& key) {
    for (std::size_t i = 0; i < Index::Count; ++i) {
        if (fieldNames[i] == key) return i;
    }

    return notAField;
}

}  // namespace FieldType

// Single pass over every '<' of [begin, end), which must start and end between two tracks.
// Plist values never contain '<' (escaped as entities), so each tag and each value ends at the next '<'.
// "onTrack(const TrackFields&, bool complete)" is called for every track, in file order.
template<typename OnTrack>
void scanTracks(const char* begin, const char* end, OnTrack&& onTrack) {
    TagCursor cursor { begin, end };

    // 0 : within "Tracks" dict, 1 : within a track dict, 2+ : nested containers
    std::size_t depth = 0;
    bool inTrack = false;
    TrackFields track;

    // "<dict" or "<array", "tag" pointing to '<' (or to '/' of a closing tag)
    const auto isContainer = [](const char* tag) {
        return (tag[1] == 'd' && tag[2] == 'i') || (tag[1] == 'a' && tag[2] == 'r');
    };

    while (true) {
        const char* tag = cursor.next();
        if (tag == end) return;

        switch (tag[1]) {
            // <key>
            case 'k': {
                const char* keyEnd = cursor.next();
                if (keyEnd == end) return;
                if (depth != 1 || !inTrack) break;

                // value opening tag
                const char* valueTag = cursor.next();
                if (valueTag == end) return;

                const auto valueBegin = reinterpret_cast<const char*>(memchr(valueTag, '>', end - valueTag));
                if (valueBegin == nullptr) return;

                // self-closing, like <true/>
                if (valueBegin[-1] == '/') break;

                // nested <dict> / <array>
                if (isContainer(valueTag)) {
                    depth++;
                    break;
                }

                // value closing tag
                const char* valueEnd = cursor.next();
                if (valueEnd == end) return;

                //
                const auto index = FieldType::indexOf({ tag + 5, static_cast<std::size_t>(keyEnd - tag - 5) });
                if (index == FieldType::notAField) break;

                track[index] = { valueBegin + 1, static_cast<std::size_t>(valueEnd - valueBegin - 1) };
                break;
            }

            // <dict> / <array>, but also <date> / <data> values
            case 'd':
            case 'a': {
                if (!isContainer(tag)) break;

                const auto tagEnd = reinterpret_cast<const char*>(memchr(tag, '>', end - tag));
                if (tagEnd == nullptr) return;
                if (tagEnd[-1] == '/') break;

                // a track begins
                if (depth == 0 && tag[1] == 'd') {
                    track = {};
                    inTrack = true;
                }

                depth++;
                break;
            }

            // </dict> / </array>, but also any closing tag
            case '/': {
                if (!isContainer(tag + 1)) break;

                // a track ends
                if (depth == 1 && inTrack) {
                    inTrack = false;

                    bool complete = true;
                    for (std::size_t i = 0; i < track.size(); ++i) {
                        if (track[i].data() != nullptr) continue;

                        const auto &defaultValue = FieldType::defaultValues[i];
                        if (defaultValue.empty()) {
                            complete = false;
                        } else {
                            track[i] = defaultValue;
                        }
                    }

                    onTrack(track, complete);
                }

                if (depth > 0) depth--;
                break;
            }

            default:
                break;
        }
    }
}
