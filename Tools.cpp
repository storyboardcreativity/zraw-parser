#include <memory.h>
#include <stdio.h>

#include "Tools.h"

#define ZRAW_LINE_BLOCK_SIZE 32

// Rounds to [begin; end)
int round(int begin, int val, int end)
{
    if (val < end)
    {
        if (val >= begin)
            return val;
        return begin;
    }
    return end - 1;
}

void __post_process_a(ZRawImageBlockLine &line, bool is_needed_field, int noise_level)
{
    std::vector<uint16_t> tmp(line.Line().size(), 0x0000);

    for (uint32_t i = 0; i < line.Line().size(); ++i)
    {
        int index0 = round(0, i - 2 + is_needed_field, line.Line().size());
        int index3 = round(0, i - 1 + is_needed_field, line.Line().size());
        int index2 = round(0, i + 0 + is_needed_field, line.Line().size());
        int index1 = round(0, i + 1 + is_needed_field, line.Line().size());

        tmp[i] = (8 * line.Line()[index2] + 8 * line.Line()[index3] + 8) / 16;

        if (noise_level)
            if (noise_level != 1)
                tmp[i] = (3 * line.Line()[index1] + 5 * line.Line()[index2] + 5 * line.Line()[index3] + 3 * line.Line()[index0] + 8) / 16;
    }

    memcpy(line.Line().data(), tmp.data(), line.Line().size() * sizeof(uint16_t));
}

void __post_process_b(ZRawImageBlockLine &line, int noise_level)
{
    std::vector<uint16_t> tmp(line.Line().size(), 0x0000);

    for (uint32_t i = 0; i < line.Line().size(); ++i)
    {
        int block_header_val = 1 << line.HeaderValues()[i / ZRAW_LINE_BLOCK_SIZE];

        int index0 = round(0, i - 2, line.Line().size());
        int index1 = round(0, i - 1, line.Line().size());
        int index2 = round(0, i, line.Line().size());
        int index3 = round(0, i + 1, line.Line().size());
        int index4 = round(0, i + 2, line.Line().size());

        if (noise_level)
        {
            if (noise_level == 1)
                tmp[i] = (4 * line.Line()[index3] + 8 * line.Line()[index2] + 4 * line.Line()[index1] + 8) / 16;
            else
                tmp[i] = (2 * line.Line()[index4] + 4 * line.Line()[index3] + 4 * line.Line()[index2] + 4 * line.Line()[index1] + 2 * line.Line()[index0] + 8) / 16;
        }
        else
        {
            int v18 = (line.Line()[index3] + 2 * line.Line()[index2] + line.Line()[index1] + 2) / 4;
            int offset = block_header_val / 2;
            
            if (v18 - line.Line()[index2] <= block_header_val / 2)
                offset = (v18 - line.Line()[index2] >= -block_header_val / 2) ? v18 - line.Line()[index2] : -block_header_val / 2;
                
            tmp[i] = offset + line.Line()[index2];
        }
    }
    
    memcpy(line.Line().data(), tmp.data(), line.Line().size() * sizeof(uint16_t));
}

void __post_process_truncate(std::vector<uint16_t> &line, int real_bitdepth, int diff)
{
    int bit_count = real_bitdepth - diff;
    if (bit_count < 0)
        bit_count = 0;
    
    for (uint32_t i = 0; i < line.size(); ++i)
        line[i] = (signed int)line[i] >> bit_count << bit_count;
}

void __place_noise_level(unsigned int noise_level, unsigned int *noise_levels)
{
    // [A B C D E F G H]
    // to
    // [A A B C D E F G]
    // (move all elements to the right by 1)
    for (int i = 7; i > 0; --i)
        noise_levels[i] = noise_levels[i - 1];

    // First element is set from outside
    // [A A B C D E F G]
    // to
    // [X A B C D E F G], where X = noise_level
    noise_levels[0] = noise_level;
}

int64_t __select_noise_level(uint32_t *noise_levels)
{
    int cnt_0 = 0;
    int cnt_1 = 0;
    int cnt_2 = 0;
    for (int i = 0; i < 8; ++i)
    {
        switch (noise_levels[i])
        {
        case 0:
            ++cnt_0;
            break;
        case 1:
            ++cnt_1;
            break;
        case 2:
            ++cnt_2;
            break;
        }
    }

    // If cnt_0 is biggest - return 0
    if (cnt_0 >= cnt_1 && cnt_0 >= cnt_2)
        return 0;

    // If cnt_1 is biggest - return 1
    if (cnt_1 >= cnt_0 && cnt_1 >= cnt_2)
        return 1;

    // If cnt_2 is biggest - return 2
    return 2;
}

uint32_t __estimate_noise_level(int noise_level_1, int noise_level_2, int less_than_distance_pix_count, uint32_t noise_levels[8])
{
    if (less_than_distance_pix_count >= noise_level_2)
        __place_noise_level(less_than_distance_pix_count < noise_level_1, noise_levels);
    else
        __place_noise_level(2u, noise_levels);
    
    return __select_noise_level(noise_levels);
}