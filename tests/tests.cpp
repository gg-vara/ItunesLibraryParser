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

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

#include "ITunesLibraryParser.hpp"

bool parsing_succeeded(const char * fileName) {
    const auto parser = ITunesLibraryParser {
        fileName,
        "output.json",
        "warnings.json"
    };

    parser.produceOutputs();

    return true;
}

TEST_CASE("Failing parsing on bad file name", "[parser]") {
    REQUIRE_THROWS(parsing_succeeded("iTunes Music Library (Ssss).xml"));
}

TEST_CASE("Parse small file", "[parser]") {
    const auto targetFile = "iTunes Music Library (S).xml";

    REQUIRE(parsing_succeeded(targetFile));

    BENCHMARK("Parsing") {
        return parsing_succeeded(targetFile);
    };
}

TEST_CASE("Parse with UTF-8 paths", "[parser]") {
    // non-ASCII characters, some of them outside of any Windows ANSI code page
    const auto dir = std::filesystem::path { std::u8string { u8"Bibliothèque ★ 音楽" } };
    std::filesystem::create_directories(dir);
    std::filesystem::copy_file("iTunes Music Library (S).xml", dir / "library.xml",
        std::filesystem::copy_options::overwrite_existing);

    //
    const auto utf8Dir = std::string { reinterpret_cast<const char *>(u8"Bibliothèque ★ 音楽/") };
    const auto xml = utf8Dir + "library.xml";
    const auto output = utf8Dir + "output.json";
    const auto warnings = utf8Dir + "warnings.json";

    REQUIRE_NOTHROW(ITunesLibraryParser { xml.c_str(), output.c_str(), warnings.c_str() }.produceOutputs());
    REQUIRE(std::filesystem::file_size(dir / "output.json") > 0);
}

TEST_CASE("Parse big file", "[parser]") {
    const auto targetFile = "iTunes Music Library.xml";

    REQUIRE(parsing_succeeded(targetFile));

    BENCHMARK("Parsing") {
        return parsing_succeeded(targetFile);
    };
}
