#pragma once

#include <string>
#include <fstream>

#include <MovAvInfoDetect.hpp>

#ifndef _MSC_VER
#include <sys/stat.h>
#endif

class ZrawProcessingModel
{
public:
    DECLARE_EVENT(void, bool, std::string) EventValidityUpdate;

public:
    ZrawProcessingModel() : _isValid(false)
    {
        _updateValidityInfo();
    }

    void InputFilePath_set(std::string path)
    {
        _inputFilePath = path;
        _updateValidityInfo();
    }

    std::string InputFilePath_get()
    {
        return _inputFilePath;
    }

    void OutputFolderPath_set(std::string path)
    {
        _outputFolderPath = path;
        _updateValidityInfo();
    }

    std::string OutputFolderPath_get()
    {
        return _outputFolderPath;
    }

    // Returns if model is ready to start conversion process
    bool IsValid()
    {
        return _isValid;
    }

    // Returns info about validity (for example, the reason of IsValid() == false)
    std::string ValidityDescriprion()
    {
        return _validityDescriprion;
    }

    struct ValidInfo
    {
        std::string InputPath;
        std::string OutputPath;

        TracksInfo_t TracksInfo;
    };
    std::unique_ptr<ValidInfo> ValidInfo_get()
    {
        if (!IsValid())
            return nullptr;

        auto res = std::make_unique<ValidInfo>();
        res->InputPath = InputFilePath_get();
        res->OutputPath = OutputFolderPath_get();
        res->TracksInfo = _tracksInfo;

        return res;
    }

protected:

    void _updateValidityInfo()
    {
        _updateValidityInfoInternal();
        TRIGGER_EVENT(EventValidityUpdate, IsValid(), ValidityDescriprion());
    }

    void _updateValidityInfoInternal()
    {
        // Check input path (if it's empty)
        if (InputFilePath_get().empty())
        {
            _isValid = false;
            _validityDescriprion = "Input file is not set.";
            return;
        }

        // Check output path (if it's empty)
        if (OutputFolderPath_get().empty())
        {
            _isValid = false;
            _validityDescriprion = "Output path is not set.";
            return;
        }

        // Check input path
        if (_pathExists(InputFilePath_get()) != 2)
        {
            _isValid = false;
            _validityDescriprion = "Could not open input file.";
            return;
        }

        // Check output path
        if (_pathExists(OutputFolderPath_get()) != 1)
        {
            _isValid = false;
            _validityDescriprion = "Output path is invalid.";
            return;
        }

        // Read input file
        auto info = MovDetectTracks(InputFilePath_get().c_str());
        _tracksInfo = info;

        // Find ZRAW tracks
        int zrawTrackIndex = -1;
        for (int i = 0; i < info.tracks.size(); ++i)
        {
            if (info.tracks[i].codec_name == "zraw")
            {
                if (zrawTrackIndex != -1)
                {
                    _isValid = false;
                    _validityDescriprion = "Input file contains more than 1 ZRAW track!";
                    return;
                }

                zrawTrackIndex = i;
            }
        }

        // Exit if file does not have any ZRAW tracks
        if (zrawTrackIndex == -1)
        {
            _isValid = false;
            _validityDescriprion = "Input file does not contain any ZRAW tracks!";
            return;
        }

        // Set validity to "true" (all is ok if we are here)
        _isValid = true;
        _validityDescriprion = "Ready to process conversion!";
    }

    int _pathExists(const std::string& path)
    {
        struct stat info;

        if (stat(path.c_str(), &info) != 0)
            return -1;    // Can't get access

        if (info.st_mode & S_IFDIR)
            return 1;    // Path points to a directory

        return 2;    // Path points to a file
    }

    std::string _inputFilePath;
    std::string _outputFolderPath;

    bool _isValid;
    std::string _validityDescriprion;

    TracksInfo_t _tracksInfo;
};
