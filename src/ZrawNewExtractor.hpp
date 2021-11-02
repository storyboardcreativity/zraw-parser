#pragma once

#include <fstream>
#include <string>
#include <strstream>
#include <array>

#include "MovAvInfoDetect.hpp"
#include "IConsoleOutput.hpp"

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose

#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

uint32_t decode_zraw_block_unk(uint8_t *frame_data, uint8_t *frame_data_out, uint32_t frame_data_size, uint32_t initial_xor)
{
    uint32_t dst[34] = {0};

    uint32_t xor_value = initial_xor;
    printf("Assuming xor_value = 0x%02X\n", xor_value);

    uint8_t *ptr = frame_data;

    uint8_t pack_count = ptr[0] ^ xor_value;
    printf("pack_count: 0x%02X\n", pack_count);

    uint32_t smth_size = 0;
    uint32_t result = 0;

    if ((uint8_t)(pack_count - 1) > 30)
    {
        printf("pack_count error (%d)\n", pack_count);
        return 0;
    }

    if (pack_count)
    {
        uint8_t *ptr0 = &ptr[1];

        for (uint32_t i = 0; i < pack_count; ++i)
        {
            uint8_t a = ptr0[i * 4 + 0] ^ xor_value;
            uint8_t b = ptr0[i * 4 + 1] ^ xor_value;
            uint8_t c = ptr0[i * 4 + 2] ^ xor_value;
            uint8_t d = ptr0[i * 4 + 3] ^ xor_value;

            uint32_t v14 = (uint8_t)(d) | (((uint8_t)(c) | ((((a) << 8) | (uint8_t)(b)) << 8)) << 8);

            printf("size[%d]: 0x%08X\n", i, v14);

            smth_size += v14;
            dst[i] = v14;

            if (v14 == 0)
            {
                printf("pack_count error [%d](%d)\n", i, 0);
                return 0;
            }
        }

        printf("all packs size: 0x%08X\n", smth_size);
        printf("full data size: 0x%08X\n", smth_size + 4 * pack_count + 1);

        if (smth_size + 4 * pack_count + 1 > frame_data_size)
        {
            printf("pack_count error\n");
            return 0;
        }

        int64_t cnt = 0;
        for (uint32_t i = xor_value; i % pack_count; ++i)
            ++cnt;

        uint8_t *pFrameOut = new uint8_t[frame_data_size];
        memset(pFrameOut, 0x00, frame_data_size);

        if (cnt)
        {
            uint32_t v9 = 0;
            while (cnt < pack_count)
                v9 += dst[cnt++];

            memcpy(pFrameOut, &frame_data[1 + pack_count * 4 + v9], smth_size - v9);
            memcpy(&pFrameOut[smth_size - v9], &frame_data[1 + pack_count * 4], v9);
        }
        else
        {
            memcpy(pFrameOut, &frame_data[1 + pack_count * 4], smth_size);
        }

        uint16_t buff[16] = {0};
        uint8_t *p = &pFrameOut[smth_size - 4];
        buff[0] = p[0];
        buff[1] = p[1];
        buff[2] = p[2];
        buff[3] = p[3];

        printf("backing XOR: %02X %02X %02X %02X\n", buff[0], buff[1], buff[2], buff[3]);

        uint8_t *pOutPtr = pFrameOut;
        for (uint32_t i = 0; i < pack_count; ++i)
        {
            uint32_t hm = pOutPtr[3] | (pOutPtr[2] << 8);
            if (hm < 0x2000 || hm > 0x8000)
                hm = 0x4000;
            if (hm > dst[i])
                hm = dst[i];
            if (i == pack_count - 1 && dst[i] - hm < 4)
                hm = dst[i] - 4;

            for (uint32_t k = 0; k < ((hm - 4) >> 3); ++k)
            {
                pOutPtr[8 * k + 4] ^= pOutPtr[1];
                pOutPtr[8 * k + 5] ^= pOutPtr[1];
                pOutPtr[8 * k + 6] ^= pOutPtr[2];
                pOutPtr[8 * k + 7] ^= pOutPtr[3];
                pOutPtr[8 * k + 8] ^= buff[0];
                pOutPtr[8 * k + 9] ^= buff[1];
                pOutPtr[8 * k + 10] ^= buff[2];
                pOutPtr[8 * k + 11] ^= buff[3];
            }

            if (((uint8_t)hm - 4) & 7)
                for (int l = ((uint8_t)hm - 4) & 7; l > 0; --l)
                    pOutPtr[hm - l] ^= buff[0];

            pOutPtr[0] = 0;
            pOutPtr[1] = 0;
            pOutPtr[2] = 0;
            pOutPtr[3] = 1;
            pOutPtr += dst[i];
        }

        result = smth_size;

        memcpy(frame_data_out, pFrameOut, frame_data_size);

        delete[] pFrameOut;

        return frame_data_size;
    }

    return 0;
}

class ZrawNewExtractor
{
public:
    ZrawNewExtractor() {}
    ~ZrawNewExtractor() {}

    enum ConversionResult
    {
        Done,
        InputFileOpenFailed
    };

