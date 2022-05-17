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
    DECLARE_EVENT(void, std::string) EventMovContainerLogUpdate;

public:
    ZrawProcessingModel() : _isValid(false)
    {
        _updateValidityInfo();
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
        std::string OutputPath;

        TracksInfo_t TracksInfo;
    };

    std::unique_ptr<ValidInfo> ValidInfo_get()
    {
        if (!IsValid())
            return nullptr;

        auto res = std::make_unique<ValidInfo>();
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
        // Check output path (if it's empty)
        if (OutputFolderPath_get().empty())
        {
            _isValid = false;
            _validityDescriprion = "Output path is not set.";
            return;
        }

        // Check output path
        if (!_folderExists(OutputFolderPath_get()))
        {
            _isValid = false;
            _validityDescriprion = "Output path is invalid.";
            return;
        }

        // Set validity to "true" (all is ok if we are here)
        _isValid = true;
        _validityDescriprion = "Ready to process conversion!";
    }

    bool _fileExists(const std::string& path)
    {
        struct stat info;

        std::fstream f_in(path, std::ios::in | std::ios::binary);
        if (!f_in.is_open())
            return false;

        f_in.close();
        return true;
    }

    bool _folderExists(const std::string& path)
    {
        struct stat info;

        if (stat(path.c_str(), &info) != 0)
            return false;
        if (info.st_mode & S_IFDIR)
            return true;
        return false;
    }

    std::string _outputFolderPath;

    bool _isValid;
    std::string _validityDescriprion;

    TracksInfo_t _tracksInfo;
};
