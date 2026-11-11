#pragma once
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>

#include "IConsoleOutput.hpp"
#include <tiny_dng_writer.h>
#include "lj92.hpp"

#define LIB_ZRAW_STATIC
#include <libzraw.h>

class ZrawConverterThread
{
public:
    struct Parameters
    {
        typedef enum CameraModel_e
        {
            Unknown,
            E2,
            E2C,
            E2G,
            E2_M4,
            E2_S6,
            E2_F6,
            E2_F8,
            E2_S6G
        } CameraModel_t;

        CameraModel_t camera_model;
        ZrawProcessingModel::RawCompression_t compression;
        ZrawProcessingModel::RawScale_t scale;
        std::string dng_path;
        std::vector<uint8_t> frameData;
        
    };

    // Bounded queue of frames waiting for a worker thread
    class Queue
    {
    public:
        struct Task
        {
            IConsoleOutput* console;
            Parameters parameters;
        };

        explicit Queue(size_t max_waiting) : _max_waiting(max_waiting), _busy(0), _closed(false) {}

        void Push(Task task)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _has_room.wait(lock, [this] { return _tasks.size() < _max_waiting; });
            _tasks.push(std::move(task));
            _has_task.notify_one();
        }

        // Returns false when the queue is closed and empty
        bool Pop(Task& task)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _has_task.wait(lock, [this] { return !_tasks.empty() || _closed; });
            if (_tasks.empty())
                return false;