    ConversionResult ProcessConversion(IConsoleOutput &console, std::string zraw_file_path, std::string output_path)
    {
        console.printf("Converting file %s\n", zraw_file_path.c_str());
        console.printf("Output folder: %s\n", output_path.c_str());

        // Get tracks info from input file
        console.printf("Detecting tracks...");
        auto tracks_info = MovDetectTracks(zraw_file_path.c_str());
        console.printf(" OK!\n");

        // Open input ZRAW file
        console.printf("Opening input file...");
        std::fstream f_in(zraw_file_path, std::ios::in | std::ios::binary);
        if (!f_in.is_open())
        {
            console.printf("\nFailed to open input file!\n");
            return InputFileOpenFailed;
        }
        console.printf(" OK!\n");

        // Process each found track
        console.printf("Processing tracks...\n");
        for (int i = 0; i < tracks_info.tracks.size(); ++i)
        {
            auto &track = tracks_info.tracks[i];

            console.printf("Track #%d (\"%s\")", i, track.codec_name.c_str());
            if (track.codec_name != "zraw")
            {
                console.printf(" - not \"zraw\", skipped.\n");
                continue;
            }
            console.printf(" - processing!\n");

            console.printf("ZRAW version = 0x%X (%s)\n", track.zraw_raw_version,
                           track.zraw_raw_version == 0x12EA78D2 ? "TRUE RAW" : (track.zraw_raw_version == 0x45A32DEF ? "ENCRYPTED HEVC" : "UNKNOWN"));

            console.printf("ZRAW unk0 = %d\n", track.zraw_unk0);
            console.printf("ZRAW unk1 = %d\n", track.zraw_unk1);

            switch (track.zraw_raw_version)
            {
            case 0x12EA78D2:
                process_zraw_old_raw(console, f_in, track, i, output_path);
                break;
            case 0x45A32DEF:
                process_zraw_new_raw(console, f_in, track, i, output_path, tracks_info.creation_time);
                break;
            default:
                console.printf("Can't decode unknown ZRAW version!\n");
                break;
            }
        }

        // Close input file
        console.printf("Closing input file...");
        f_in.close();
        console.printf(" OK!\n");

        return ConversionResult::Done;
    }

protected:
    std::vector<uint8_t> zrawFrame(std::istream &f_in, TrackInfo_t &track, uint32_t frame_index)
    {
        // Extract saved offset and size
        uint32_t offset = track.frames[frame_index].frame_offset;
        uint32_t size = track.frames[frame_index].frame_size;

        // Seek read position to zraw frame offset
        f_in.seekg(offset, f_in.beg);

        // Read frame
        std::vector<uint8_t> frame(size);
        f_in.read((char *)frame.data(), size);

        return frame;
    }

    std::string exec(const char *cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
        return result;
    }

    void process_zraw_old_raw(IConsoleOutput &console, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path)
    {
        for (int i = 0; i < track.frames.size(); ++i)
        {
            // Read ZRAW frame data from .ZRAW (.MOV) file
            console.printf("Reading %d frame...\n", i);
            auto frame_data = zrawFrame(f_in, track, i);

            // Write temporary zraw frame data for our "zraw-decoder" tool
            console.printf("Saving %d frame's temp data...\n", i);
            std::string temp_zraw_frame_path = output_path + "/track_" + std::to_string(track_index) + "_" + std::to_string(i) + "._zraw";
            std::ofstream fout(temp_zraw_frame_path, std::ios::out | std::ios::binary);
            fout.write((char *)frame_data.data(), frame_data.size() * sizeof(uint8_t));
            fout.close();

            // Convert extracted ZRAW frame to DNG
            console.printf("Converting zraw %d frame to DNG...\n", i);
            std::string output_dng_path = output_path + "/track_" + std::to_string(track_index) + "_" + std::to_string(i) + ".dng";

#ifdef _MSC_VER
            std::string command = ".\\zraw-decoder";
#else
            std::string command = "./zraw-decoder";
#endif

            command += " -i " + temp_zraw_frame_path + " -o " + output_dng_path;
            auto output = exec(command.c_str());
            console.printf(output.c_str());
        }
    }

    void process_zraw_new_raw(IConsoleOutput &console, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path, uint32_t xor_value)
    {
        switch (track.zraw_unk1)
        {
        case 1:
            process_zraw_XORred(console, f_in, track, track_index, output_path, xor_value);
            break;
        case 0:
            process_zraw_AESed(console, f_in, track, track_index, output_path);
            break;
        default:
            console.printf("Can't decrypt unknown ZRAW version!\n");
            break;
        }
    }

    void process_zraw_AESed(IConsoleOutput &console, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path)
    {
        console.printf("Can't decode AESed version!\n");
    }

    void process_zraw_XORred(IConsoleOutput &console, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path, uint32_t xor_value)
    {
        // Prepare output file path
        std::string file_path = output_path + "/track_" + std::to_string(track_index) + ".avc";
        console.printf("Track output file path: %s\n", file_path.c_str());

        // Open output file
        console.printf("Opening output file...");
        std::fstream f_out(file_path, std::ios::out | std::ios::binary);
        if (!f_out.is_open())
        {
            console.printf("\nFailed to open output file!\n");
            return;
        }
        console.printf(" OK!\n");

        // Process each frame in track
        console.printf("Processing frames in track...\n");
        for (int p = 0; p < track.frames.size(); ++p)
        {
            console.printf("Frame %d...", p);

            auto size = track.universal_sample_size == -1 ? track.frames[p].frame_size : track.universal_sample_size;

            console.printf(" (size = %d)", size);

            auto data = new uint8_t[size];

            f_in.seekg(track.frames[p].frame_offset);
            f_in.read((char *)data, size);

            uint8_t *decrypted_data = new uint8_t[size];

            // DeXOR frame data
            uint32_t decrypted_size = decode_zraw_block_unk(data, decrypted_data, size, xor_value);

            f_out.write((char *)decrypted_data, decrypted_size);

            delete[] decrypted_data;

            console.printf(" OK!\n");
        }

        // Close output file
        console.printf("Closing output file...");
        f_out.close();
        console.printf(" OK!\n");
    }
};