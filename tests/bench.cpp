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

// Per-stage timings of the parser
// usage : bench [xmlFile] [iterations]

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <chrono>
#include <vector>

#include "ITunesLibraryParser.hpp"

using Clock = std::chrono::steady_clock;

struct Samples : public std::vector<double> {
    void print(const char * name) {
        std::sort(begin(), end());
        printf("%-10s min %8.2f ms   median %8.2f ms\n", name, front(), (*this)[size() / 2]);
    }
};

static double msSince(const Clock::time_point& from) {
    return std::chrono::duration<double, std::milli>(Clock::now() - from).count();
}

int main(int argc, char ** argv) {
    const char * xmlFile = argc > 1 ? argv[1] : "iTunes Music Library.xml";
    const int iterations = argc > 2 ? atoi(argv[2]) : 30;

    Samples load, convert, write, total;
    std::size_t tracksCount = 0;

    // first run is a warmup (file cache)
    for (int i = 0; i <= iterations; ++i) {
        const auto t0 = Clock::now();

        // load
        const ITunesXMLLibrary library { xmlFile };
        const auto t1 = Clock::now();

        // scan and generate JSON
        const JSONLibrary json { library };
        tracksCount = json.tracksCount() + json.warningsCount();
        const auto t2 = Clock::now();

        // write
        json.writeTracks("output.json");
        json.writeWarnings("warnings.json");

        if (i == 0) continue;

        load.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        convert.push_back(std::chrono::duration<double, std::milli>(t2 - t1).count());
        write.push_back(msSince(t2));
        total.push_back(msSince(t0));
    }

    printf("%s : %zu tracks, %d iterations\n", xmlFile, tracksCount, iterations);
    load.print("load");
    convert.print("convert");
    write.print("write");
    total.print("total");

    return 0;
}
