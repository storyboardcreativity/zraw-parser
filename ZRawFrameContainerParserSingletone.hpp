#pragma once

#include <istream>
#include <byteswap.h>
#include <string.h>

// For AES decryption
#include <openssl/aes.h>
#include <openssl/conf.h>
#include <openssl/evp.h>

#include "ZRawFrame.hpp"

class ZRawFrameContainerParserSingletone
{
    ZRawFrameContainerParserSingletone() {}
    static ZRawFrameContainerParserSingletone *_instance;

public:
    static ZRawFrameContainerParserSingletone &Instance()
    {
        if (_instance == nullptr)
            _instance = new ZRawFrameContainerParserSingletone();
        return *_instance;
    }

    ZRawFrame ParseFrame(std::istream &stream)
    {
        ZRawFrame frame;
        _processBlock(stream, frame);
        return frame;
    }

private:
    int32_t _processBlock(std::istream &stream, ZRawFrame &frame)
    {
        uint32_t size = 0;
        stream.read((char *)&size, 4);
        size = bswap_32(size);

        char magic[5] = {0};
        stream.read((char *)magic, 4);

        printf("[block] offset: 0x%X | size = %d | type = \'%s\'\n", (uint32_t)stream.tellg() - 8, size, magic);

        int32_t position = 8;
        if (strcmp(magic, "zraw") == 0)
            position += _processZrawBlock(stream, frame, size - 8);
        else if (strcmp(magic, "ver ") == 0)
            position += _processVersionBlock(stream, frame, size - 8);
        else if (strcmp(magic, "pram") == 0)
            position += _processParametersBlock(stream, frame, size - 8);
        else if (strcmp(magic, "dpc ") == 0)
            position += _processDefectivePixelsCorrectionBlock(stream, frame, size - 8);
        else if (strcmp(magic, "awb ") == 0)
            position += _processAutoWhiteBalanceBlock(stream, frame, size - 8);
        else if (strcmp(magic, "blc ") == 0)
            position += _processSensorBlackLevelsBlock(stream, frame, size - 8);
        else if (strcmp(magic, "meta") == 0)
            position += _processMetaDataBlock(stream, frame, size - 8);
        else if (strcmp(magic, "ccm ") == 0)
            position += _processColorCorrectionMatricesBlock(stream, frame, size - 8);
        else if (strcmp(magic, "gamm") == 0)
            position += _processGammaCurveBlock(stream, frame, size - 8);
        else if (strcmp(magic, "data") == 0)
            position += _processFrameDataBlock(stream, frame, size - 8);
        else
        {
            printf("UNKNOWN BLOCK TYPE! Offset: 0x%X\n", (uint32_t)stream.tellg() - 8);
            exit(-1);
        }

        if (position != size)
        {
            printf("Failed to read block! Read size = %d\n", position);
            exit(-1);
        }
    }

    int32_t _processZrawBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        auto pos = stream.tellg();

        // Process all subblocks
        while (stream.tellg() < pos + data_size)
            _processBlock(stream, frame);

