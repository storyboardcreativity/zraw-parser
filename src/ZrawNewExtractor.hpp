#pragma once

#include <fstream>
#include <string>
#include <strstream>
#include <atomic>

#include <MovAvInfoDetect.hpp>
#include <IConsoleOutput.hpp>
#include <IProgressBar.hpp>

#include <ZrawConverterThread.hpp>

#define TINY_DNG_WRITER_IMPLEMENTATION
#include <tiny_dng_writer.h>

// for windows mkdir
#ifdef _WIN32
#include <direct.h>
#endif

class ZrawNewExtractor
{
protected:
    std::vector<ZrawConverterThread*> _threads;

public:
    ZrawNewExtractor()
    {
        for (auto i = 0; i < std::thread::hardware_concurrency(); ++i)
            _threads.push_back(new ZrawConverterThread());
    }

    ~ZrawNewExtractor()
    {
        for (int i = 0; i < _threads.size(); ++i)
        {
            delete _threads[i];
        }
    }

    enum ConversionResult
    {
        Done,
        InputFileOpenFailed,
        OutputFileOpenFailed,
        DirectoryCreationFailed,
        Interrupted,
        NotImplemented
    };

    ConversionResult ProcessConversion(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::string zraw_file_path, std::string output_path, ZrawProcessingModel::RawCompression_t compression)
    {
        console.printf("Converting file %s\n", zraw_file_path.c_str());

        std::string fileName = remove_extension(base_name(zraw_file_path));
        auto dirPath = output_path + "/" + fileName;
        console.printf("Output folder: %s\n", dirPath.c_str());

#ifdef _WIN32
        if (_mkdir(dirPath.c_str()) != 0)
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

        //auto tracks_info = MovDetectTracks(zraw_file_path.c_str());
        //console.printf(tracks_info.output_log.c_str());

        TinyMovFileReader reader;
        auto mov = reader.OpenMovFile(zraw_file_path.c_str());

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

        TinyMovTrackMediaVideoDescription::Extension_ZRAW_t ext_zraw;
        ext_zraw.version = 0;
        ext_zraw.exists = false;
        for (int i = 0; i < mov.Tracks().size(); ++i)
        {
            if (itsTimeToStopOkay)
            {
                f_in.close();
                return Interrupted;
            }

            auto& track = mov.Tracks()[i];
            if (track.Media().Type() != TinyMovTrackMedia::Type_t::Video)
            {
                console.printf("Track #%d - not a video, skipped.\n", i);
                continue;
            }

            auto& desc_table = track.Media().Info().DescriptionTable().VideoDescriptionTable();
            if (desc_table.size() != 1)
            {
                console.printf("Track #%d - wrong description table, skipped.\n", i);
                continue;
            }

            auto& desc = desc_table[0];
            if (desc.DataFormat() != MKTAG('z', 'r', 'a', 'w'))
            {
                console.printf("Track #%d - not ZRAW, skipped.\n", i);
                continue;
            }

            console.printf("Track #%d\n", i);

            ext_zraw = desc.Ext_ZRAW();
            if (!ext_zraw.exists)
            {
                console.printf("Track #%d - error! Track does not contain 'zraw' extension with version information! Skipped.\n", i);
                continue;
            }

            console.printf("ZRAW version = 0x%X (%s)\n", ext_zraw.version,
                ext_zraw.version == 0x12EA78D2 ? "TRUE RAW" :
                (ext_zraw.version == 0x45A32DEF ? "ENCRYPTED HEVC" : "UNKNOWN"));

            console.printf("ZRAW unk0 = %d\n", ext_zraw.unk0);
            console.printf("ZRAW unk1 = %d\n", ext_zraw.unk1);

            break;
        }

        ConversionResult subRes = Done;
        switch (ext_zraw.version)
        {
        case 0x12EA78D2:
            subRes = process_zraw_old_raw(itsTimeToStopOkay, console, progressBar, f_in, mov, dirPath, compression);
            break;
        case 0x45A32DEF:
            subRes = process_zraw_new_raw(itsTimeToStopOkay, console, progressBar, f_in, mov, dirPath, ext_zraw);
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

        // Close input file
        console.printf("Closing input file...\n");
        progressBar.SetDescription("Closing input file...");
        f_in.close();

        return ConversionResult::Done;
    }

protected:
    std::vector<uint8_t> zrawFrame(std::istream &f_in, TinyMovTrack &track, uint32_t frame_index)
    {
        // Extract saved offset and size
        const auto offset = track.Media().Info().ChunkOffsets()[frame_index];
        const auto size = track.Media().Info().SampleSizes()[frame_index];

        // Seek read position to zraw frame offset
        f_in.seekg(offset, f_in.beg);

        // Read frame
        std::vector<uint8_t> frame(size);
        f_in.read((char *)frame.data(), size);

        return frame;
    }

    void wait_for_dng_threads_to_finish(IConsoleOutput &console, IProgressBar& progressBar)
    {
        console.printf("Waiting for DNG processing threads to finish their tasks...\n");
        progressBar.SetDescription("Waiting for DNG processing threads to finish their tasks...");

        bool all_finished = false;
        while (!all_finished)
        {
            for (int th_index = 0; th_index < _threads.size(); ++th_index)
            {
                if (_threads[th_index]->IsProcessingFrame())
                    break;

                if (th_index == _threads.size() - 1)
                    all_finished = true;
            }
        }
    }

    ConversionResult process_zraw_old_raw(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::istream &f_in, TinyMovFile &mov, std::string &output_path, ZrawProcessingModel::RawCompression_t compression)
    {
        for (int i = 0; i < mov.Tracks().size(); ++i)
        {
            auto& track = mov.Tracks()[i];
            if (track.Media().Type() != TinyMovTrackMedia::Type_t::Video)
                continue;

            auto& desc_table = track.Media().Info().DescriptionTable().VideoDescriptionTable();
            if (desc_table.size() != 1)
                continue;

            auto& desc = desc_table[0];
            if (desc.DataFormat() != MKTAG('z', 'r', 'a', 'w'))
                continue;

            auto ext_zraw = desc.Ext_ZRAW();
            if (!ext_zraw.exists)
                continue;

            if (ext_zraw.version != 0x12EA78D2)
                continue;

            for (int p = 0; p < track.Media().Info().SampleSizes().size(); ++p)
            {
                if (itsTimeToStopOkay)
                {
                    wait_for_dng_threads_to_finish(console, progressBar);

                    return Interrupted;
                }

                progressBar.ChangePercent(p * (100.0 / track.Media().Info().SampleSizes().size()));

                // Read ZRAW frame data from .ZRAW (.MOV) file
                console.printf("Reading frame #%d...\n", p);
                progressBar.SetDescription("Reading frame #%d...", p);
                auto frame_data = zrawFrame(f_in, track, p);

                std::string output_dng_path = output_path + "/track_" + std::to_string(i) + "_" + std::to_string(p) + ".dng";

                //#define _ZRAWF_EXPORT
#ifdef _ZRAWF_EXPORT

                std::string output_zrawf_path = output_path + "/track_" + std::to_string(i) + "_" + std::to_string(p) + ".zrawf";
                std::fstream frame_out(output_zrawf_path, std::ios::out | std::ios::binary);
                frame_out.write((char*)frame_data.data(), frame_data.size());
                frame_out.close();

#endif

                bool th_found = false;
                while (!th_found)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    for (int th_index = 0; th_index < _threads.size(); ++th_index)
                    {
                        if (_threads[th_index]->IsProcessingFrame() == false)
                        {
                            // Convert extracted ZRAW frame to DNG
                            console.printf("Converting zraw frame #%d to DNG...\n", p);
                            progressBar.SetDescription("Converting zraw frame #%d to DNG...", p);

                            _threads[th_index]->SetProcessingFrame(console, frame_data, output_dng_path, compression);

                            th_found = true;
                            break;
                        }
                    }
                }
            }
        }

        wait_for_dng_threads_to_finish(console, progressBar);

        std::vector<TinyMovTrack> tracks;
        for (int i = 0; i < mov.Tracks().size(); ++i)
        {
            auto& track = mov.Tracks()[i];
            if (track.Media().Type() != TinyMovTrackMedia::Type_t::Video)
            {
                tracks.push_back(track);
                continue;
            }

            auto& desc_table = track.Media().Info().DescriptionTable().VideoDescriptionTable();
            if (desc_table.size() != 1)
            {
                tracks.push_back(track);
                continue;
            }

            auto& desc = desc_table[0];
            if (desc.DataFormat() != MKTAG('z', 'r', 'a', 'w'))
            {
                tracks.push_back(track);
                continue;
            }
        }
        mov.Tracks().clear();
        for (auto it = tracks.begin(); it != tracks.end(); ++it)
            mov.Tracks().push_back(*it);

        TinyMovFileWriter writer;
        std::string fileName = remove_extension(base_name(mov.Path()));
        std::string output_mov_path = output_path + "/" + fileName + ".mov";
        writer.SaveMovFile(mov, output_mov_path);

        return Done;
    }

