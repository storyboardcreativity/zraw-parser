#pragma once

#include <IBatchConversionListView.hpp>
#include <IConsoleOutput.hpp>

class BatchConversionListPresenter
{
public:
    BatchConversionListPresenter(
        ZrawProcessingModel& model,
        IBatchConversionListView& view,
        IConsoleOutput& debug_visitor)
        : _model(model), _view(view), _debug_visitor(debug_visitor)
    {
        _view.EventInputFileSelected += MakeDelegate(this, &BatchConversionListPresenter::OnPathSelectedInView);
        _view.EventInputFilesAdded += MakeDelegate(this, &BatchConversionListPresenter::OnFilesAddedFromView);
        _view.EventInputFileEnabled += MakeDelegate(this, &BatchConversionListPresenter::OnPathEnabledInView);
        _model.EventInputFilePathEnabledUpdate += MakeDelegate(this, &BatchConversionListPresenter::OnPathEnabledInModel);
        _model.EventInputFilePathsUpdate += MakeDelegate(this, &BatchConversionListPresenter::OnInputFilePathsUpdateInModel);
    }

    ~BatchConversionListPresenter()
    {
        _model.EventInputFilePathsUpdate -= MakeDelegate(this, &BatchConversionListPresenter::OnInputFilePathsUpdateInModel);
        _model.EventInputFilePathEnabledUpdate -= MakeDelegate(this, &BatchConversionListPresenter::OnPathEnabledInModel);
        _view.EventInputFileEnabled -= MakeDelegate(this, &BatchConversionListPresenter::OnPathEnabledInView);
        _view.EventInputFilesAdded -= MakeDelegate(this, &BatchConversionListPresenter::OnFilesAddedFromView);
        _view.EventInputFileSelected -= MakeDelegate(this, &BatchConversionListPresenter::OnPathSelectedInView);
    }

    void Lock() const
    {
        _view.Lock();
    }

    void Unlock() const
    {
        _view.Unlock();
    }

protected:

    void OnFilesAddedFromView(std::vector<std::string>& addedFilesList)
    {
        _model.InputFilePath_add(addedFilesList);
    }

    void OnPathEnabledInView(std::string path, bool isEnabled)
    {
        _model.InputFilePath_enable(path, isEnabled);
    }

    void OnPathSelectedInView(std::string path)
    {
        _model.InputFilePathActive_set(path);
    }

    void OnPathEnabledInModel(std::string path, bool isEnabled)
    {
        _view.SetPathEnabled(path, isEnabled);
    }

    void OnInputFilePathsUpdateInModel()
    {
        auto pathsList = _model.InputFilePaths_get();
        for (auto it = pathsList.begin(); it != pathsList.end(); ++it)
            _view.AddPath(*it, _model.InputFilePathType_get(*it), _model.InputFilePathEnabled_get(*it));
    }

    ZrawProcessingModel& _model;
    IBatchConversionListView& _view;
    IConsoleOutput& _debug_visitor;
};
