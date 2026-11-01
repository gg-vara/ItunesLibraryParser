# ItunesLibraryParser
[![CodeFactor](https://www.codefactor.io/repository/github/amphaal/ituneslibraryparser/badge)](https://www.codefactor.io/repository/github/amphaal/ituneslibraryparser)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/33cc1a2ab72b410ab6732ce7c6581a08)](https://www.codacy.com/gh/Amphaal/ItunesLibraryParser/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Amphaal/ItunesLibraryParser&amp;utm_campaign=Badge_Grade)

**Turn your iTunes library into clean JSON, in the blink of an eye.**

ItunesLibraryParser reads the `iTunes Music Library.xml` file exported by iTunes and produces a compact JSON list of your tracks, ready to be consumed by any app, website or script.

A library of 25,000 tracks is converted in **under 20 milliseconds** : literally faster than a blink of an eye.

## Why ItunesLibraryParser ?

- ⚡ **Blazing fast** : built around the vector instructions of modern CPUs and all of their cores. Converting your library feels instantaneous, even with tens of thousands of tracks.
- ✅ **Accurate** : every track is checked, and special characters are handled properly. `Simon & Garfunkel`, `"Live"` versions or `R★O★C★K★S` all come out exactly as displayed in iTunes.
- 🧹 **Clean data** : tracks missing important information (album, artist...) are set apart in a separate warnings file, so that your app only receives complete tracks, and you know exactly what to fix in your library.
- 💻 **Cross-platform** : Windows and macOS, on Intel / AMD (AVX2) and Apple Silicon (Neon) processors.
- 🔌 **Easy to embed** : a header-only C++ library, added to your project with a couple of CMake lines.

## Performance

Time to convert a full library, from reading the XML file to writing the JSON files :

| Library                   | Size    | Tracks | v0.1 engine | **v0.2.0**   |
|---------------------------|--------:|-------:|------------:|-------------:|
| Sample library (included) | 45.5 MB | 24,574 | 43 ms       | **14 ms**    |
| Real-world library        | 56.5 MB | 29,453 | 46 ms       | **17 ms**    |

That is **3x faster** than the previous engine.

<sub>Measured on an AMD Ryzen 9 5900X, Windows 11, library file already in the system cache, median of 30 runs.</sub>

## What you get

`output.json`, one entry per track (indented here for readability, the actual file is compact) :

```json
[
    {
        "Track ID": "74",
        "Disc Number": "1",
        "Track Number": "46",
        "Year": "2015",
        "Date Added": "2018-09-05T23:43:47Z",
        "Name": "The Tree When We Sat Once",
        "Album Artist": "Marcin Przybylowicz & Mikolai Stroinski",
        "Album": "The Witcher 3: Wild Hunt (Original Game Soundtrack)",
        "Genre": "Game Soundtrack"
    }
]
```

`warnings.json` lists the tracks with missing information, in the same format, so you can easily spot them. When nothing is missing, no warnings file is produced.

The exported fields can be customized in [`src/config/TracksScan.hpp`](src/config/TracksScan.hpp).

## Getting started

Add the library to your CMake project :

```cmake
add_subdirectory(ItunesLibraryParser)
target_link_libraries(your-app PRIVATE itunes-library-parser)
```

Then convert a library in one line :

```cpp
#include "ITunesLibraryParser.hpp"

ITunesLibraryParser { "iTunes Music Library.xml", "output.json", "warnings.json" }.produceOutputs();
```

A C++20 compiler (clang recommended) and a CPU supporting AVX2 (any x64 CPU since 2013) or Neon (Apple Silicon) are required.

## Run the benchmark yourself

```bash
cmake -GNinja -B build -DBUILD_TESTING=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build
cd build/tests
./ITunesLibraryParser_bench "iTunes Music Library.xml" 30
```

You can also point it to your own library file to see how fast it is on your machine.

## License
    ItunesLibraryParser
    Allows JSON parsing of XML Itunes Library file
    Copyright (C) 2021 Guillaume Vara <guillaume.vara@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Any graphical resources available within the source code may
    use a different license and copyright : please refer to their metadata
    for further details. Graphical resources without explicit references to a
    different license and copyright still refer to this GPL.
