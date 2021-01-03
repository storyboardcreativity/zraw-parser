#pragma once

#include <stdint.h>
#include <vector>

#define ZRAW_LINE_BLOCK_SIZE 32

class ZRawImageBlockLine
{
public:
    ZRawImageBlockLine(uint32_t block_count, uint16_t default_value)
        : _values(std::vector<uint16_t>(block_count * ZRAW_LINE_BLOCK_SIZE, default_value)),
          _header_values(block_count, 0) {}

    uint16_t *operator[](const int index)
    {
        return &_values.data()[index * ZRAW_LINE_BLOCK_SIZE];
    }

    std::vector<uint16_t> &Line()
    {
        return _values;
    }

    std::vector<uint32_t> &HeaderValues()
    {
        return _header_values;
    }

private:
    std::vector<uint16_t> _values;
    std::vector<uint32_t> _header_values;
};