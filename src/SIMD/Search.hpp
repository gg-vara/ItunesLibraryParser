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
#include <string.h>

#include <cstddef>
#include <string_view>

#if defined(__AVX2__)
    #include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
#else
    #error "AVX2 or Neon instruction set is required"
#endif

// bitmask of the bytes equal to "c" within the 64 bytes starting at "p"
inline uint64_t simd_char_mask64(const char* p, const char c) {
#if defined(__AVX2__)
    const auto needle = _mm256_set1_epi8(c);
    const auto lo = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
    const auto hi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + 32));
    const uint32_t loMask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(lo, needle));
    const uint32_t hiMask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(hi, needle));
    return loMask | (static_cast<uint64_t>(hiMask) << 32);
#else
    const auto src = reinterpret_cast<const uint8_t*>(p);
    const auto needle = vdupq_n_u8(static_cast<uint8_t>(c));
    const uint8x16_t bits = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
    };

    const auto m0 = vandq_u8(vceqq_u8(vld1q_u8(src),      needle), bits);
    const auto m1 = vandq_u8(vceqq_u8(vld1q_u8(src + 16), needle), bits);
    const auto m2 = vandq_u8(vceqq_u8(vld1q_u8(src + 32), needle), bits);
    const auto m3 = vandq_u8(vceqq_u8(vld1q_u8(src + 48), needle), bits);

    // pairwise adds pack each 8 bytes group into one byte
    auto sum = vpaddq_u8(vpaddq_u8(m0, m1), vpaddq_u8(m2, m3));
    sum = vpaddq_u8(sum, sum);
    return vgetq_lane_u64(vreinterpretq_u64_u8(sum), 0);
#endif
}

// first position of "needle" within "s" from "from", or npos; candidates are filtered on "needle[anchor]",
// which should be a rare character of the needle
inline std::size_t simd_find(const std::string_view& s, const std::string_view& needle,
                             const std::size_t anchor, const std::size_t from = 0) {
    if (needle.empty() || anchor >= needle.size() || s.size() < needle.size()) return std::string_view::npos;

    // candidates starting positions are within [b, last]
    const std::size_t last = s.size() - needle.size();

    for (std::size_t b = from; b <= last; b += 64) {
        auto mask = simd_char_mask64(s.data() + b + anchor, needle[anchor]);

        const auto span = last - b + 1;
        if (span < 64) mask &= (uint64_t { 1 } << span) - 1;

        //
        while (mask != 0) {
            const auto pos = b + __builtin_ctzll(mask);
            if (memcmp(s.data() + pos, needle.data(), needle.size()) == 0) return pos;
            mask &= mask - 1;
        }
    }

    return std::string_view::npos;
}

// last position of "needle" within "s", or npos; candidates are filtered on "needle[anchor]",
// which should be a rare character of the needle
inline std::size_t simd_rfind(const std::string_view& s, const std::string_view& needle, const std::size_t anchor) {
    if (needle.empty() || anchor >= needle.size() || s.size() < needle.size()) return std::string_view::npos;

    // candidates starting positions are within [b, last]
    std::size_t last = s.size() - needle.size();
    std::size_t b = last >= 63 ? last - 63 : 0;

    while (true) {
        auto mask = simd_char_mask64(s.data() + b + anchor, needle[anchor]);

        const auto span = last - b + 1;
        if (span < 64) mask &= (uint64_t { 1 } << span) - 1;

        //
        while (mask != 0) {
            const auto bit = 63 - __builtin_clzll(mask);
            const auto pos = b + bit;
            if (memcmp(s.data() + pos, needle.data(), needle.size()) == 0) return pos;
            mask &= ~(uint64_t { 1 } << bit);
        }

        //
        if (b == 0) return std::string_view::npos;
        last = b - 1;
        b = last >= 63 ? last - 63 : 0;
    }
}

// Iterates over every '<' position of [begin, end), 64 bytes at a time.
// May read up to 63 bytes past "end" : the underlying buffer must be padded accordingly.
struct TagCursor {
 public:
    TagCursor(const char* begin, const char* end) :
        _block(begin), _end(end), _mask(simd_char_mask64(begin, '<')) {}

    // position of the next '<', or "end" if none
    const char* next() {
        while (_mask == 0) {
            _block += 64;
            if (_block >= _end) return _end;
            _mask = simd_char_mask64(_block, '<');
        }

        //
        const char* found = _block + __builtin_ctzll(_mask);
        _mask &= _mask - 1;

        return found < _end ? found : _end;
    }

 private:
    const char* _block;
    const char* const _end;
    uint64_t _mask;
};
