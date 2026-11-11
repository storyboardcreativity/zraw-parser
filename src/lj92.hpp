#pragma once

#include <stdint.h>
#include <string.h>

#include <memory>
#include <vector>

#include <tiny_dng_writer.h>

#include "bitwriter.hpp"

// The AVX2 residuals are picked at run time, no -mavx2 needed. Define ZRAW_NO_SIMD to build
// the portable code only (same as zraw-decoder-lib).
#if !defined(ZRAW_NO_SIMD) && (defined(__x86_64__) || defined(_M_X64))
#define ZRAW_HAVE_AVX2_PATH
#include <immintrin.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#define ZRAW_TARGET_AVX2 __attribute__((target("avx2")))
#else
#define ZRAW_TARGET_AVX2
#endif
#endif

// Lossless JPEG (predictor 6) encoder, produces the same bytes as tinydngwriter's lj92_encode
class Lj92Encoder
{
public:
    // On success out points at a per-thread buffer, valid until this thread encodes again.
    // Returns false for what this encoder does not handle (bit depth above 14, a value above the bit depth).
    static bool Encode(const uint16_t* image, int width, int height, int bitdepth, const uint8_t*& out, size_t& out_len)
    {
        // Residuals are 16-bit, which is enough for up to 14 bits of data
        if (bitdepth < 1 || bitdepth > 14 || width <= 0 || height <= 0)
            return false;

        thread_local std::vector<int16_t> res;
        res.resize((size_t)width * height);

        bool in_range;
#ifdef ZRAW_HAVE_AVX2_PATH
        if (_cpu_has_avx2())
            in_range = _residuals_avx2(image, width, height, bitdepth, res.data());
        else
#endif
            in_range = _residuals(image, width, height, bitdepth, res.data());

        if (!in_range)
            return false;

        tinydngwriter::lje self = {};
        self.width = width;
        self.height = height;
        self.bitdepth = bitdepth;

        {
            // Four histograms break the dependency between neighboring increments
            const uint8_t* size_class = _size_classes();
            int h[4][17] = {};
            const int16_t* q = res.data();
            size_t i = 0, count = res.size();
            for (; i + 4 <= count; i += 4)
            {
                ++h[0][size_class[_magnitude(q[i])]];
                ++h[1][size_class[_magnitude(q[i + 1])]];
                ++h[2][size_class[_magnitude(q[i + 2])]];
                ++h[3][size_class[_magnitude(q[i + 3])]];
            }
            for (; i < count; ++i)
                ++h[0][size_class[_magnitude(q[i])]];

            for (int s = 0; s < 17; ++s)
                self.hist[s] += h[0][s] + h[1][s] + h[2][s] + h[3][s];
        }
        tinydngwriter::createEncodeTable(&self);

        uint8_t header[128];
        self.encoded = header;
        self.encodedWritten = 0;
        tinydngwriter::writeHeader(&self);

        // Per size class: the Huffman code shifted above the magnitude bits, and the total length
        uint32_t code[17], len[17];
        size_t total_bits = 0;
        for (int s = 0; s < 17; ++s)
        {
            int h = self.huffsym[s];
            code[s] = (uint32_t)self.huffenc[h] << s;
            len[s] = self.huffbits[h] + s;
            total_bits += (size_t)self.hist[s] * len[s];
        }

        // total_bits / 4 is twice the size in bytes, enough even if every byte gets a stuffed 0x00
        thread_local std::unique_ptr<uint8_t[]> buf;
        thread_local size_t capacity = 0;
        const size_t need = self.encodedWritten + total_bits / 4 + 16;
        if (capacity < need)
        {
            buf.reset(new uint8_t[need]);
            capacity = need;
        }
        memcpy(buf.get(), header, self.encodedWritten);

        const uint8_t* size_class = _size_classes();
        JpegBitWriter bw(buf.get() + self.encodedWritten);
        for (size_t i = 0; i < res.size(); ++i)
        {
            int d = res[i];
            int s = size_class[_magnitude(d)];

            // Negative values are stored as d + 2^s - 1
            uint32_t mag = (uint32_t)(d - (d < 0)) & ((1u << s) - 1);
            bw.put(code[s] | mag, len[s]);
        }

        // Pad with zero bits, as the original does
        bw.flush(false);

        uint8_t* w = bw.w;
        *w++ = 0xFF;
        *w++ = 0xD9;  // EOI

        out = buf.get();
        out_len = (size_t)(w - buf.get());
        return true;
    }

private:
    // JPEG size class (number of bits of |d|, 0 for 0) for every |d| below 2^15. A table is
    // used because BSR is slow on some CPUs and not every CPU has LZCNT.
    struct SizeClasses
    {
        uint8_t of_magnitude[32768];

        SizeClasses()
        {
            of_magnitude[0] = 0;
            for (int i = 1; i < 32768; ++i)
                of_magnitude[i] = of_magnitude[i >> 1] + 1;
        }
    };

    static const uint8_t* _size_classes()
    {
        static const SizeClasses classes;
        return classes.of_magnitude;
    }

    static int _magnitude(int d)
    {
        return d < 0 ? -d : d;
    }