        return stream.tellg() - pos;
    }

    int32_t _processVersionBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;

        uint32_t version = 0;
        stream.read((char *)&version, 4);
        version = bswap_32(version);
        len += 4;

        frame.Version().version = version;

        uint8_t reserved_data[16];
        stream.read((char *)reserved_data, 16);
        len += 16;

        return len;
    }

    int32_t _processParametersBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        struct // sizeof == 176
        {
            uint32_t pattern;
            uint32_t compression;
            uint32_t __is_lossless_0;
            uint32_t width;
            uint32_t height;
            uint32_t bitdepth_above_8;
            uint32_t __is_lossless_1;
            uint32_t __unk1;
            uint32_t __unk2;
            uint32_t is_stride_enabled;
            uint32_t line_stride;
            uint32_t align_mode;
            uint32_t debug;
            uint32_t budget_mb_bits;
            uint32_t __unk3[15];
            uint32_t buffer_init_bits;
            uint32_t __unk4[11];
            uint32_t noise_level_2;
            uint32_t noise_level_1;
            uint32_t noise_level_distance;
        } data;
        stream.read((char *)&data, sizeof(data));

        frame.Parameters().pattern = data.pattern;
        frame.Parameters().compression = data.compression;
        frame.Parameters().__is_lossless_0 = data.__is_lossless_0;
        frame.Parameters().width = data.width;
        frame.Parameters().height = data.height;
        frame.Parameters().bitdepth_above_8 = data.bitdepth_above_8;
        frame.Parameters().__is_lossless_1 = data.__is_lossless_1;
        frame.Parameters().is_stride_enabled = data.is_stride_enabled;
        frame.Parameters().line_stride = data.line_stride;
        frame.Parameters().align_mode = data.align_mode;
        frame.Parameters().debug = data.debug;
        frame.Parameters().noise_level_2 = data.noise_level_2;
        frame.Parameters().noise_level_1 = data.noise_level_1;
        frame.Parameters().noise_level_distance = data.noise_level_distance;

        return sizeof(data);
    }

    int32_t _processDefectivePixelsCorrectionBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;

        uint32_t tables_count = 0;
        stream.read((char *)&tables_count, 4);
        tables_count = bswap_32(tables_count);
        frame.DefectionPixelsTable().tables_count = tables_count;
        len += 4;

        if (tables_count > 32)
        {
            printf("iso_num (%d > %d) error\n", tables_count, 32);
            exit(-1);
        }

        for (uint32_t i = 0; i < tables_count; ++i)
        {
            uint32_t a = 0;
            stream.read((char *)&a, 4);
            a = bswap_32(a);
            len += 4;
            frame.DefectionPixelsTable().defective_pixels_corretion_tables[i].a = a;

            uint32_t bright_pixels_count = 0;
            stream.read((char *)&bright_pixels_count, 4);
            bright_pixels_count = bswap_32(bright_pixels_count);
            len += 4;
            frame.DefectionPixelsTable().defective_pixels_corretion_tables[i].bright_pixels_count = bright_pixels_count;

            if (bright_pixels_count > ZRAW_DEFECTIVE_PIXELS_MAX)
            {
                printf("bright_pixels_count can't be more than %d! Got: %d\n", ZRAW_DEFECTIVE_PIXELS_MAX, bright_pixels_count);
                exit(-1);
            }

            for (uint32_t p = 0; p < bright_pixels_count; ++p)
            {
                uint32_t bright_table_value = 0;
                stream.read((char *)&bright_table_value, 4);
                bright_table_value = bswap_32(bright_table_value);
                len += 4;
                frame.DefectionPixelsTable().defective_pixels_corretion_tables[i].bright_pixels_table[p] = bright_table_value;
            }

            uint32_t dark_pixels_count = 0;
            stream.read((char *)&dark_pixels_count, 4);
            dark_pixels_count = bswap_32(dark_pixels_count);
            len += 4;
            frame.DefectionPixelsTable().defective_pixels_corretion_tables[i].dark_pixels_count = dark_pixels_count;

            if (dark_pixels_count > 0x4000)
            {
                printf("dark_pixels_count can't be more than %d! Got: %d\n", ZRAW_DEFECTIVE_PIXELS_MAX, dark_pixels_count);
                exit(-1);
            }

            for (uint32_t p = 0; p < dark_pixels_count; ++p)
            {
                uint32_t dark_table_value = 0;
                stream.read((char *)&dark_table_value, 4);
                dark_table_value = bswap_32(dark_table_value);
                len += 4;
                frame.DefectionPixelsTable().defective_pixels_corretion_tables[i].dark_pixels_table[p] = dark_table_value;
            }
        }

        return len;
    }

    int32_t _processAutoWhiteBalanceBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;

        uint32_t gain_red = 0;
        stream.read((char *)&gain_red, 4);
        len += 4;
        frame.AutoWhiteBalance().gain_red = gain_red;

        uint32_t gain_green = 0;
        stream.read((char *)&gain_green, 4);
        len += 4;
        frame.AutoWhiteBalance().gain_green = gain_green;

        uint32_t gain_blue = 0;
        stream.read((char *)&gain_blue, 4);
        len += 4;
        frame.AutoWhiteBalance().gain_blue = gain_blue;

        return len;
    }

    int32_t _processSensorBlackLevelsBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;

        struct
        {
            uint16_t levels_per_component[4];
        } data;
        stream.read((char *)&data, sizeof(data));
        len += sizeof(data);

        for (int i = 0; i < 4; ++i)
            frame.SensorBlackLevels().levels_per_component[i] = data.levels_per_component[i];

        return len;
    }

    int32_t _processMetaDataBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;

        uint32_t metadata = 0;
        stream.read((char *)&metadata, 4);
        metadata = bswap_32(metadata);
        len += 4;

        frame.MetaData().metadata = metadata;

        return len;
    }

    int32_t _processColorCorrectionMatricesBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;

        // Read count
        uint32_t matrices_count = 0;
        stream.read((char *)&matrices_count, 4);
        matrices_count = bswap_32(matrices_count);
        len += 4;

        frame.ColorCorrectionMatrices().matrices_count = matrices_count;

        if (matrices_count <= 0 || ZRAW_COLOR_CORRETION_MATRICES_MAX < matrices_count)
        {
            printf("Error!\n");
            exit(-1);
        }

        struct // sizeof == 40
        {
            uint16_t a;
            uint16_t __gap;
            uint32_t b[9];
        } matrices[10];

        if (sizeof(matrices) < data_size - 4)
        {
            printf("Error!\n");
            exit(-1);
        }

        // Read AES-encrypted buffer
        uint8_t *buff = new uint8_t[data_size - 4];
        stream.read((char *)buff, data_size - 4);
        len += data_size - 4;

        // Decrypt read buffer
        uint8_t ivec[16];
        memset(&ivec, 0x00, 16);
        AES_KEY aes_key_obj;
        AES_set_decrypt_key((uint8_t *)"jisofsdguhfusxc", 128, &aes_key_obj);
        AES_cbc_encrypt(buff, (uint8_t *)matrices, data_size - 4, &aes_key_obj, ivec, AES_DECRYPT);

        // Delete buff
        delete[] buff;

        // Save read matrices info
        for (int i = 0; i < matrices_count; ++i)
        {
            frame.ColorCorrectionMatrices().matrices[i].a = matrices[i].a;
            for (int p = 0; p < matrices_count; ++p)
                frame.ColorCorrectionMatrices().matrices[i].b[p] = matrices[i].b[p];
        }

        return len;
    }

    int32_t _processGammaCurveBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;

        uint32_t curve_type = 0;
        stream.read((char *)&curve_type, 4);
        curve_type = bswap_32(curve_type);
        len += 4;

        frame.GammaCurveInfo().curve_type = curve_type;

        uint32_t curve_points_count = 0;
        stream.read((char *)&curve_points_count, 4);
        curve_points_count = bswap_32(curve_points_count);
        len += 4;

        frame.GammaCurveInfo().curve_points_count = curve_points_count;

        for (uint32_t i = 0; i < curve_points_count; ++i)
        {
            uint16_t value = 0;
            stream.read((char *)&value, 2);
            value = bswap_32(value);
            len += 2;

            frame.GammaCurveInfo().curve_points[i] = value;
        }

        return len;
    }

    int32_t _processFrameDataBlock(std::istream &stream, ZRawFrame &frame, uint32_t data_size)
    {
        uint32_t len = 0;
        
        // Read each frame line
        for (int i = 0; i < frame.Parameters().height; ++i)
        {
            // Read image line header (line bytes * 16 count)
            uint16_t count = 0;
            stream.read((char *)&count, 2);
            len += 2;

            // Check for stride errors
            if (frame.Parameters().line_stride < count + 1)
            {
                printf("Stride error!\n");
                exit(-1);
            }

            // Read image line data (-sizeof(uint16_t) because we've read an uint16_t before)
            std::vector<uint8_t> line_data;
            for (int p = 0; p < (16 * (count + 1) - sizeof(uint16_t)) / 2; ++p)
            {
                uint16_t data;
                stream.read((char *)&data, sizeof(uint16_t));
                len += 2;

                // Save read line cell value
                line_data.push_back(((uint8_t*)&data)[0]);
                line_data.push_back(((uint8_t*)&data)[1]);
            }
            
            // Save frame line data
            frame.Data().push_back(line_data);
        }

        return len;
    }
};