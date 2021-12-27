#pragma once

#include <fstream>
#include <string>
#include <strstream>
#include <atomic>

#include <MovAvInfoDetect.hpp>
#include <IConsoleOutput.hpp>
#include <IProgressBar.hpp>

#define TINY_DNG_WRITER_IMPLEMENTATION
#include <tiny_dng_writer.h>

#define LIB_ZRAW_STATIC
#include <libzraw.h>

// for windows mkdir
#ifdef _WIN32
#include <direct.h>
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
        InputFileOpenFailed,
        OutputFileOpenFailed,
        DirectoryCreationFailed,
        Interrupted,
        NotImplemented
    };

    ConversionResult ProcessConversion(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::string zraw_file_path, std::string output_path)
    {
        console.printf("Converting file %s\n", zraw_file_path.c_str());

        std::string fileName = remove_extension(base_name(zraw_file_path));
        auto dirPath = output_path + "/" + fileName;
        console.printf("Output folder: %s\n", dirPath.c_str());

#ifdef _WIN32
        if(_mkdir(dirPath.c_str()) != 0)
        {
            if (errno != EEXIST)
            {
                console.printf("Could not create folder: %s\n", dirPath.c_str());
                return DirectoryCreationFailed;
            }
        }
#else
#error !!! mkdir for Linux is not implemented yet!
#endif

        // Get tracks info from input file
        console.printf("Detecting tracks...\n");
        progressBar.SetDescription("Detecting tracks...");
        auto tracks_info = MovDetectTracks(zraw_file_path.c_str());

        console.printf(tracks_info.output_log.c_str());

        if (itsTimeToStopOkay)
            return Interrupted;

        // Open input ZRAW file
        console.printf("Opening input file...\n");
        progressBar.SetDescription("Opening input file...");
        std::fstream f_in(zraw_file_path, std::ios::in | std::ios::binary);
        if (!f_in.is_open())
        {
            console.printf("Failed to open input file!\n");
            return InputFileOpenFailed;
        }

        // Process each found track
        console.printf("Processing tracks...\n");
        progressBar.SetDescription("Processing tracks...");
        for (int i = 0; i < tracks_info.tracks.size(); ++i)
        {
            if (itsTimeToStopOkay)
            {
                f_in.close();
                return Interrupted;
            }

            auto &track = tracks_info.tracks[i];

            if (track.codec_name != "zraw")
            {
                console.printf("Track #%d (\"%s\") - not ZRAW, skipped.\n", i, track.codec_name.c_str());
                continue;
            }
            console.printf("Track #%d (\"%s\")\n", i, track.codec_name.c_str());

            console.printf("ZRAW version = 0x%X (%s)\n", track.zraw_raw_version,
                           track.zraw_raw_version == 0x12EA78D2 ? "TRUE RAW" : (track.zraw_raw_version == 0x45A32DEF ? "ENCRYPTED HEVC" : "UNKNOWN"));

            console.printf("ZRAW unk0 = %d\n", track.zraw_unk0);
            console.printf("ZRAW unk1 = %d\n", track.zraw_unk1);

            ConversionResult subRes = Done;
            switch (track.zraw_raw_version)
            {
            case 0x12EA78D2:
                subRes = process_zraw_old_raw(itsTimeToStopOkay, console, progressBar, f_in, track, i, dirPath);
                break;
            case 0x45A32DEF:
                subRes = process_zraw_new_raw(itsTimeToStopOkay, console, progressBar, f_in, track, i, dirPath, tracks_info.creation_time);
                break;
            default:
                console.printf("Can't decode unknown ZRAW version!\n");
                subRes = NotImplemented;
            }

            if (subRes != Done)
            {
                f_in.close();
                return subRes;
            }
        }

        // Close input file
        console.printf("Closing input file...\n");
        progressBar.SetDescription("Closing input file...");
        f_in.close();

        return ConversionResult::Done;
    }

