#pragma once

#include <IFileSelectionView.hpp>
#include <IConsoleOutput.hpp>

class FileSelectionPresenter
{
public:
    DECLARE_EVENT(void, std::string) EventOutputPathSelection;

    FileSelectionPresenter(IFileSelectionView& view, IConsoleOutput& debug_visitor) : _view(view), _debug_visitor(debug_visitor)
    {
        _view.EventOutputPathSelection += MakeDelegate(this, &FileSelectionPresenter::OnOutputPathSelection);
    }

    ~FileSelectionPresenter()
    {
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

    IFileSelectionView& _view;
    IConsoleOutput& _debug_visitor;
};
