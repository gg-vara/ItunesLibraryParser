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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <array>
#include <new>
#include <string>
#include <string_view>
#include <utility>

#include "TracksScanner.hpp"

// growable output buffer
struct OneWayBuffer {
 public:
    OneWayBuffer(const std::size_t estimatedSize = 0) :
        _size(estimatedSize ? estimatedSize : 64) {
            _buffer = reinterpret_cast<char*>(malloc(_size));
            if (_buffer == nullptr) throw std::bad_alloc();
        }

    OneWayBuffer(OneWayBuffer&& other) noexcept :
        _size(std::exchange(other._size, 0)),
        _index(std::exchange(other._index, 0)),
        _buffer(std::exchange(other._buffer, nullptr)) {}

    ~OneWayBuffer() {
        free(_buffer);
    }

    OneWayBuffer(const OneWayBuffer&) = delete;
    void operator=(const OneWayBuffer&) = delete;

    OneWayBuffer& operator<<(const char& c) {
        _ensureCapacity(1);
        _buffer[_index++] = c;
        return *this;
    }

    OneWayBuffer& operator<<(const std::string_view& view) {
        if (view.empty()) return *this;
        _ensureCapacity(view.size());
        memcpy(_buffer + _index, view.data(), view.size());
        _index += view.size();
        return *this;
    }

    // appends an XML text node value as JSON string content (no englobing quotes)
    OneWayBuffer& appendJSONEscaped(const std::string_view& xmlValue) {
        // fast path : most values need neither decoding nor escaping
        bool plain = true;
        for (const auto &c : xmlValue) {
            if (c == '&' || c == '"' || c == '\\' || static_cast<unsigned char>(c) < 0x20) {
                plain = false;
                break;
            }
        }

        if (plain) return *this << xmlValue;

        // worst case : "\u00XX" per input byte
        _ensureCapacity(xmlValue.size() * 6);

        const auto size = xmlValue.size();
        for (std::size_t i = 0; i < size; ++i) {
            const char c = xmlValue[i];
            if (c == '&') {
                i += _appendXMLEntity(xmlValue.substr(i));
            } else {
                _appendJSONChar(c);
            }
        }

        return *this;
    }

    const std::string_view view() const {
        return std::string_view { _buffer, _index };
    }

 private:
    std::size_t _size;
    std::size_t _index = 0;
    char* _buffer;

    void _ensureCapacity(const std::size_t toAdd) {
        if (_index + toAdd <= _size) return;

        //
        auto newSize = _size ? _size * 2 : 64;
        while (newSize < _index + toAdd) newSize *= 2;

        auto newBuffer = reinterpret_cast<char*>(realloc(_buffer, newSize));
        if (newBuffer == nullptr) throw std::bad_alloc();

        _buffer = newBuffer;
        _size = newSize;
    }

    // capacity must have been ensured beforehand
    void _appendJSONChar(const char c) {
        static constexpr const char hex[] = "0123456789abcdef";

        switch (c) {
            case '"':  _buffer[_index++] = '\\'; _buffer[_index++] = '"';  return;
            case '\\': _buffer[_index++] = '\\'; _buffer[_index++] = '\\'; return;
            case '\n': _buffer[_index++] = '\\'; _buffer[_index++] = 'n';  return;
            case '\r': _buffer[_index++] = '\\'; _buffer[_index++] = 'r';  return;
            case '\t': _buffer[_index++] = '\\'; _buffer[_index++] = 't';  return;
            default:
                break;
        }

        //
        if (static_cast<unsigned char>(c) < 0x20) {
            _buffer[_index++] = '\\';
            _buffer[_index++] = 'u';
            _buffer[_index++] = '0';
            _buffer[_index++] = '0';
            _buffer[_index++] = hex[(c >> 4) & 0xF];
            _buffer[_index++] = hex[c & 0xF];
            return;
        }

        _buffer[_index++] = c;
    }

