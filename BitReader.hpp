#pragma once

#include <istream>
#include <stdexcept>

class BitReader
{
public:
    BitReader(std::istream &stream)
        : _stream(stream),
          _curr_byte(0x00),
          _bits_left_in_curr_byte(0),
          _position_in_bits(0)
    {
        _stream.seekg(0, _stream.beg);
    }

    uint64_t ReadBits(uint32_t bit_count)
    {
        if (bit_count == 0)
            return 0;
        
        if (bit_count > sizeof(uint64_t) * 8)
            throw new std::invalid_argument("bit_count can't be more than 64!");

        uint64_t value = 0;
        for (uint32_t i = 0; i < bit_count; ++i)
            value |= (((uint64_t)_getNextBit()) << i);

        return value;
    }

    void FlushBits(uint32_t bit_count)
    {
        SeekToBit(CurrentPositionInBits() + bit_count);
    }

    void BitAlignTo(uint32_t value)
    {
        if (value == 0)
            return;
        
        uint32_t mod = _position_in_bits % value;
        if (mod)
            FlushBits(value - mod);
    }
    
    void SeekToBit(uint64_t bit_index)
    {
        uint64_t byte_index = bit_index / 8;
        _stream.seekg(byte_index, _stream.beg);

        // Reset counters and refresh current byte
        _bits_left_in_curr_byte = 0;
        _position_in_bits = bit_index;
        _refreshCurrentByte();

        // Skip last bits
        _curr_byte >>= (bit_index % 8);
        _bits_left_in_curr_byte -= bit_index % 8;
    }

    uint64_t ShowBits(uint32_t bit_count)
    {
        // Save current position
        uint64_t bits_before = _position_in_bits;

        // Read bits
        uint64_t bits_to_show = ReadBits(bit_count);

        // Set carry back to saved position
        SeekToBit(bits_before);

        // Return result
        return bits_to_show;
    }

    uint64_t CurrentPositionInBits()
    {
        return _position_in_bits;
    }

private:
    void _refreshCurrentByte()
    {
        // If we have no bits left in current byte
        if (_bits_left_in_curr_byte == 0)
        {
            // Read next byte
            _stream.read((char *)&_curr_byte, 1);

            // Refresh left bits counter
            _bits_left_in_curr_byte = 8;
        }
    }

    bool _getNextBit()
    {
        // Refresh current byte in case we have no bits left
        _refreshCurrentByte();

        // Get current bit
        bool result = _curr_byte & 0x01;

        // Make next bit current
        _curr_byte >>= 1;

        // Refresh bit counters
        --_bits_left_in_curr_byte;
        ++_position_in_bits;

        // Return saved bit
        return result;
    }

    std::istream &_stream;
    uint8_t _curr_byte;
    int _bits_left_in_curr_byte;
    uint64_t _position_in_bits;
};