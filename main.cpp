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

#include "main_form.hpp"

int main()
{
    main_form form;
    form.show();
    nana::exec();
}