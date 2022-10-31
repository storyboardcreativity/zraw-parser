#pragma once

#include <IFileSelectionView.hpp>
#include <IConsoleOutput.hpp>

class FileSelectionPresenter
{
public:
    DECLARE_EVENT(void, std::string) EventOutputPathSelection;
    DECLARE_EVENT(void, IFileSelectionView::CompressionMode_t) EventCompressionModeSelection;

    FileSelectionPresenter(IFileSelectionView& view, IConsoleOutput& debug_visitor) : _view(view), _debug_visitor(debug_visitor)
    {
        _view.EventOutputPathSelection += MakeDelegate(this, &FileSelectionPresenter::OnOutputPathSelection);
        _view.EventCompressionModeSelection += MakeDelegate(this, &FileSelectionPresenter::OnCompressionModeSelection);
    }

    ~FileSelectionPresenter()
    {
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

    IFileSelectionView& _view;
    IConsoleOutput& _debug_visitor;
};