            task = std::move(_tasks.front());
            _tasks.pop();
            ++_busy;
            _has_room.notify_one();
            return true;
        }

        void TaskDone()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            --_busy;
            if (_busy == 0 && _tasks.empty())
                _is_idle.notify_all();
        }

        void WaitUntilIdle()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _is_idle.wait(lock, [this] { return _busy == 0 && _tasks.empty(); });
        }

        void Close()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _closed = true;
            _has_task.notify_all();
        }

    private:
        std::mutex _mutex;
        std::condition_variable _has_task;
        std::condition_variable _has_room;
        std::condition_variable _is_idle;

        std::queue<Task> _tasks;
        size_t _max_waiting;
        size_t _busy;
        bool _closed;
    };

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

    // RAW -> 1/2 RAW
    static std::vector<uint16_t> downscale_raw_twice(std::vector<uint16_t>& image, uint32_t width, uint32_t height)
    {
        std::vector<uint16_t> result(width * height / 4);

        for (int y = 0; y < height; y += 4)
        {
            for (int x = 0; x < width; x += 4)
            {
                result[(y / 2 + 0) * (width / 2) + (x / 2 + 0)] = ((uint32_t)image[(y + 0) * width + x + 0] + (uint32_t)image[(y + 0) * width + x + 2] + (uint32_t)image[(y + 2) * width + x + 0] + (uint32_t)image[(y + 2) * width + x + 2]) / 4;
                result[(y / 2 + 0) * (width / 2) + (x / 2 + 1)] = ((uint32_t)image[(y + 0) * width + x + 1] + (uint32_t)image[(y + 0) * width + x + 3] + (uint32_t)image[(y + 2) * width + x + 1] + (uint32_t)image[(y + 2) * width + x + 3]) / 4;
                result[(y / 2 + 1) * (width / 2) + (x / 2 + 0)] = ((uint32_t)image[(y + 1) * width + x + 0] + (uint32_t)image[(y + 1) * width + x + 2] + (uint32_t)image[(y + 3) * width + x + 0] + (uint32_t)image[(y + 3) * width + x + 2]) / 4;
                result[(y / 2 + 1) * (width / 2) + (x / 2 + 1)] = ((uint32_t)image[(y + 1) * width + x + 1] + (uint32_t)image[(y + 1) * width + x + 3] + (uint32_t)image[(y + 3) * width + x + 1] + (uint32_t)image[(y + 3) * width + x + 3]) / 4;
            }
        }

        return result;
    }

    // Packs samples MSB-first at the given bit depth (TIFF layout). Returns the number of bytes written.
    static size_t pack_samples(const uint16_t* in, size_t count, int bits, uint8_t* out)
    {
        uint8_t* w = out;
        uint64_t acc = 0;
        int bits_in_acc = 0;
        for (size_t i = 0; i < count; ++i)
        {
            acc = (acc << bits) | in[i];
            bits_in_acc += bits;
            while (bits_in_acc >= 8)
            {
                bits_in_acc -= 8;
                *w++ = (uint8_t)(acc >> bits_in_acc);
            }
        }

        if (bits_in_acc)
            *w++ = (uint8_t)(acc << (8 - bits_in_acc));

        return (size_t)(w - out);
    }

    // Writes the same file as DNGWriter::WriteToFile, byte for byte, without copying the image data.
    // The layout is built with a 1-byte image. It is last in the data area, so only the IFD offset
    // in the header and the StripByteCounts entry depend on its size.
    static bool write_dng(const std::string& path, tinydngwriter::DNGImage& dng_image, const uint8_t* data, size_t data_size, std::string& err)
    {
        const unsigned char stand_in = 0;
        dng_image.SetImageData(&stand_in, 1);

        std::ostringstream data_os, ifd_os;
        if (!dng_image.WriteDataToStream(&data_os) || !dng_image.WriteIFDToStream(0, (unsigned)dng_image.GetStripOffset(), &ifd_os))
        {
            err = dng_image.Error();
            return false;
        }

        std::string tags_data = data_os.str(), ifd = ifd_os.str();
        const size_t strip_offset = dng_image.GetStripOffset();
        if (strip_offset + 1 != tags_data.size())
        {
            err = "unexpected DNG layout";
            return false;
        }

        // IFD is a 2-byte entry count followed by 12-byte entries. StripByteCounts (tag 279) stores its value inline.
        uint16_t entries;
        memcpy(&entries, ifd.data(), 2);
        bool patched = false;
        for (size_t e = 0; e < entries; ++e)
        {
            char* entry = &ifd[2 + 12 * e];
            uint16_t tag;
            memcpy(&tag, entry, 2);
            if (tag == 279)
            {
                uint32_t value = (uint32_t)data_size;
                memcpy(entry + 8, &value, 4);
                patched = true;
            }
        }

        if (!patched)
        {
            err = "StripByteCounts is missing";
            return false;
        }

        uint8_t header[8] = { 0x49, 0x49, 0x2A, 0x00 };  // TIFF, little-endian
        uint32_t ifd_offset = (uint32_t)(8 + strip_offset + data_size);
        memcpy(header + 4, &ifd_offset, 4);
        const uint32_t next_ifd_offset = 0;

        std::ofstream f_out(path, std::ios::out | std::ios::binary);
        f_out.write((const char*)header, 8);
        f_out.write(tags_data.data(), strip_offset);
        f_out.write((const char*)data, data_size);
        f_out.write(ifd.data(), ifd.size());
        f_out.write((const char*)&next_ifd_offset, 4);
        f_out.close();

        if (!f_out)
        {
            err = "write failed";
            return false;
        }

        return true;
    }

    static bool process_dng(IConsoleOutput& console, Parameters& parameters)
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
        auto reading_state = zraw_decoder__read_hisi_frame(decoder, parameters.frameData.data(), parameters.frameData.size());
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

        switch (parameters.scale)
        {
        case ZrawProcessingModel::RawScale_t::Full:
            dng_image.SetImageWidth(frame_info.width_in_photodiodes);
            dng_image.SetImageLength(frame_info.height_in_photodiodes);
            dng_image.SetRowsPerStrip(frame_info.height_in_photodiodes);
            break;

        case ZrawProcessingModel::RawScale_t::Half:
            dng_image.SetImageWidth(frame_info.width_in_photodiodes / 2);
            dng_image.SetImageLength(frame_info.height_in_photodiodes / 2);
            dng_image.SetRowsPerStrip(frame_info.height_in_photodiodes / 2);
            break;

        case ZrawProcessingModel::RawScale_t::Quarter:
            dng_image.SetImageWidth(frame_info.width_in_photodiodes / 4);
            dng_image.SetImageLength(frame_info.height_in_photodiodes / 4);
            dng_image.SetRowsPerStrip(frame_info.height_in_photodiodes / 4);
            break;

        default:
            console.printf("Unknown RAW scale mode!\n");
            return false;
        }

        dng_image.SetSamplesPerPixel(1);

        // Bits Per Photodiode value
        uint16_t bps[1] = { (uint16_t)frame_info.bits_per_photodiode_value };
        dng_image.SetBitsPerSample(1, bps);

        dng_image.SetPlanarConfig(tinydngwriter::PLANARCONFIG_CONTIG);

        switch (parameters.compression)
        {
        case ZrawProcessingModel::RawCompression_t::None:
            dng_image.SetCompression(tinydngwriter::COMPRESSION_NONE);
            break;

        case ZrawProcessingModel::RawCompression_t::LosslessJPEG:
            dng_image.SetCompression(tinydngwriter::COMPRESSION_NEW_JPEG);
            break;

        default:
            console.printf("Unknown RAW compression mode!\n");
            return false;
        }
        
        dng_image.SetPhotometric(tinydngwriter::PHOTOMETRIC_CFA);
        dng_image.SetXResolution(300.0);
        dng_image.SetYResolution(300.0);
        dng_image.SetOrientation(tinydngwriter::ORIENTATION_TOPLEFT);
        dng_image.SetResolutionUnit(tinydngwriter::RESUNIT_NONE);
        dng_image.SetImageDescription("[Storyboard Creativity] ZRAW -> DNG converter generated image.");

        // Camera Model
        switch (parameters.camera_model)
        {
        case Parameters::CameraModel_t::E2:
            dng_image.SetUniqueCameraModel("Z CAM E2");
            break;

        case Parameters::CameraModel_t::E2C:
            dng_image.SetUniqueCameraModel("Z CAM E2C");
            break;

        case Parameters::CameraModel_t::E2G:
            dng_image.SetUniqueCameraModel("Z CAM E2G");
            break;

        case Parameters::CameraModel_t::E2_M4:
            dng_image.SetUniqueCameraModel("Z CAM E2-M4");
            break;

        case Parameters::CameraModel_t::E2_S6:
            dng_image.SetUniqueCameraModel("Z CAM E2-S6");
            break;

        case Parameters::CameraModel_t::E2_F6:
            dng_image.SetUniqueCameraModel("Z CAM E2-F6");
            break;

        case Parameters::CameraModel_t::E2_F8:
            dng_image.SetUniqueCameraModel("Z CAM E2-F8");
            break;

        case Parameters::CameraModel_t::E2_S6G:
            dng_image.SetUniqueCameraModel("Z CAM E2-S6G");
            break;

        case Parameters::CameraModel_t::Unknown:
        default:
            dng_image.SetUniqueCameraModel("Unknown camera with ZRAW codec");
            break;
        }

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
        // Reused between frames to avoid allocating a large buffer every time
        thread_local std::vector<uint16_t> image_data;
        image_data.resize(frame_info.width_in_photodiodes * frame_info.height_in_photodiodes);
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

        uint32_t w = frame_info.width_in_photodiodes;
        uint32_t h = frame_info.height_in_photodiodes;
        switch (parameters.scale)
        {
        case ZrawProcessingModel::RawScale_t::Full:
            break;

        case ZrawProcessingModel::RawScale_t::Half:

            // Make 1/2
            image_data = downscale_raw_twice(image_data, w, h);
            w /= 2;
            h /= 2;

            break;

        case ZrawProcessingModel::RawScale_t::Quarter:

            // Make 1/2
            image_data = downscale_raw_twice(image_data, w, h);
            w /= 2;
            h /= 2;

            // Make 1/4
            image_data = downscale_raw_twice(image_data, w, h);
            w /= 2;
            h /= 2;

            break;

        default:
            console.printf("Unknown RAW scale mode!\n");
            return false;
        }

        std::string err;
        bool written = false;
        switch (parameters.compression)
        {
        case ZrawProcessingModel::RawCompression_t::None:
        {
            // Same bytes as SetImageDataPacked: packed samples, zero padded to 16 bits per sample
            thread_local std::vector<uint8_t> packed;
            packed.resize(image_data.size() * sizeof(uint16_t));
            size_t packed_size = pack_samples(image_data.data(), image_data.size(), frame_info.bits_per_photodiode_value, packed.data());
            memset(packed.data() + packed_size, 0, packed.size() - packed_size);

            written = write_dng(parameters.dng_path, dng_image, packed.data(), packed.size(), err);
            break;
        }

        case ZrawProcessingModel::RawCompression_t::LosslessJPEG:
        {
            // Like SetImageDataJpeg, each pair of CFA rows is encoded as one row twice as wide
            const uint8_t* jpeg;
            size_t jpeg_size;
            if (h % 2 == 0 && Lj92Encoder::Encode(image_data.data(), w * 2, h / 2, frame_info.bits_per_photodiode_value, jpeg, jpeg_size))
            {
                written = write_dng(parameters.dng_path, dng_image, jpeg, jpeg_size, err);
            }
            else
            {
                // Anything Lj92Encoder does not handle goes through tinydngwriter
                dng_image.SetImageDataJpeg(image_data.data(), w, h, frame_info.bits_per_photodiode_value);

                tinydngwriter::DNGWriter dng_writer(false);
                dng_writer.AddImage(&dng_image);
                written = dng_writer.WriteToFile(parameters.dng_path.c_str(), &err);
            }
            break;
        }

        default:
            return false;
        }

        if (!written)
        {
            console.printf("Could not write DNG file %s: %s\n", parameters.dng_path.c_str(), err.c_str());
            return false;
        }

        return true;
    }

    void _thread_func()
    {
        while (true)
        {
            Queue::Task task;
            if (!_queue.Pop(task))
                break;

            task.console->printf("Received DNG frame task!\n");
            process_dng(*task.console, task.parameters);

            _queue.TaskDone();
        }
    }

    Queue& _queue;

    std::thread _thread;

public:
    ZrawConverterThread(Queue& queue) : _queue(queue), _thread(&ZrawConverterThread::_thread_func, this) {}

    // The queue must be closed first, otherwise join() never returns
    ~ZrawConverterThread()
    {
        _thread.join();
    }
};
