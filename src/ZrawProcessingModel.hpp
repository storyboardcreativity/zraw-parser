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
    DECLARE_EVENT(void) EventInputFilePathsUpdate;
    DECLARE_EVENT(void, std::string, bool) EventInputFilePathEnabledUpdate;
    DECLARE_EVENT(void, std::string) EventInputFilePathSelectedUpdate;

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

    void InputFilePathActive_set(std::string path)
    {
        if (path == _inputFilePathActive)
            return;

        _inputFilePathActive = (_inputFilesPaths.find(path) == _inputFilesPaths.end()) ? "" : path;

        TRIGGER_EVENT(EventInputFilePathSelectedUpdate, _inputFilePathActive);
    }

    std::string InputFilePathActive_get() const
    {
        return _inputFilePathActive;
    }

    bool InputFilePathEnabled_get(std::string path)
    {
        const auto it = _inputFilesPaths.find(path);
        if (it == _inputFilesPaths.end())
            return false;

        return it->second.enabled;
    }

    std::string InputFilePathType_get(std::string path)
    {
        const auto it0 = _inputFilesPaths.find(path);
        if (it0 == _inputFilesPaths.end())
            return "";

        auto& info = it0->second.tracksInfo;
        if (info.tracks.size() == 0)
            return "";

        std::string res = "";
        for (auto it = info.tracks.begin(); it != info.tracks.end(); ++it)
        {
            if (it->codec_name == "zraw")
            {
                switch (it->zraw_raw_version)
                {
                case 0x12EA78D2:
                    res = "RAW CFA";
                    break;

                case 0x45A32DEF:
                    res = "HEVC";
                    {
                        switch (it->zraw_unk1)
                        {
                        case 0:
                            res += " (AES)";
                            break;

                        case 1:
                            res += " (XOR)";
                            break;

                        default:
                            res += " (UNK)";
                            break;
                        }
                    }
                    break;

                default:
                    break;
                }

                break;
            }
        }
        return res;
    }

    void InputFilePath_add(std::vector<std::string>& paths)
    {
        bool anythingChanged = false;

        for (auto it = paths.begin(); it != paths.end(); ++it)
        {
            if (_inputFilesPaths.find(*it) != _inputFilesPaths.end())
                return;

            InputFileInfo_t info;
            info.checked = false;
            info.correct = false;
            info.enabled = true;
            _inputFilesPaths[*it] = info;

            anythingChanged = true;
        }

        if (!anythingChanged)
            return;
        
        _updateInputFilesValidity();

        TRIGGER_EVENT(EventInputFilePathsUpdate);
    }

    void InputFilePath_remove(std::string path)
    {
        const auto it = _inputFilesPaths.find(path);
        if (it == _inputFilesPaths.end())
            return;

        _inputFilesPaths.erase(it);

        TRIGGER_EVENT(EventInputFilePathsUpdate);
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
        const auto it = _inputFilesPaths.find(InputFilePathActive_get());
        if (it == _inputFilesPaths.end())
            return nullptr;

        auto res = std::make_unique<ValidInfo>();
        res->OutputPath = OutputFolderPath_get();
        res->TracksInfo = it->second.tracksInfo;

        return res;
    }

    void InputFilePath_enable(std::string path, bool isEnabled)
    {
        const auto it = _inputFilesPaths.find(path);
        if (it == _inputFilesPaths.end())
            return;

        it->second.enabled = isEnabled;

        TRIGGER_EVENT(EventInputFilePathEnabledUpdate, it->first, isEnabled);
    }

    std::vector<std::string> InputFilePathsEnabled_get()
    {
        std::vector<std::string> res;

        for (auto it = _inputFilesPaths.begin(); it != _inputFilesPaths.end(); ++it)
        {
            if (!it->second.enabled)
                continue;

            if (!it->second.checked)
                continue;

            if (!it->second.correct)
                continue;

            res.push_back(it->first);
        }

        return res;
    }

    std::vector<std::string> InputFilePaths_get()
    {
        std::vector<std::string> res;

        for (auto it = _inputFilesPaths.begin(); it != _inputFilesPaths.end(); ++it)
            res.push_back(it->first);

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

        if (!_updateInputFilesValidity())
        {
            _isValid = false;
            _validityDescriprion = "No any ZRAW files were found in the input file list.";
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

    bool _updateInputFilesValidity()
    {
        bool atLeastOneZrawFileIsFound = false;
        for (auto it = _inputFilesPaths.begin(); it != _inputFilesPaths.end(); ++it)
        {
            auto& path = it->first;
            auto& pathInfo = it->second;

            if (pathInfo.checked)
            {
                if (!pathInfo.correct)
                    continue;
            }
            else if (!_checkFile(path, pathInfo))
                continue;

            atLeastOneZrawFileIsFound = true;
        }

        return atLeastOneZrawFileIsFound;
    }

    typedef struct
    {
        bool checked;
        bool correct;
        bool enabled;
        TracksInfo_t tracksInfo;
    } InputFileInfo_t;
    std::map<std::string, InputFileInfo_t> _inputFilesPaths;

    bool _checkFile(std::string path, InputFileInfo_t& info)
    {
        info.checked = true;
        info.correct = false;

        if (path.empty())
            return false;

        if (!_fileExists(path))
            return false;

        // Read input file
        auto tracksInfo = MovDetectTracks(path.c_str());
        info.tracksInfo = tracksInfo;

        //TRIGGER_EVENT(EventMovContainerLogUpdate, info.output_log);

        // Find ZRAW tracks
        int zrawTrackIndex = -1;
        for (int i = 0; i < tracksInfo.tracks.size(); ++i)
        {
            if (tracksInfo.tracks[i].codec_name == "zraw")
            {
                // We can't have more than 1 ZRAW track in video file!
                if (zrawTrackIndex != -1)
                    return false;

                zrawTrackIndex = i;
            }
        }

        if (zrawTrackIndex != -1)
            info.correct = true;

        // Exit if file does not have any ZRAW tracks
        return info.correct;
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

    std::string _inputFilePathActive;

    bool _isValid;
    std::string _validityDescriprion;
};
