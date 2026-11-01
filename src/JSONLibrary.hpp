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

#include <algorithm>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "ITunesXMLLibrary.hpp"
#include "JSONWriter.hpp"
#include "TracksScanner.hpp"

// JSON conversion of a library tracks, held in memory until written.
// Tracks are split in chunks, each one being scanned and converted on its own thread.
struct JSONLibrary {
 public:
    JSONLibrary(const ITunesXMLLibrary& library) {
        const auto tracks = library.tracks();
        const auto splits = _split(tracks);

        //
        _chunks.reserve(splits.size() - 1);
        for (std::size_t i = 0; i + 1 < splits.size(); ++i) {
            _chunks.emplace_back(static_cast<std::size_t>(splits[i + 1] - splits[i]));
        }

        //
        std::vector<std::thread> workers;
        for (std::size_t i = 1; i < _chunks.size(); ++i) {
            workers.emplace_back(_convert, splits[i], splits[i + 1], std::ref(_chunks[i]));
        }

        _convert(splits[0], splits[1], _chunks[0]);

        for (auto &worker : workers) {
            worker.join();
        }
    }

    JSONLibrary(const JSONLibrary&) = delete;
    void operator=(const JSONLibrary&) = delete;

    // tracks having all fields
    std::size_t tracksCount() const { return _count(&Chunk::tracks); }

    // tracks missing some fields
    std::size_t warningsCount() const { return _count(&Chunk::warnings); }

    void writeTracks(const char * filePath) const {
        _write<compactLayout>(filePath, &Chunk::tracks);
    }

    void writeWarnings(const char * filePath) const {
        _write<prettyLayout>(filePath, &Chunk::warnings);
    }

 private:
    // tracks JSON objects, joined
    struct Part {
        OneWayBuffer json;
        std::size_t count = 0;
    };

    struct Chunk {
        Chunk(const std::size_t xmlSize) : tracks { OneWayBuffer { xmlSize / _xmlToJSONRatio } } {}

        Part tracks;
        Part warnings;
    };

    // below this size per thread, spawning threads costs more than it saves
    static constexpr const std::size_t _minChunkSize = 1 << 20;

    // approx. size of a track XML compared to its JSON
    static constexpr const std::size_t _xmlToJSONRatio = 6;

    std::vector<Chunk> _chunks;

    // chunks boundaries, right after a "</dict>" closing a track (tracks do not contain nested dictionaries)
    static std::vector<const char*> _split(const std::string_view& tracks) {
        static constexpr const auto trackEnd = std::string_view { "</dict>" };

        const std::size_t hwThreads = std::max(1u, std::thread::hardware_concurrency());
        const std::size_t chunksC = std::clamp<std::size_t>(tracks.size() / _minChunkSize, 1, hwThreads);

        std::vector<const char*> splits { tracks.data() };
        for (std::size_t i = 1; i < chunksC; ++i) {
            const std::size_t approx = tracks.size() * i / chunksC;
            if (tracks.data() + approx <= splits.back()) continue;

            // anchored on 'c', rarer than '<'
            const auto found = simd_find(tracks, trackEnd, 4, approx);
            if (found == std::string_view::npos) break;

            splits.push_back(tracks.data() + found + trackEnd.size());
        }

        splits.push_back(tracks.data() + tracks.size());
        return splits;
    }

    static void _convert(const char* begin, const char* end, Chunk& chunk) {
        scanTracks(begin, end, [&chunk](const TrackFields& track, const bool complete) {
            if (complete) {
                JSONTracksWriter<compactLayout>::write(chunk.tracks.json, track, chunk.tracks.count == 0);
                chunk.tracks.count++;
            } else {
                JSONTracksWriter<prettyLayout>::write(chunk.warnings.json, track, chunk.warnings.count == 0);
                chunk.warnings.count++;
            }
        });
    }

    std::size_t _count(Part Chunk::* part) const {
        std::size_t total = 0;
        for (const auto &chunk : _chunks) total += (chunk.*part).count;
        return total;
    }

    template<const JSONLayout& Layout>
    void _write(const char * filePath, Part Chunk::* part) const {
        std::ofstream file { filePath, std::ios::out | std::ios::trunc | std::ios::binary };
        if (!file) throw std::runtime_error(std::string { "Cannot write JSON file [" } + filePath + "]");

        //
        file << Layout.head;

        bool isFirst = true;
        for (const auto &chunk : _chunks) {
            const auto &chunkPart = chunk.*part;
            if (chunkPart.count == 0) continue;

            if (!isFirst) file << Layout.tracksSep;
            file << chunkPart.json.view();
            isFirst = false;
        }

        file << Layout.tail;
    }
};
