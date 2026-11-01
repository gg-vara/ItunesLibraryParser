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

/* UPDATE THIS FILE */

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace FieldType {

// output order of the fields; keys may appear in any order within the XML file
enum Index : std::size_t {
    TrackID,
    DiscNumber,
    TrackNumber,
    Year,
    DateAdded,
    Name,
    AlbumArtist,
    Album,
    Genre,
    Count
};

// XML <key> names, which are also used as JSON keys
constexpr const std::array<std::string_view, Index::Count> fieldNames {
    "Track ID",
    "Disc Number",
    "Track Number",
    "Year",
    "Date Added",
    "Name",
    "Album Artist",
    "Album",
    "Genre",
};

// value used when the field is missing; empty means the track is reported as incomplete
constexpr const std::array<std::string_view, Index::Count> defaultValues {
    "",
    "1",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

}  // namespace FieldType