protected:
    std::vector<uint8_t> zrawFrame(std::istream &f_in, TrackInfo_t &track, uint32_t frame_index)
    {
        // Extract saved offset and size
        const auto offset = track.frames[frame_index].frame_offset;
        const auto size = track.frames[frame_index].frame_size;

        // Seek read position to zraw frame offset
        f_in.seekg(offset, f_in.beg);

        // Read frame
        std::vector<uint8_t> frame(size);
        f_in.read((char *)frame.data(), size);

        return frame;
    }

    bool process_zraw_decoder_state(IConsoleOutput &console, zraw_decoder_state_t state)
    {
        switch (state)
        {
        case ZRAW_DECODER_STATE__INVALID_INSTANCE:
            console.printf("ZRAW decoder: Could not create ZRAW decoding context!\n");
            return false;

        case ZRAW_DECODER_STATE__STANDBY:
            console.printf("ZRAW decoder: STANDBY\n");
            return true;

        case ZRAW_DECODER_STATE__NO_SPACE_TO_WRITE_CFA:
            console.printf("ZRAW decoder: NO_SPACE_TO_WRITE_CFA\n");
            return false;

        case ZRAW_DECODER_STATE__FRAME_IS_READ:
            console.printf("ZRAW decoder: FRAME_IS_READ\n");
            return true;

        case ZRAW_DECODER_STATE__FRAME_READING_FAILED:
            console.printf("ZRAW decoder: FRAME_READING_FAILED\n");
            return false;

        case ZRAW_DECODER_STATE__FRAME_IS_DECOMPRESSED:
            console.printf("ZRAW decoder: FRAME_IS_DECOMPRESSED\n");
            return true;

        case ZRAW_DECODER_STATE__FRAME_DECOMPRESSION_FAILED:
            console.printf("ZRAW decoder: FRAME_DECOMPRESSION_FAILED\n");
            return false;

        case ZRAW_DECODER_STATE__INSTANCE_IS_REMOVED:
            console.printf("ZRAW decoder: INSTANCE_IS_REMOVED\n");
            return true;

        case ZRAW_DECODER_STATE__EXCEPTION:
            console.printf("ZRAW decoder: EXCEPTION -> %s\n", zraw_decoder__exception_message());
            break;

        case ZRAW_DECODER_STATE__UNEXPECTED_FAILURE:
        default:
            console.printf("ZRAW decoder: UNEXPECTED_FAILURE\n");
            return false;
        }

        console.printf("ZRAW decoder: WARNING! Missing switch-case!\n");
        return false;
    }

    void process_dng(IConsoleOutput &console, IProgressBar& progressBar, std::vector<uint8_t>& buffer, std::string outputRawFilePath)
    {
        // Create ZRAW decoder
        auto decoder = zraw_decoder__create();
        if (decoder == nullptr)
        {
            console.printf("Error! Could not create ZRAW decoding context!\n");
            return;
        }

        // Read frame from buffer
        progressBar.SetDescription("Fetching ZRAW frame info - %s", outputRawFilePath.c_str());
        auto reading_state = zraw_decoder__read_hisi_frame(decoder, buffer.data(), buffer.size());
        if (!process_zraw_decoder_state(console, reading_state))
        {
            console.printf("Frame reading failed!\n");
            return;
        }

        zraw_frame_info_t frame_info;
        auto info_state = zraw_decoder__get_hisi_frame_info(decoder, frame_info);
        if (!process_zraw_decoder_state(console, info_state))
        {
            console.printf("Getting HiSilicon frame info failed!\n");
            return;
        }

        // Decode frame
        progressBar.SetDescription("Decompressing ZRAW CFA - %s", outputRawFilePath.c_str());
        auto decompression_state = zraw_decoder__decompress_hisi_frame(decoder);
        if (!process_zraw_decoder_state(console, decompression_state))
        {
            console.printf("Frame decompression failed!\n");
            return;
        }

        tinydngwriter::DNGImage dng_image;
        dng_image.SetBigEndian(false);

        dng_image.SetSubfileType(false, false, false);
        dng_image.SetImageWidth(frame_info.width_in_photodiodes);
        dng_image.SetImageLength(frame_info.height_in_photodiodes);
        dng_image.SetRowsPerStrip(frame_info.height_in_photodiodes);
        dng_image.SetSamplesPerPixel(1);

        // Bits Per Photodiode value
        uint16_t bps[1] = { (uint16_t)frame_info.bits_per_photodiode_value };
        dng_image.SetBitsPerSample(1, bps);

        dng_image.SetPlanarConfig(tinydngwriter::PLANARCONFIG_CONTIG);
        dng_image.SetCompression(tinydngwriter::COMPRESSION_NONE);
        dng_image.SetPhotometric(tinydngwriter::PHOTOMETRIC_CFA);
        dng_image.SetXResolution(300.0);
        dng_image.SetYResolution(300.0);
        dng_image.SetOrientation(tinydngwriter::ORIENTATION_TOPLEFT);
        dng_image.SetResolutionUnit(tinydngwriter::RESUNIT_NONE);
        dng_image.SetImageDescription("[Storyboard Creativity] ZRAW -> DNG converter generated image.");

        dng_image.SetUniqueCameraModel("Z CAM E2");

        // CM1
        double matrix1[] =
        {
            0.9784, -0.4995, 0.003,
            -0.3625, 1.1454, 0.2475,
            -0.0961, 0.2097, 0.6377
        };
        dng_image.SetColorMatrix1(3, matrix1);
        dng_image.SetCalibrationIlluminant1(17);

        // CM2
        double matrix2[] =
        {
            0.6770, -0.1895, -0.0744,
            -0.5232, 1.3145, 0.2303,
            -0.1664, 0.2691, 0.5703
        };
        dng_image.SetColorMatrix2(3, matrix2);
        dng_image.SetCalibrationIlluminant2(21);

        // We set analog WB as neutral
        double analog_balance[] = { 1.0, 1.0, 1.0 };
        dng_image.SetAnalogBalance(3, analog_balance);

        // We set post WB according to awb(or wb) gains
        double rgain = frame_info.awb_gain_r;
        double ggain = frame_info.awb_gain_g;
        double bgain = frame_info.awb_gain_b;
        double wbalance[3] =
        {
            (1.0 * ggain / rgain),
            (1.0),
            (1.0 * ggain / bgain)
        };
        dng_image.SetAsShotNeutral(3, wbalance);

        // Black Levels
        dng_image.SetBlackLevelRepeatDim(2, 2);
        dng_image.SetBlackLevel(4, frame_info.cfa_black_levels);

        dng_image.SetDNGVersion(1, 2, 0, 0);

        dng_image.SetCFARepeatPatternDim(2, 2);

        uint8_t cfa_pattern[4] = { 0, 1, 1, 2 };
        dng_image.SetCFAPattern(4, cfa_pattern);

        double white_levels[1] = { (double)((1 << frame_info.bits_per_photodiode_value) - 1) };
        dng_image.SetWhiteLevelRational(1, white_levels);

        // Get CFA data
        progressBar.SetDescription("Saving DNG file - %s", outputRawFilePath.c_str());
        std::vector<uint16_t> image_data(frame_info.width_in_photodiodes * frame_info.height_in_photodiodes);
        auto cfa_state = zraw_decoder__get_decompressed_CFA(decoder, image_data.data(), image_data.size() * sizeof(uint16_t));
        if (!process_zraw_decoder_state(console, cfa_state))
        {
            console.printf("Receiving CFA failed!\n");
            return;
        }

        // Release ZRAW decoder
        auto free_state = zraw_decoder__free(decoder);
        if (!process_zraw_decoder_state(console, free_state))
        {
            console.printf("Failed to remove ZRAW decoding context!\n");
            return;
        }

        dng_image.SetImageDataPacked(image_data.data(), image_data.size(), frame_info.bits_per_photodiode_value, true);

        tinydngwriter::DNGWriter dng_writer(false);
        dng_writer.AddImage(&dng_image);

        std::string err;
        dng_writer.WriteToFile(outputRawFilePath.c_str(), &err);
    }

    ConversionResult process_zraw_old_raw(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path)
    {
        for (int i = 0; i < track.frames.size(); ++i)
        {
            if (itsTimeToStopOkay)
                return Interrupted;

            progressBar.ChangePercent(i * (100.0 / track.frames.size()));

            // Read ZRAW frame data from .ZRAW (.MOV) file
            console.printf("Reading frame #%d...\n", i);
            progressBar.SetDescription("Reading frame #%d...", i);
            auto frame_data = zrawFrame(f_in, track, i);

            // Convert extracted ZRAW frame to DNG
            console.printf("Converting zraw frame #%d to DNG...\n", i);
            progressBar.SetDescription("Converting zraw frame #%d to DNG...", i);
            std::string output_dng_path = output_path + "/track_" + std::to_string(track_index) + "_" + std::to_string(i) + ".dng";
            process_dng(console, progressBar, frame_data, output_dng_path);
        }

        return Done;
    }

    ConversionResult process_zraw_new_raw(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path, uint32_t xor_value)
    {
        switch (track.zraw_unk1)
        {
        case 1:
            return process_zraw_XORred(itsTimeToStopOkay, console, progressBar, f_in, track, track_index, output_path, xor_value);

        case 0:
            return process_zraw_AESed(itsTimeToStopOkay, console, progressBar, f_in, track, track_index, output_path);
        }

        console.printf("Can't decrypt unknown ZRAW version!\n");
        return NotImplemented;
    }

    ConversionResult process_zraw_AESed(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path)
    {
        console.printf("Can't decode AESed version!\n");
        return NotImplemented;
    }

    ConversionResult process_zraw_XORred(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::istream &f_in, TrackInfo_t &track, uint32_t track_index, std::string &output_path, uint32_t xor_value)
    {
        // Prepare output file path
        std::string file_path = output_path + "/track_" + std::to_string(track_index) + ".avc";
        console.printf("Track output file path: %s\n", file_path.c_str());

        // Open output file
        console.printf("Opening output file...\n");
        progressBar.SetDescription("Opening output file...");
        std::fstream f_out(file_path, std::ios::out | std::ios::binary);
        if (!f_out.is_open())
        {
            console.printf("Failed to open output file!\n");
            return OutputFileOpenFailed;
        }

        // Process each frame in track
        console.printf("Processing frames in track...\n");
        progressBar.SetDescription("Processing frames in track...");
        for (int p = 0; p < track.frames.size(); ++p)
        {
            if (itsTimeToStopOkay)
            {
                f_out.close();
                return Interrupted;
            }

            progressBar.ChangePercent(p * (100.0 / track.frames.size()));

            auto size = track.universal_sample_size == -1 ? track.frames[p].frame_size : track.universal_sample_size;

            console.printf("Frame %d... (size = %d)\n", p, size);
            progressBar.SetDescription("Processing frame %d...", p, size);

            auto data = new uint8_t[size];

            f_in.seekg(track.frames[p].frame_offset);
            f_in.read((char *)data, size);

            uint8_t *decrypted_data = new uint8_t[size];

            // DeXOR frame data
            uint32_t decrypted_size = decode_zraw_block_unk(data, decrypted_data, size, xor_value);

            f_out.write((char *)decrypted_data, decrypted_size);

            delete[] data;
            delete[] decrypted_data;
        }

        // Close output file
        console.printf("Closing output file...\n");
        progressBar.SetDescription("Closing output file...");
        f_out.close();

        return Done;
    }

    template<class T>
    T remove_extension(T const & filename)
    {
        typename T::size_type const p(filename.find_last_of('.'));
        return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
    }

    std::string base_name(std::string const & path)
    {
        return path.substr(path.find_last_of("/\\") + 1);
    }
};