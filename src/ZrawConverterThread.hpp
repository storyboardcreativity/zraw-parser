#pragma once
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#include "IConsoleOutput.hpp"
#include <tiny_dng_writer.h>

#define LIB_ZRAW_STATIC
#include <libzraw.h>

class ZrawConverterThread
{
protected:
    static bool process_zraw_decoder_state(IConsoleOutput &console, zraw_decoder_state_t state)
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

    static bool process_dng(IConsoleOutput& console, std::vector<uint8_t>& buffer, std::string outputRawFilePath, ZrawProcessingModel::RawCompression_t compression)
    {
        // Create ZRAW decoder
        auto decoder = zraw_decoder__create();
        if (decoder == nullptr)
        {
            console.printf("Error! Could not create ZRAW decoding context!\n");
            return false;
        }

        // Read frame from buffer
        //progressBar.SetDescription("Fetching ZRAW frame info - %s", outputRawFilePath.c_str());
        auto reading_state = zraw_decoder__read_hisi_frame(decoder, buffer.data(), buffer.size());
        if (!process_zraw_decoder_state(console, reading_state))
        {
            console.printf("Frame reading failed!\n");
            return false;
        }

        zraw_frame_info_t frame_info;
        auto info_state = zraw_decoder__get_hisi_frame_info(decoder, frame_info);
        if (!process_zraw_decoder_state(console, info_state))
        {
            console.printf("Getting HiSilicon frame info failed!\n");
            return false;
        }

        // Decode frame
        //progressBar.SetDescription("Decompressing ZRAW CFA - %s", outputRawFilePath.c_str());
        auto decompression_state = zraw_decoder__decompress_hisi_frame(decoder);
        if (!process_zraw_decoder_state(console, decompression_state))
        {
            console.printf("Frame decompression failed!\n");
            return false;
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

        switch (compression)
        {
        case ZrawProcessingModel::RawCompression_t::None:
            dng_image.SetCompression(tinydngwriter::COMPRESSION_NONE);
            break;

        case ZrawProcessingModel::RawCompression_t::LosslessJPEG:
            dng_image.SetCompression(tinydngwriter::COMPRESSION_NEW_JPEG);
            break;

        default:
            return false;
        }
        
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
        //progressBar.SetDescription("Saving DNG file - %s", outputRawFilePath.c_str());
        std::vector<uint16_t> image_data(frame_info.width_in_photodiodes * frame_info.height_in_photodiodes);
        auto cfa_state = zraw_decoder__get_decompressed_CFA(decoder, image_data.data(), image_data.size() * sizeof(uint16_t));
        if (!process_zraw_decoder_state(console, cfa_state))
        {
            console.printf("Receiving CFA failed!\n");
            return false;
        }

        // Release ZRAW decoder
        auto free_state = zraw_decoder__free(decoder);
        if (!process_zraw_decoder_state(console, free_state))
        {
            console.printf("Failed to remove ZRAW decoding context!\n");
            return false;
        }

        switch (compression)
        {
        case ZrawProcessingModel::RawCompression_t::None:
            dng_image.SetImageDataPacked(image_data.data(), image_data.size(), frame_info.bits_per_photodiode_value, true);
            break;

        case ZrawProcessingModel::RawCompression_t::LosslessJPEG:
            dng_image.SetImageDataJpeg(image_data.data(), frame_info.width_in_photodiodes, frame_info.height_in_photodiodes, frame_info.bits_per_photodiode_value);
            break;

        default:
            return false;
        }

        tinydngwriter::DNGWriter dng_writer(false);
        dng_writer.AddImage(&dng_image);

        std::string err;
        dng_writer.WriteToFile(outputRawFilePath.c_str(), &err);

        return true;
    }

    void _thread_func()
    {
        while (this->_runs)
        {
            if (!_finished)
            {
                if (_console == nullptr)
                {
                    _finished = true;
                    continue;
                }

                _console->printf("Received DNG frame task!\n");
                process_dng(*_console, _currentFrameData, _currentFrameOutputDngPath, _compression);

                _finished = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // FIXME: Warning! We need to initialize these bools before thread is started because it uses these fields!
    std::atomic_bool _runs;
    std::atomic_bool _finished;

    std::thread _thread;

    std::mutex _mutex;

    IConsoleOutput* _console;
    std::vector<uint8_t> _currentFrameData;
    std::string _currentFrameOutputDngPath;
    ZrawProcessingModel::RawCompression_t _compression;

public:
    ZrawConverterThread() : _runs(true), _finished(true), _thread(&ZrawConverterThread::_thread_func, this), _console(nullptr) {}

    bool SetProcessingFrame(IConsoleOutput& console, std::vector<uint8_t>& frameData, std::string currentFrameOutputDngPath, ZrawProcessingModel::RawCompression_t compression)
    {
        _mutex.lock();

        // If previous frame is still processing - exit
        if (!_finished)
        {
            _mutex.unlock();
            return false;
        }

        // Prepare parameters
        _console = &console;
        _currentFrameOutputDngPath = currentFrameOutputDngPath;
        _currentFrameData = frameData;
        _compression = compression;

        // Start processing
        _finished = false;
        
        _mutex.unlock();

        return true;
    }

    bool IsProcessingFrame()
    {
        bool res = false;
        _mutex.lock();
        res = !_finished;
        _mutex.unlock();
        return res;
    }

    ~ZrawConverterThread()
    {
        _runs = false;
        _thread.join();
    }
};
