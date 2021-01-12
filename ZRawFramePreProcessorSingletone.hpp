#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "byteswap.hpp"
#include "ZRawFrame.hpp"

class ZRawFramePreProcessorSingletone
{
    ZRawFramePreProcessorSingletone() {}
    static ZRawFramePreProcessorSingletone *_instance;

public:
    static ZRawFramePreProcessorSingletone &Instance()
    {
        if (_instance == nullptr)
            _instance = new ZRawFramePreProcessorSingletone();
        return *_instance;
    }

    void PreProcess(ZRawFrame &frame)
    {
        if (frame.MetaData().metadata == 2)
        {
            printf("Not implemented yet!\n");
            exit(-1);
        }

        uint32_t real_bitdepth = frame.Parameters().bitdepth_above_8 * 2 + 8;
        uint32_t colors_count = 2 << (real_bitdepth - 1);
        uint32_t lut_el_count = colors_count & 0x1FFFE;

        // LUT for each CFA color component
        uint16_t *lut[4] =
            {
                new uint16_t[lut_el_count],
                new uint16_t[lut_el_count],
                new uint16_t[lut_el_count],
                new uint16_t[lut_el_count]};

        // 1. Find step between sensor-black-level-relative colors
        double blk_corrected_steps[4] =
            {
                (double)colors_count / (double)(colors_count - frame.SensorBlackLevels().levels_per_component[0]),
                (double)colors_count / (double)(colors_count - frame.SensorBlackLevels().levels_per_component[1]),
                (double)colors_count / (double)(colors_count - frame.SensorBlackLevels().levels_per_component[2]),
                (double)colors_count / (double)(colors_count - frame.SensorBlackLevels().levels_per_component[3])};

        // 2. Round colors between [sensor_blc; colors_count)
        bool round_to_sensor_blc = true;

        // Process LUT for each input value
        for (uint16_t i = 0; i < colors_count; ++i)
        {
            // Process each color component
            uint16_t components[4] = {i, i, i, i};
            for (uint32_t comp_index = 0; comp_index < 4; ++comp_index)
            {
                // Round to sensor black level if needed
                if (round_to_sensor_blc)
                {
                    // Take component black level
                    uint16_t black_level = frame.SensorBlackLevels().levels_per_component[comp_index];

                    // All colors below black_level are converted to black_level
                    if (components[comp_index] < black_level)
                        components[comp_index] = black_level;

                    // Rescale color from [black_level, colors_count) to [0, colors_count)
                    components[comp_index] = fminf(colors_count, (components[comp_index] - black_level) * blk_corrected_steps[comp_index]);
                }
            }

            // Save lut for this input value
            lut[0][i] = components[0];
            lut[1][i] = components[1];
            lut[2][i] = components[2];
            lut[3][i] = components[3];
        }

        // Correct CFA with lut
        uint32_t w = frame.Parameters().width / 2;
        uint32_t h = frame.Parameters().height / 2;

        for (int i = 0; i < h; ++i)
            for (int p = 0; p < w; ++p)
            {
                uint16_t *a = &frame.Pixels()[2 * i].data()[2 * p];
                uint16_t *b = &frame.Pixels()[2 * i + 1].data()[2 * p];

                a[0] = lut[0][a[0]];
                a[1] = lut[1][a[1]];
                b[0] = lut[2][b[0]];
                b[1] = lut[3][b[1]];
            }

        delete[] lut[0];
        delete[] lut[1];
        delete[] lut[2];
        delete[] lut[3];

        // Change colors based on AWB
        double r_gain = frame.AutoWhiteBalance().gain_red;
        double g_gain = frame.AutoWhiteBalance().gain_green;
        double b_gain = frame.AutoWhiteBalance().gain_blue;
        for (int i = 0; i < h; ++i)
            for (int p = 0; p < w; ++p)
            {
                uint16_t *a = &frame.Pixels()[2 * i].data()[2 * p];
                uint16_t *b = &frame.Pixels()[2 * i + 1].data()[2 * p];

                a[0] = 4.0 * (((double)a[0] * r_gain) / 1000.0);
                a[1] = 4.0 * (((double)a[1] * g_gain) / 1000.0);
                b[0] = 4.0 * (((double)b[0] * g_gain) / 1000.0);
                b[1] = 4.0 * (((double)b[1] * b_gain) / 1000.0);
            }
    }
};