    // Predictor 6 residuals: row 0 predicts from the left, column 0 from above. Returns false if a
    // value does not fit into bitdepth bits.
    static bool _residuals(const uint16_t* image, int width, int height, int bitdepth, int16_t* res)
    {
        uint16_t all_bits = 0;
        for (int y = 0; y < height; ++y)
        {
            const uint16_t* row = image + (size_t)y * width;
            int16_t* r = res + (size_t)y * width;
            all_bits |= row[0];
            if (y == 0)
            {
                r[0] = (int16_t)(row[0] - (1 << (bitdepth - 1)));
                for (int x = 1; x < width; ++x)
                {
                    all_bits |= row[x];
                    r[x] = (int16_t)(row[x] - row[x - 1]);
                }
            }
            else
            {
                const uint16_t* up = row - width;
                r[0] = (int16_t)(row[0] - up[0]);
                for (int x = 1; x < width; ++x)
                {
                    all_bits |= row[x];
                    r[x] = (int16_t)(row[x] - (up[x] + ((row[x - 1] - up[x - 1]) >> 1)));
                }
            }
        }

        // The limit is a power of two, so a value reaches it only if the OR of all values does
        return all_bits < (1 << bitdepth);
    }

#ifdef ZRAW_HAVE_AVX2_PATH
    static bool _cpu_has_avx2()
    {
        static const bool has_avx2 = _detect_avx2();
        return has_avx2;
    }

    static bool _detect_avx2()
    {
#if defined(__GNUC__) || defined(__clang__)
        unsigned int a, b, c, d;
        if (__get_cpuid_max(0, nullptr) < 7)
            return false;

        // AVX needs the operating system to save the vector registers
        __cpuid(1, a, b, c, d);
        bool os_saves_registers = (c & (1 << 27)) != 0;
        bool has_avx = (c & (1 << 28)) != 0;
        if (!os_saves_registers || !has_avx)
            return false;

        unsigned int xcr0, xcr0_high;
        __asm__("xgetbv" : "=a"(xcr0), "=d"(xcr0_high) : "c"(0));
        if ((xcr0 & 6) != 6)
            return false;

        __cpuid_count(7, 0, a, b, c, d);
        return (b & (1 << 5)) != 0;
#else
        int info[4];
        __cpuid(info, 0);
        if (info[0] < 7)
            return false;

        // AVX needs the operating system to save the vector registers
        __cpuid(info, 1);
        bool os_saves_registers = (info[2] & (1 << 27)) != 0;
        bool has_avx = (info[2] & (1 << 28)) != 0;
        if (!os_saves_registers || !has_avx || (_xgetbv(0) & 6) != 6)
            return false;

        __cpuidex(info, 7, 0);
        return (info[1] & (1 << 5)) != 0;
#endif
    }

    // Same as _residuals, 16 pixels at a time on 16-bit lanes. The arithmetic shift matches C's >>.
    ZRAW_TARGET_AVX2 static bool _residuals_avx2(const uint16_t* image, int width, int height, int bitdepth, int16_t* res)
    {
        __m256i all_bits_v = _mm256_setzero_si256();
        uint16_t all_bits = 0;
        for (int y = 0; y < height; ++y)
        {
            const uint16_t* row = image + (size_t)y * width;
            int16_t* r = res + (size_t)y * width;
            all_bits |= row[0];
            int x = 1;
            if (y == 0)
            {
                r[0] = (int16_t)(row[0] - (1 << (bitdepth - 1)));
                for (; x + 16 <= width; x += 16)
                {
                    __m256i c = _mm256_loadu_si256((const __m256i*)(row + x));
                    __m256i left = _mm256_loadu_si256((const __m256i*)(row + x - 1));
                    all_bits_v = _mm256_or_si256(all_bits_v, c);
                    _mm256_storeu_si256((__m256i*)(r + x), _mm256_sub_epi16(c, left));
                }
                for (; x < width; ++x)
                {
                    all_bits |= row[x];
                    r[x] = (int16_t)(row[x] - row[x - 1]);
                }
            }
            else
            {
                const uint16_t* up = row - width;
                r[0] = (int16_t)(row[0] - up[0]);
                for (; x + 16 <= width; x += 16)
                {
                    __m256i c = _mm256_loadu_si256((const __m256i*)(row + x));
                    __m256i u = _mm256_loadu_si256((const __m256i*)(up + x));
                    __m256i cl = _mm256_loadu_si256((const __m256i*)(row + x - 1));
                    __m256i ul = _mm256_loadu_si256((const __m256i*)(up + x - 1));
                    all_bits_v = _mm256_or_si256(all_bits_v, c);
                    __m256i pred = _mm256_add_epi16(u, _mm256_srai_epi16(_mm256_sub_epi16(cl, ul), 1));
                    _mm256_storeu_si256((__m256i*)(r + x), _mm256_sub_epi16(c, pred));
                }
                for (; x < width; ++x)
                {
                    all_bits |= row[x];
                    r[x] = (int16_t)(row[x] - (up[x] + ((row[x - 1] - up[x - 1]) >> 1)));
                }
            }
        }

        uint16_t lanes[16];
        _mm256_storeu_si256((__m256i*)lanes, all_bits_v);
        for (int k = 0; k < 16; ++k)
            all_bits |= lanes[k];

        return all_bits < (1 << bitdepth);
    }
#endif
};
