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

#include <filesystem>
#include <system_error>

#include "ITunesXMLLibrary.hpp"
#include "JSONLibrary.hpp"

class ITunesLibraryParser {
 public:
    ITunesLibraryParser(const char * xmlFilePath,
                        const char * outputJSONFilePath,
                        const char * warningJSONFilePath) noexcept :
    _xmlFilePath(xmlFilePath),
    _outputJSONFilePath(outputJSONFilePath),
    _warningJSONFilePath(warningJSONFilePath) {}

    void produceOutputs() const {
        const ITunesXMLLibrary library { _xmlFilePath };
        const JSONLibrary json { library };

        //
        json.writeTracks(_outputJSONFilePath);

        // if has missing fields tracks
        if (json.warningsCount()) {
            json.writeWarnings(_warningJSONFilePath);
        } else {
            // prevent stale warnings from a previous run
            std::error_code ec;
            std::filesystem::remove(_warningJSONFilePath, ec);
        }
    }

 private:
    const char * _xmlFilePath;
    const char * _outputJSONFilePath;
    const char * _warningJSONFilePath;
};