    // decodes the XML entity starting at "&", returns the count of extra consumed characters
    std::size_t _appendXMLEntity(const std::string_view& from) {
        const auto end = from.find(';');

        // malformed : keep as-is
        if (end == std::string_view::npos || end < 2 || end > 10) {
            _appendJSONChar('&');
            return 0;
        }

        //
        const auto name = from.substr(1, end - 1);
        uint32_t codepoint = 0;

        if (name[0] == '#') {
            const bool isHex = name.size() > 1 && (name[1] == 'x' || name[1] == 'X');
            const auto digits = name.substr(isHex ? 2 : 1);
            if (digits.empty()) {
                _appendJSONChar('&');
                return 0;
            }

            for (const auto &d : digits) {
                uint32_t v;
                if (d >= '0' && d <= '9') {
                    v = d - '0';
                } else if (isHex && d >= 'a' && d <= 'f') {
                    v = d - 'a' + 10;
                } else if (isHex && d >= 'A' && d <= 'F') {
                    v = d - 'A' + 10;
                } else {
                    _appendJSONChar('&');
                    return 0;
                }
                codepoint = codepoint * (isHex ? 16 : 10) + v;
            }
        } else if (name == "amp") {
            codepoint = '&';
        } else if (name == "lt") {
            codepoint = '<';
        } else if (name == "gt") {
            codepoint = '>';
        } else if (name == "quot") {
            codepoint = '"';
        } else if (name == "apos") {
            codepoint = '\'';
        } else {
            _appendJSONChar('&');
            return 0;
        }

        //
        _appendUTF8(codepoint);
        return end;
    }

    // capacity must have been ensured beforehand (an entity is always longer than its UTF-8 / escaped form)
    void _appendUTF8(const uint32_t cp) {
        if (cp < 0x80) {
            _appendJSONChar(static_cast<char>(cp));
        } else if (cp < 0x800) {
            _buffer[_index++] = static_cast<char>(0xC0 | (cp >> 6));
            _buffer[_index++] = static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            _buffer[_index++] = static_cast<char>(0xE0 | (cp >> 12));
            _buffer[_index++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            _buffer[_index++] = static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x110000) {
            _buffer[_index++] = static_cast<char>(0xF0 | (cp >> 18));
            _buffer[_index++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            _buffer[_index++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            _buffer[_index++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
};

// separators of a JSON tracks array
struct JSONLayout {
    std::string_view head;
    std::string_view tail;
    std::string_view tracksSep;
    std::string_view trackOpen;
    std::string_view trackClose;
    std::string_view fieldIndent;
    std::string_view fieldsSep;
};

// single line
constexpr const JSONLayout compactLayout { "[", "]", ",", "{", "}", "", "," };

// human readable
constexpr const JSONLayout prettyLayout { "[\n", "\n]", ",\n", "\t{\n", "\n\t}", "\t\t", ",\n" };

// writes tracks as JSON objects, separated by "tracksSep"
template<const JSONLayout& Layout>
struct JSONTracksWriter {
 public:
    static void write(OneWayBuffer& output, const TrackFields& track, const bool isFirst) {
        static const auto prefixes = _fieldPrefixes();

        if (!isFirst) output << Layout.tracksSep;

        for (std::size_t y = 0; y < track.size(); ++y) {
            output << prefixes[y];
            output.appendJSONEscaped(track[y]);
        }

        output << '"';
        output << Layout.trackClose;
    }

 private:
    // what precedes each field value, from the track opening (first field) or the previous value
    static std::array<std::string, FieldType::Index::Count> _fieldPrefixes() {
        std::array<std::string, FieldType::Index::Count> result;

        for (std::size_t y = 0; y < result.size(); ++y) {
            auto &prefix = result[y];
            if (y == 0) {
                prefix += Layout.trackOpen;
            } else {
                prefix += '"';
                prefix += Layout.fieldsSep;
            }

            prefix += Layout.fieldIndent;
            prefix += '"';
            prefix += FieldType::fieldNames[y];
            prefix += "\":\"";
        }

        return result;
    }
};
