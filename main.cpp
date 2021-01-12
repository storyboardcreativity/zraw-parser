#include <fstream>
#include <iostream>

#include "byteswap.hpp"
#include "ZRawFrameContainerParserSingletone.hpp"
#include "ZRawFrameDecompressorSingletone.hpp"

void write_pixels_to_pgm_p5(std::ostream &out, ZRawFrame &frame)
{
    uint32_t real_bitdepth = frame.Parameters().bitdepth_above_8 * 2 + 8;
    if (real_bitdepth > 16)
        return;

    // Write magic
    out << "P5" << std::endl;

    // Calc maxval
    uint32_t max_val = 0xFFFF;
    max_val >>= 16 - real_bitdepth;

    // Write "w h max_val"
    out << frame.Parameters().width << ' ' << frame.Parameters().height << ' ' << max_val << std::endl;

    for (int i = 0; i < frame.Pixels().size(); ++i)
    {
        for (int p = 0; p < frame.Pixels()[i].size(); ++p)
        {
            // Swap bytes (frame pixel is encoded in other endian)
            uint16_t value = bswap_16(frame.Pixels()[i][p]);

            // Write pixel to file
            out.write((char*)&value, sizeof(value));
        }
    }
}

int main()
{
    /// Read ZRAW frame

    std::fstream in_file("checker.raw", std::ios::in | std::ios::binary);
    if (!in_file.is_open())
    {
        std::cout << "Could not open RAW file!" << std::endl;
        return -1;
    }

    // Parse frame container to frame object
    auto frame = ZRawFrameContainerParserSingletone::Instance().ParseFrame(in_file);

    in_file.close();

    // Decompress frame to pixels
    ZRawFrameDecompressorSingletone::Instance().DecompressFrame(frame);
    
    printf("Frame width: %d\n", frame.Parameters().width);
    printf("Frame height: %d\n", frame.Parameters().height);
    printf("Frame pixels lines: %d\n", frame.Pixels().size());
    for (int i = 0; i < frame.Pixels().size(); ++i)
    {
        printf("Frame line [%d] pixels count: %d\n", i, frame.Pixels()[i].size());
        break;
    }

    /// Write PGM file

    std::fstream out_file("checker.pgm", std::ios::out | std::ios::binary);
    if (!out_file.is_open())
    {
        std::cout << "Could not open PGM file!" << std::endl;
        return -1;
    }

    write_pixels_to_pgm_p5(out_file, frame);

    out_file.close();

    return 0;
}