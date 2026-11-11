#pragma once

#include <stdint.h>
#include <string.h>

#include "byteswap.hpp"

// JPEG bit writer: MSB first, a 0x00 is stuffed after every 0xFF byte.
// Writes 32 bits at a time, so it needs a little-endian machine.
struct JpegBitWriter
{
    uint8_t* w;
    uint64_t acc;
    int n;

    explicit JpegBitWriter(uint8_t* out) : w(out), acc(0), n(0) {}

    void byte(uint8_t b)
    {
        *w++ = b;
        if (b == 0xFF)
            *w++ = 0;
    }

    // v holds exactly len bits, len <= 32
    void put(uint32_t v, int len)
    {
        acc = (acc << len) | v;
        n += len;
        if (n >= 32)
        {
            n -= 32;

            // A 0xFF byte in word is a zero byte in inv
            uint32_t word = (uint32_t)(acc >> n), inv = ~word;
            if (((inv - 0x01010101u) & ~inv & 0x80808080u) == 0)
            {
                word = bswap_32(word);
                memcpy(w, &word, 4);
                w += 4;
            }
            else
            {
                for (int k = 24; k >= 0; k -= 8)
                    byte((uint8_t)(word >> k));
            }
        }
    }

    // Pads the last partial byte with 1 or 0 bits
    void flush(bool ones)
    {
        while (n >= 8)
        {
            n -= 8;
            byte((uint8_t)(acc >> n));
        }

        if (n)
            byte((uint8_t)((acc << (8 - n)) | (ones ? 0xFF >> n : 0)));

        n = 0;
    }
};
