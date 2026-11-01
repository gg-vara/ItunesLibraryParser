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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <cassert>
#include <new>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <string_view>
#include <thread>
#include <vector>

#include "SIMD/Search.hpp"
#include "UTF8Path.hpp"

struct ITunesLibraryFileName : public std::filesystem::path {
 public:
    // UTF-8 encoded path
    ITunesLibraryFileName(const char * filePath) :
        std::filesystem::path(utf8Path(filePath)) {
        //
        if (!std::filesystem::exists(*this) || std::filesystem::is_directory(*this)) {
            std::string m ("Cannot open expected XML Library File" );
            m += " [";
            m += utf8String(std::filesystem::absolute(*this));
            m += "]";
            throw std::logic_error(m);
        }

        //
        fileSize = std::filesystem::file_size(*this);
    }

    ITunesLibraryFileName(const ITunesLibraryFileName&) = delete;
    void operator=(const ITunesLibraryFileName&) = delete;

    uintmax_t fileSize;
};

// ITunes XML Library file stored on memory
struct ITunesXMLLibrary {
 public:
    // SIMD searches may read past the end of the searched range (up to 64 bytes + needle size),
    // so the buffer is followed by zeroed bytes
    static constexpr const std::size_t paddingSize = 256;

    ITunesXMLLibrary(const ITunesLibraryFileName&& filePath) :
        _fileSize(filePath.fileSize) {
        //
        _ptr = reinterpret_cast<char *>(malloc(_fileSize + paddingSize));
        if (_ptr == nullptr) throw std::bad_alloc();
        memset(_ptr + _fileSize, 0, paddingSize);

        // reading from multiple handles at once is faster, even from file cache
        const std::filesystem::path &path = filePath;
        const std::size_t partsC = std::clamp<std::size_t>(_fileSize / _minPartSize, 1, _maxParts);
        std::vector<char> succeeded(partsC, 0);
        std::vector<std::thread> workers;

        for (std::size_t p = 1; p < partsC; ++p) {
            workers.emplace_back([&, p] {
                succeeded[p] = _readRange(path, _fileSize * p / partsC, _fileSize * (p + 1) / partsC);
            });
        }

        succeeded[0] = _readRange(path, 0, _fileSize / partsC);

        for (auto &worker : workers) {
            worker.join();
        }

        //
        if (std::find(succeeded.begin(), succeeded.end(), 0) != succeeded.end()) {
            free(_ptr);
            throw std::runtime_error("Cannot read XML Library File [" + utf8String(path) + "]");
        }
    }

    ~ITunesXMLLibrary() {
        free(_ptr);
    }

    const std::string_view asStringView() const {
        return std::string_view { _ptr, static_cast<std::size_t>(_fileSize) };
    }

    // content of the "Tracks" dictionary, whatever whitespaces / line endings are used
    const std::string_view tracks() const {
        static constexpr const auto tracksKey    = std::string_view { "<key>Tracks</key>" };
        static constexpr const auto playlistsKey = std::string_view { "<key>Playlists</key>" };
        static constexpr const auto openDict     = std::string_view { "<dict>" };
        static constexpr const auto closeDict    = std::string_view { "</dict>" };

        const auto file = asStringView();

        // searches are anchored on the rarest character of each pattern
        auto begin = simd_find(file, tracksKey, 5);
        if (begin == std::string_view::npos) {
            throw std::logic_error("No Tracks found within XML Library File");
        }

        begin = file.find(openDict, begin + tracksKey.size());
        if (begin == std::string_view::npos) {
            throw std::logic_error("Malformed Tracks dictionary within XML Library File");
        }
        begin += openDict.size();

        // without playlists, Tracks dict is the last entry of the root dict
        auto endSearchAt = simd_rfind(file, playlistsKey, 5);
        if (endSearchAt == std::string_view::npos) {
            endSearchAt = file.rfind(closeDict);
        }

        const auto end = endSearchAt == std::string_view::npos || endSearchAt == 0 ?
            std::string_view::npos :
            file.rfind(closeDict, endSearchAt - 1);

        if (end == std::string_view::npos || end < begin) {
            throw std::logic_error("Malformed Tracks dictionary within XML Library File");
        }

        return file.substr(begin, end - begin);
    }

    ITunesXMLLibrary(const ITunesXMLLibrary&) = delete;
    void operator=(const ITunesXMLLibrary&) = delete;

 private:
    static constexpr const std::size_t _minPartSize = 4 << 20;
    static constexpr const std::size_t _maxParts = 8;

    const std::size_t _fileSize;
    char* _ptr;

    // reads [from, to) of the file at the same position within the buffer
    bool _readRange(const std::filesystem::path& path, const std::size_t from, const std::size_t to) {
        FILE * f = openFile(path, "rb");
        if (f == nullptr) return false;

        //
#ifdef _WIN32
        const bool seeked = _fseeki64(f, static_cast<int64_t>(from), SEEK_SET) == 0;
#else
        const bool seeked = fseeko(f, static_cast<off_t>(from), SEEK_SET) == 0;
#endif
        const auto read = seeked ? fread(_ptr + from, 1, to - from, f) : 0;

        fclose(f);
        return read == to - from;
    }
};
