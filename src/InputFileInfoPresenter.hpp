#pragma once

#include <IInputFileInfoView.hpp>
#include <IConsoleOutput.hpp>

#include <ZrawProcessingModel.hpp>

class InputFileInfoPresenter
{
public:
    InputFileInfoPresenter(ZrawProcessingModel& model, IInputFileInfoView& view, IConsoleOutput& debug_visitor) : _model(model), _view(view), _debug_visitor(debug_visitor)
    {
        _model.EventInputFilePathSelectedUpdate += MakeDelegate(this, &InputFileInfoPresenter::OnInputPathSelectionChangedInModel);
    }

    ~InputFileInfoPresenter()
    {
        _model.EventInputFilePathSelectedUpdate -= MakeDelegate(this, &InputFileInfoPresenter::OnInputPathSelectionChangedInModel);
    }

    void OnInputPathSelectionChangedInModel(std::string path)
    {
        _view.Clear();

        auto info = _model.ValidInfo_get();
        if (info == nullptr)
        {
            _debug_visitor.printf("Warning! Attempt to show info of null model. Skipped.");
            return;
        }

        // Paths category
        auto& cat0 = _view.CreateCategory("Paths");
        cat0.SetProperty("Output folder", info->OutputPath);

        cat0.SetProperty("Input file", path);

        // Tracks category
        auto& cat = _view.CreateCategory("Tracks");
        for (int i = 0; i < info->TracksInfo.tracks.size(); ++i)
        {
            cat.SetProperty("Track #" + std::to_string(i), info->TracksInfo.tracks[i].codec_name);

            if (info->TracksInfo.tracks[i].codec_name == "zraw")
            {
                auto& zrawCat = _view.CreateCategory("ZRAW (Track #" + std::to_string(i) + ")");

                zrawCat.SetProperty("Frames", "%d", info->TracksInfo.tracks[i].frames.size());
                zrawCat.SetProperty("Version", "0x%X", info->TracksInfo.tracks[i].zraw_raw_version);

                switch(info->TracksInfo.tracks[i].zraw_raw_version)
                {
                case 0x12EA78D2:
                    zrawCat.SetProperty("Type", "Compressed RAW CFA");
                    break;

                case 0x45A32DEF:
                    zrawCat.SetProperty("Type", "HEVC bitstream");
                    {
                        switch(info->TracksInfo.tracks[i].zraw_unk1)
                        {
                        case 0:
                            zrawCat.SetProperty("Encryption", "AES");
                            break;

                        case 1:
                            zrawCat.SetProperty("Encryption", "XOR");
                            break;

                        default:
                            zrawCat.SetProperty("Encryption", "Unknown");
                            break;
                        }
                    }
                    break;

                default:
                    zrawCat.SetProperty("Type", "Unknown");
                    break;
                }

                zrawCat.SetProperty("Frame Width", "%d", info->TracksInfo.tracks[i].width);
                zrawCat.SetProperty("Frame Height", "%d", info->TracksInfo.tracks[i].height);

                if (info->TracksInfo.tracks[i].universal_sample_size != -1)
                    zrawCat.SetProperty("Universal Sample Size", "%d", info->TracksInfo.tracks[i].universal_sample_size);

                auto& zrawFrameSizesCat = _view.CreateCategory("ZRAW (Track #" + std::to_string(i) + ") sample sizes");
                auto& zrawFrameOffsetsCat = _view.CreateCategory("ZRAW (Track #" + std::to_string(i) + ") sample offsets");

                zrawFrameSizesCat.Lock();
                zrawFrameOffsetsCat.Lock();

                for (int p = 0; p < info->TracksInfo.tracks[i].frames.size(); ++p)
                {
                    auto& frame = info->TracksInfo.tracks[i].frames[p];

                    zrawFrameSizesCat.SetProperty("Sample #" + std::to_string(p), "%lld (0x%llX)", frame.frame_size, frame.frame_size);
                    zrawFrameOffsetsCat.SetProperty("Sample #" + std::to_string(p), "%lld (0x%llX)", frame.frame_offset, frame.frame_offset);
                }

                zrawFrameSizesCat.Unlock();
                zrawFrameOffsetsCat.Unlock();
            }
        }
    }

protected:

    ZrawProcessingModel& _model;
    IInputFileInfoView& _view;
    IConsoleOutput& _debug_visitor;
};
