#include <fstream>
#include <iostream>

#include "byteswap.hpp"
#include "ZRawFrameContainerParserSingletone.hpp"
#include "ZRawFrameDecompressorSingletone.hpp"

#include "MovAvInfoDetect.hpp"

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
            out.write((char *)&value, sizeof(value));
        }
    }
}

int main()
{
    // Input ZRAW video file name
    std::string path = "example_E2_0_93.ZRAW";

    // Pre-process .ZRAW (.MOV) file (get tracks info)
    auto tracks = MovDetectTracks(path.c_str());

    // Create .ZRAW (.MOV) file representation based on tracks info
    ZrawMovRepresentation mov_rep(path, tracks);

    // Process each ZRAW stream
    for (int i = 0; i < mov_rep.zrawStreamsCount(); ++i)
    {
        // Prefix for all frame files related to the current stream
        std::string prefix = "s" + std::to_string(i) + "_";

        // Process each frame in the current stream
        for (int p = 0; p < mov_rep.zrawStreamFramesCount(i); ++p)
        {
            // Read ZRAW frame data from .ZRAW (.MOV) file
            auto frame_data = mov_rep.zrawFrame(i, p);

            // Interpret data as std::istream
            std::istrstream databuf_stream(reinterpret_cast<const char *>(frame_data.data()), frame_data.size());

            // Parse frame container to frame object
            auto frame = ZRawFrameContainerParserSingletone::Instance().ParseFrame(databuf_stream);

            // Decompress frame to pixels
            ZRawFrameDecompressorSingletone::Instance().DecompressFrame(frame);

            // Open output PGM frame file
            std::fstream out_file(prefix + "frame" + std::to_string(p) + ".pgm", std::ios::out | std::ios::binary);
            if (!out_file.is_open())
            {
                std::cout << "Could not open PGM file!" << std::endl;
                return -1;
            }

            // Write PGM data to file
            write_pixels_to_pgm_p5(out_file, frame);

            // Close frame file
            out_file.close();
        }
    }

    return 0;
}