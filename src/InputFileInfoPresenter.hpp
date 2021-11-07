#pragma once

#include <IInputFileInfoView.hpp>
#include <IConsoleOutput.hpp>

class InputFileInfoPresenter
{
public:
    InputFileInfoPresenter(IInputFileInfoView& view, IConsoleOutput& debug_visitor) : _view(view), _debug_visitor(debug_visitor)
    {
    }

    ~InputFileInfoPresenter()
    {
    }

    void UpdateInfo(TracksInfo_t& info)
    {
        _view.Clear();
        auto& cat = _view.CreateCategory("Tracks");
        for (int i = 0; i < info.tracks.size(); ++i)
        {
            cat.SetProperty("Track #" + std::to_string(i), info.tracks[i].codec_name);

            if (info.tracks[i].codec_name == "zraw")
            {
                auto& zrawCat = _view.CreateCategory("ZRAW (Track #" + std::to_string(i) + ")");
                zrawCat.SetProperty("Frames", "%d", info.tracks[i].frames.size());
                zrawCat.SetProperty("Version", "0x%X", info.tracks[i].zraw_raw_version);
                
                
            }
        }
    }

protected:

    IInputFileInfoView& _view;
    IConsoleOutput& _debug_visitor;
};
