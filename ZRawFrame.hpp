#pragma once

#include <stdint.h>
#include <vector>

typedef struct ZRawFrameVersion_s
{
    uint32_t version;
} ZRawFrameVersion_t;

typedef struct ZRawFrameParameters_s
{
    uint32_t pattern;     // Type of bayer pattern
    uint32_t compression; // Compression mode (0 - encoded, 1 - raw)
    uint32_t __is_lossless_0;
    uint32_t width;
    uint32_t height;
    uint32_t bitdepth_above_8; // = (bitdepth_real - 8) / 2
    uint32_t __is_lossless_1;
    uint32_t is_stride_enabled;
    uint32_t line_stride;
    uint32_t align_mode;
    uint32_t debug;
    uint32_t noise_level_2;
    uint32_t noise_level_1;
    uint32_t noise_level_distance;
} ZRawFrameParameters_t;

#define ZRAW_DEFECTIVE_PIXELS_MAX 0x4000
typedef struct ZRawFrameDefectivePixelsInfo_s
{
    uint32_t tables_count;
    struct
    {
        uint32_t a;
        uint32_t bright_pixels_count;
        uint32_t bright_pixels_table[ZRAW_DEFECTIVE_PIXELS_MAX];
        uint32_t dark_pixels_count;
        uint32_t dark_pixels_table[ZRAW_DEFECTIVE_PIXELS_MAX];
    } defective_pixels_corretion_tables[32];
} ZRawFrameDefectivePixelsInfo_t;

typedef struct ZRawFrameAutoWhiteBalanceInfo_s
{
    uint32_t gain_red;
    uint32_t gain_green;
    uint32_t gain_blue;
} ZRawFrameAutoWhiteBalanceInfo_t;

typedef struct ZRawFrameSensorBlackLevels_s
{
    uint16_t levels_per_component[4];
} ZRawFrameSensorBlackLevels_t;

typedef struct ZRawFrameMetaData_s
{
    uint32_t metadata;
} ZRawFrameMetaData_t;

#define ZRAW_COLOR_CORRETION_MATRICES_MAX 10
typedef struct ZRawFrameColorCorretionMatrices_s
{
    uint32_t matrices_count;
    struct
    {
        uint16_t a;
        uint32_t b[9];
    } matrices[ZRAW_COLOR_CORRETION_MATRICES_MAX];
} ZRawFrameColorCorretionMatrices_t;

#define ZRAW_GAMMA_CURVE_POINTS_MAX 2048
typedef struct ZRawFrameGammaCurveInfo_s
{
    uint32_t curve_type; // 0 - default; 1 - sRGB; 2 - HDR; 3 - user-defined
    uint32_t curve_points_count;
    uint16_t curve_points[ZRAW_GAMMA_CURVE_POINTS_MAX];
} ZRawFrameGammaCurveInfo_t;

class ZRawFrame
{
public:
    ZRawFrameVersion_t &Version()
    {
        return _version;
    }

    ZRawFrameParameters_t &Parameters()
    {
        return _parameters;
    }

    ZRawFrameDefectivePixelsInfo_t &DefectionPixelsTable()
    {
        return _defection;
    }

    ZRawFrameAutoWhiteBalanceInfo_t &AutoWhiteBalance()
    {
        return _auto_white_balance;
    }

    ZRawFrameSensorBlackLevels_t &SensorBlackLevels()
    {
        return _black_levels;
    }

    ZRawFrameMetaData_t &MetaData()
    {
        return _metadata;
    }

    ZRawFrameColorCorretionMatrices_t &ColorCorrectionMatrices()
    {
        return _color_corretion_matrices;
    }

    ZRawFrameGammaCurveInfo_t &GammaCurveInfo()
    {
        return _gamma_curve;
    }

    std::vector<std::vector<uint8_t>> &Data()
    {
        return _data;
    }

    std::vector<std::vector<uint16_t>> &Pixels()
    {
        return _pixels;
    }

private:
    ZRawFrameVersion_t _version;
    ZRawFrameParameters_t _parameters;
    ZRawFrameDefectivePixelsInfo_t _defection;
    ZRawFrameAutoWhiteBalanceInfo_t _auto_white_balance;
    ZRawFrameSensorBlackLevels_t _black_levels;
    ZRawFrameMetaData_t _metadata;
    ZRawFrameColorCorretionMatrices_t _color_corretion_matrices;
    ZRawFrameGammaCurveInfo_t _gamma_curve;
    std::vector<std::vector<uint8_t>> _data;
    std::vector<std::vector<uint16_t>> _pixels;
};