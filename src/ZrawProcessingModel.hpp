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
    typedef enum InputFileInfoState_e
    {
        NotConverted,
        Unconvertable,
        InProcess,
        ConversionOk,
        ConversionInterrupted,
        ConversionFailed
    } InputFileInfoState_t;

    typedef enum RawCompression_e
    {
        None,
        LosslessJPEG
    } RawCompression_t;

    typedef enum RawScale_e
    {
        Full,
        Half,
        Quarter
    } RawScale_t;

    // Triggered on each model validity update (bool - isOk, string - description)
    DECLARE_EVENT(void, bool, std::string) EventValidityUpdate;

    // ...
    DECLARE_EVENT(void, std::string) EventMovContainerLogUpdate;

    // Triggered on each input file list change
    DECLARE_EVENT(void) EventInputFilePathsUpdate;

    // Triggered on each input file check (like [v] in the view)
    DECLARE_EVENT(void, std::string, bool) EventInputFilePathEnabledUpdate;

    // Triggered on each input file selection (click on it or smth else)
    DECLARE_EVENT(void, std::string) EventInputFilePathSelectedUpdate;

    // Triggered on each input file conversion state change
    DECLARE_EVENT(void, std::string, InputFileInfoState_t) EventInputFilePathConversionStateUpdate;

    ZrawProcessingModel() : _isValid(false), _compression(LosslessJPEG), _scale(Full)
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

    void RawCompression_set(RawCompression_t compression)
    {
        _compression = compression;
    }

    RawCompression_t RawCompression_get()
    {
        return _compression;
    }

    void RawScale_set(RawScale_t scale)
    {
        _scale = scale;
    }

    RawScale_t RawScale_get()
    {
        return _scale;
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

        auto& info = it0->second.mov;
        if (info.Tracks().size() == 0)
            return "";

        std::string res = "";
        for (auto it = info.Tracks().begin(); it != info.Tracks().end(); ++it)
        {
            auto& track = *it;
            if (track.Media().Type() != TinyMovTrackMedia::Type_t::Video)
                continue;

            auto& desc_table = track.Media().Info().DescriptionTable().VideoDescriptionTable();
            if (desc_table.size() != 1)
                continue;

            auto ext_zraw = desc_table[0].Ext_ZRAW();
            if (!ext_zraw.exists)
                continue;

            switch (ext_zraw.version)
            {
            case 0x12EA78D2:
                res = "RAW CFA";
                break;

            case 0x45A32DEF:
                res = "HEVC";
                {
                    switch (ext_zraw.unk1)
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
        return res;
    }

    void InputFilePath_add(std::vector<std::string>& paths)
    {
        bool anythingChanged = false;

        for (auto it = paths.begin(); it != paths.end(); ++it)
        {
            if (_inputFilesPaths.find(*it) != _inputFilesPaths.end())
                continue;

            InputFileInfo_t info;
            info.fileVerified = false;
            info.isConvertable = false;
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

        TinyMovFile FileInfo;
    };

    std::unique_ptr<ValidInfo> ValidInfo_get()
    {
        const auto it = _inputFilesPaths.find(InputFilePathActive_get());
        if (it == _inputFilesPaths.end())
            return nullptr;

        auto res = std::make_unique<ValidInfo>();
        res->OutputPath = OutputFolderPath_get();
        res->FileInfo = it->second.mov;

        return res;
    }

    void InputFilePath_enable(std::string path, bool isEnabled)
    {
        const auto it = _inputFilesPaths.find(path);
        if (it == _inputFilesPaths.end())
            return;

        if (it->second.isConvertable == false)
            return;

        it->second.enabled = isEnabled;
        InputFilePathConversionState_set(path, NotConverted);

        TRIGGER_EVENT(EventInputFilePathEnabledUpdate, it->first, isEnabled);
    }

    std::vector<std::string> InputFilePathsEnabled_get()
    {
        std::vector<std::string> res;

        for (auto it = _inputFilesPaths.begin(); it != _inputFilesPaths.end(); ++it)
        {
            if (!it->second.enabled)
                continue;

            if (!it->second.fileVerified)
                continue;

            if (!it->second.isConvertable)
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

    /* Conversion State */

    void InputFilePathConversionState_set(std::string path, InputFileInfoState_t state)
    {
        const auto it = _inputFilesPaths.find(path);
        if (it == _inputFilesPaths.end())
            return;

        if (it->second.conversionState == state)
            return;

        it->second.conversionState = state;

        TRIGGER_EVENT(EventInputFilePathConversionStateUpdate, path, state);
    }

    InputFileInfoState_t InputFilePathConversionState_get(std::string path)
    {
        const auto it = _inputFilesPaths.find(path);
        if (it == _inputFilesPaths.end())
            return Unconvertable;

        return it->second.conversionState;
    }

    /* Progress Bar */

    // WARNING! Do not save this returned reference! It can become inactive after the input file is deleted from the list!
    IProgressBar* InputFilePathProgressBar_get(std::string path)
    {
        const auto it = _inputFilesPaths.find(path);
        if (it == _inputFilesPaths.end())
            return nullptr;

        return &it->second.conversionProgress;
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

            if (pathInfo.fileVerified)
            {
                if (!pathInfo.isConvertable)
                    continue;
            }
            else if (!_checkFile(path, pathInfo))
                continue;

            atLeastOneZrawFileIsFound = true;
        }

        return atLeastOneZrawFileIsFound;
    }

    class ProgressBarInterface : public IProgressBar
    {
    public:
        ProgressBarInterface() : _percent(0) {}

        void ChangePercent(unsigned int percent) override
        {
            if (_percent == percent)
                return;

            _percent = percent;

            TRIGGER_EVENT(EventPercentUpdate, this, percent);
        }

        void SetDescription(std::string format, ...) override
        {
            // 1. Calculate buffer length
            va_list args;
            va_start(args, format);
            auto bufsz = vsnprintf(NULL, 0, format.c_str(), args);
            va_end(args);

            // 2. Create buffer
            char *buffer = new char[bufsz + 1];

            // 3. Print to buffer
            va_start(args, format);
            vsprintf(buffer, format.c_str(), args);
            va_end(args);

            // 4. Set caption
            auto str = std::string(buffer);
            if (_description != str)
            {
                _description = std::string(buffer);
                TRIGGER_EVENT(EventDescriptionUpdate, this, _description);
            }

            // 5. Remove buffer
            delete[] buffer;
        }

    protected:
        unsigned int _percent;
        std::string _description;
    };

    typedef struct
    {
        bool fileVerified;
        bool isConvertable;
        bool enabled;

        TracksInfo_t tracksInfo;
        TinyMovFile mov;

        InputFileInfoState_t conversionState;
        ProgressBarInterface conversionProgress;
    } InputFileInfo_t;
    std::map<std::string, InputFileInfo_t> _inputFilesPaths;

    bool _checkFile(std::string path, InputFileInfo_t& info)
    {
        info.fileVerified = true;
        info.isConvertable = false;

        if (path.empty())
        {
            InputFilePathConversionState_set(path, Unconvertable);
            return false;
        }

        if (!_fileExists(path))
        {
            InputFilePathConversionState_set(path, Unconvertable);
            return false;
        }

        // Read input file
        //auto tracksInfo = MovDetectTracks(path.c_str());
        //info.tracksInfo = tracksInfo;

        try
        {
            TinyMovFileReader reader;
            info.mov = reader.OpenMovFile(path.c_str());
        }
        catch (...)
        {
            InputFilePathConversionState_set(path, Unconvertable);
            return false;
        }

        //TRIGGER_EVENT(EventMovContainerLogUpdate, info.output_log);

        // Find ZRAW tracks
        int zrawTrackIndex = -1;
        for (int i = 0; i < info.mov.Tracks().size(); ++i)
        {
            auto& track = info.mov.Tracks()[i];
            if (track.Media().Type() != TinyMovTrackMedia::Type_t::Video)
                continue;

            auto& desc_table = track.Media().Info().DescriptionTable().VideoDescriptionTable();
            if (desc_table.size() != 1)
                continue;

            auto& desc = desc_table[0];

            if (desc.DataFormat() == MKTAG('z', 'r', 'a', 'w'))
                zrawTrackIndex = i;
        }

        if (zrawTrackIndex != -1)
        {
            info.isConvertable = true;
            InputFilePathConversionState_set(path, NotConverted);
        }
        else
            InputFilePathConversionState_set(path, Unconvertable);

        // Exit if file does not have any ZRAW tracks
        return info.isConvertable;
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

    RawCompression_t _compression;
    RawScale_t _scale;

};
