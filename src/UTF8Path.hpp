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
#include <string.h>

#include <filesystem>
#include <string>
#include <string_view>

// File paths are given as UTF-8 strings, whatever the platform.
// On Windows, narrow paths would otherwise be interpreted with the ANSI code page.

inline std::filesystem::path utf8Path(const char * path) {
    return std::filesystem::path { std::u8string_view { reinterpret_cast<const char8_t *>(path) } };
}

// UTF-8 representation, for messages
inline std::string utf8String(const std::filesystem::path& path) {
    const auto u8 = path.u8string();
    return std::string { u8.begin(), u8.end() };
}

inline FILE * openFile(const std::filesystem::path& path, const char * mode) {
#ifdef _WIN32
    const std::wstring wideMode { mode, mode + strlen(mode) };
    return _wfopen(path.c_str(), wideMode.c_str());
#else
    return fopen(path.c_str(), mode);
#endif
}