    ConversionResult process_zraw_new_raw(
        std::atomic<bool>& itsTimeToStopOkay,
        IConsoleOutput &console,
        IProgressBar& progressBar,
        std::istream &f_in,
        TinyMovFile &mov,
        std::string &output_path,
        TinyMovTrackMediaVideoDescription::Extension_ZRAW_t& zraw_info)
    {
        switch (zraw_info.unk1)
        {
        case 1:
            return process_zraw_XORred(itsTimeToStopOkay, console, progressBar, f_in, mov, output_path);

        case 0:
            return process_zraw_AESed(itsTimeToStopOkay, console, progressBar, f_in, mov, output_path);
        }

        console.printf("Can't decrypt unknown ZRAW version!\n");
        return NotImplemented;
    }

    ConversionResult process_zraw_AESed(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::istream &f_in, TinyMovFile &mov, std::string &output_path)
    {
        console.printf("Can't decode AESed version!\n");
        return NotImplemented;
    }

    ConversionResult process_zraw_XORred(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput &console, IProgressBar& progressBar, std::istream &f_in, TinyMovFile &mov, std::string &output_path)
    {
        // Prepare output file path
        std::string fileName = remove_extension(base_name(mov.Path()));
        std::string file_path = output_path + "/" + fileName + ".mov";
        console.printf("Track output file path: %s\n", file_path.c_str());

        TinyMovFileWriter writer;

        // Handler for video sample description
        writer.VideoDescriptionHandler([](TinyMovTrackMediaVideoDescription video_description)
        {
            if (video_description.DataFormat() != MKTAG('z', 'r', 'a', 'w'))
                return video_description;

            auto desc = video_description;

            desc.DataFormat(MKTAG('h', 'e', 'v', '1'));
            desc.CompressorName("IMVT HEVC");

            desc.Extensions()[0].tag = MKTAG('h', 'v', 'c', 'C');
            std::vector<uint8_t> data(
                {
                    0x01, 0x02, 0x60, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7B, 0xF0,
                    0x00, 0xFC, 0xFD, 0xFA, 0xFA, 0x00, 0x00, 0x0F, 0x03, 0x20, 0x00, 0x01, 0x00, 0x17, 0x40, 0x01,
                    0x0C, 0x01, 0xFF, 0xFF, 0x02, 0x60, 0x00, 0x00, 0x03, 0x00, 0xB0, 0x00, 0x00, 0x03, 0x00, 0x00,
                    0x03, 0x00, 0x7B, 0xBC, 0x09, 0x21, 0x00, 0x01, 0x00, 0x23, 0x42, 0x01, 0x01, 0x02, 0x60, 0x00,
                    0x00, 0x03, 0x00, 0xB0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x7B, 0xA0, 0x03, 0xC0, 0x80,
                    0x10, 0xE4, 0xD8, 0xDB, 0xE4, 0x91, 0x4B, 0xD3, 0x50, 0x10, 0x10, 0x10, 0x08, 0x22, 0x00, 0x01,
                    0x00, 0x0A, 0x44, 0x01, 0xC0, 0xF2, 0xC6, 0x8D, 0x03, 0xB3, 0x40, 0x00
                });
            desc.Extensions()[0].data = data;

            return desc;
        });

        // Handler for video chunk data
        uint32_t chunk_index = 0;
        uint32_t percent = 0;
        writer.ChunkHandler([&](TinyMovTrack& track, uint64_t chunk_offset, std::vector<uint8_t>& chunk_data)
        {
            if (track.Media().Type() != TinyMovTrackMedia::Type_t::Video)
                return chunk_data;

            if (track.Media().Info().DescriptionTable().VideoDescriptionTable().size() != 1)
                return chunk_data;

            auto& desc = track.Media().Info().DescriptionTable().VideoDescriptionTable()[0];
            if (desc.DataFormat() != MKTAG('z', 'r', 'a', 'w'))
                return chunk_data;

            auto ext = desc.Ext_ZRAW();
            if (!ext.exists)
                return chunk_data;

            if (ext.unk1 != 1)
                return chunk_data;

            uint32_t percent_new = chunk_index * (100.0 / track.Media().Info().ChunkOffsets().size());
            if (percent_new != percent)
                progressBar.ChangePercent(percent_new);
            ++chunk_index;

            std::vector<uint8_t> chunk_data_dst = chunk_data;

            uint32_t dst[34] = { 0 };

            uint32_t xor_value = track.Parent().CreationTime();

            uint8_t pack_count = chunk_data[0] ^ xor_value;
            printf("pack_count: 0x%02X\n", pack_count);

            uint32_t smth_size = 0;
            uint32_t result = 0;

            if ((uint8_t)(pack_count - 1) > 30)
            {
                printf("pack_count error (%d)\n", pack_count);
                return chunk_data;
            }

            if (pack_count)
            {
                uint8_t *ptr0 = &(chunk_data.data()[1]);

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
                        return chunk_data;
                    }
                }

                printf("all packs size: 0x%08X\n", smth_size);
                printf("full data size: 0x%08X\n", smth_size + 4 * pack_count + 1);

                if (smth_size + 4 * pack_count + 1 > chunk_data.size())
                {
                    printf("pack_count error\n");
                    return chunk_data;
                }

                int64_t cnt = 0;
                for (uint32_t i = xor_value; i % pack_count; ++i)
                    ++cnt;

                uint8_t *pFrameOut = new uint8_t[chunk_data.size()];
                memset(pFrameOut, 0x00, chunk_data.size());

                if (cnt)
                {
                    uint32_t v9 = 0;
                    while (cnt < pack_count)
                        v9 += dst[cnt++];

                    memcpy(pFrameOut, &(chunk_data.data()[1 + pack_count * 4 + v9]), smth_size - v9);
                    memcpy(&pFrameOut[smth_size - v9], &(chunk_data.data()[1 + pack_count * 4]), v9);
                }
                else
                {
                    memcpy(pFrameOut, &(chunk_data.data()[1 + pack_count * 4]), smth_size);
                }

                uint16_t buff[16] = { 0 };
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

                // ------------------------------------------------------------------------
                // Now we need to repack NAL units (convert '00 00 00 01' to NAL unit size)

                std::vector<uint32_t> nal_offsets;
                uint32_t sz = chunk_data.size();
                for (int g = 0; g < sz; ++g)
                    if ((*(uint32_t*)&pFrameOut[g]) == bswap_32(0x00000001))
                        nal_offsets.push_back(g);

                if (nal_offsets.size() > 0)
                {
                    for (int g = 0; g < nal_offsets.size() - 1; ++g)
                        *(uint32_t*)&pFrameOut[nal_offsets[g]] = bswap_32(nal_offsets[g + 1] - nal_offsets[g] - 4);
                    *(uint32_t*)&pFrameOut[nal_offsets[nal_offsets.size() - 1]] = bswap_32(chunk_data.size() - nal_offsets[nal_offsets.size() - 1] - 4);
                }

                // ------------------------------------------------------------------------

                memcpy(chunk_data_dst.data(), pFrameOut, chunk_data.size());

                delete[] pFrameOut;

                return chunk_data_dst;
            }

            return chunk_data;
        });

        console.printf("Processing output file...\n");
        progressBar.SetDescription("Processing output file...");

        writer.SaveMovFile(mov, file_path);

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
