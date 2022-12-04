#pragma once

#include <IFileSelectionView.hpp>
#include <IConsoleOutput.hpp>

class FileSelectionPresenter
{
public:
    DECLARE_EVENT(void, std::string) EventOutputPathSelection;
    DECLARE_EVENT(void, IFileSelectionView::CompressionMode_t) EventCompressionModeSelection;
    DECLARE_EVENT(void, IFileSelectionView::RawScaleMode_t) EventRawScaleModeSelection;

    FileSelectionPresenter(IFileSelectionView& view, IConsoleOutput& debug_visitor) : _view(view), _debug_visitor(debug_visitor)
    {
        _view.EventOutputPathSelection += MakeDelegate(this, &FileSelectionPresenter::OnOutputPathSelection);
        _view.EventCompressionModeSelection += MakeDelegate(this, &FileSelectionPresenter::OnCompressionModeSelection);
        _view.EventRawScaleModeSelection += MakeDelegate(this, &FileSelectionPresenter::OnRawScaleModeSelection);
    }

    ~FileSelectionPresenter()
    {
        _view.EventRawScaleModeSelection -= MakeDelegate(this, &FileSelectionPresenter::OnRawScaleModeSelection);
        _view.EventCompressionModeSelection -= MakeDelegate(this, &FileSelectionPresenter::OnCompressionModeSelection);
        _view.EventOutputPathSelection -= MakeDelegate(this, &FileSelectionPresenter::OnOutputPathSelection);
    }

    void SetStatusText(std::string text, bool isOk)
    {
        _view.SetStatusText(text, isOk);
    }

    void SetActivity(bool isActive)
    {
        _view.SetActivity(isActive);
    }

protected:

    void OnOutputPathSelection(std::string path)
    {
        _debug_visitor.printf("Selected output path: %s\n", path.c_str());
        _view.SetSelectedOutputPathFieldText(path);

        TRIGGER_EVENT(EventOutputPathSelection, path);
    }

    void OnCompressionModeSelection(IFileSelectionView::CompressionMode_t mode)
    {
        switch (mode)
        {
        case IFileSelectionView::CompressionMode_t::None:
            _debug_visitor.printf("Selected compression mode: %s\n", "None");
            break;

        case IFileSelectionView::CompressionMode_t::LosslessJPEG:
            _debug_visitor.printf("Selected compression mode: %s\n", "Lossless JPEG");
            break;

        default:
            _debug_visitor.printf("Selected compression mode: %s\n", "Unknown!");
            break;
        }

        _view.SetCompressionMode(mode);

        TRIGGER_EVENT(EventCompressionModeSelection, mode);
    }

    void OnRawScaleModeSelection(IFileSelectionView::RawScaleMode_t mode)
    {
        switch (mode)
        {
        case IFileSelectionView::RawScaleMode_t::Full:
            _debug_visitor.printf("Selected RAW scale mode: %s\n", "Full (1/1)");
            break;

        case IFileSelectionView::RawScaleMode_t::Half:
            _debug_visitor.printf("Selected RAW scale mode: %s\n", "Half (1/2)");
            break;

        case IFileSelectionView::RawScaleMode_t::Quarter:
            _debug_visitor.printf("Selected RAW scale mode: %s\n", "Quarter (1/4)");
            break;

        default:
            _debug_visitor.printf("Selected RAW scale mode: %s\n", "Unknown!");
            break;
        }

        _view.SetRawScaleMode(mode);

        TRIGGER_EVENT(EventRawScaleModeSelection, mode);
    }

    IFileSelectionView& _view;
    IConsoleOutput& _debug_visitor;
};
