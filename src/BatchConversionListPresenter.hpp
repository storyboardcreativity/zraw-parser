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
        _model.EventInputFilePathConversionStateUpdate += MakeDelegate(this, &BatchConversionListPresenter::OnInputFilePathConversionStateChangeInModel);
    }

    ~BatchConversionListPresenter()
    {
        _model.EventInputFilePathConversionStateUpdate -= MakeDelegate(this, &BatchConversionListPresenter::OnInputFilePathConversionStateChangeInModel);
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
        {
            _view.AddPath(*it, _model.InputFilePathType_get(*it), _model.InputFilePathEnabled_get(*it));

            if (_progressBars0.find(*it) == _progressBars0.end())
            {
                auto pbPtr = _model.InputFilePathProgressBar_get(*it);
                _progressBars0[*it] = pbPtr;
                _progressBars1[pbPtr] = *it;

                pbPtr->EventPercentUpdate += MakeDelegate(this, &BatchConversionListPresenter::OnInputFilePathPercentChangeInProgressBar);
                OnInputFilePathConversionStateChangeInModel(*it, _model.InputFilePathConversionState_get(*it));
            }
        }
    }

    void OnInputFilePathPercentChangeInProgressBar(IProgressBar* pb, unsigned int percent)
    {
        auto pathIt = _progressBars1.find(pb);
        if (pathIt == _progressBars1.end())
            return;

        _view.ChangeInputFilePercent(pathIt->second, percent);
    }

    void OnInputFilePathConversionStateChangeInModel(std::string path, ZrawProcessingModel::InputFileInfoState_t state)
    {
        switch (state)
        {
        case ZrawProcessingModel::Unconvertable:
        {
            _view.ChangeInputFileColor(path, 254, 162, 0, 1.0);
            break;
        }

        case ZrawProcessingModel::InProcess:
        {
            uint32_t color[4] = COLOR5;
            _view.ChangeInputFileColor(path, color[0], color[1], color[2], color[3]);
            break;
        }

        case ZrawProcessingModel::ConversionOk:
        {
            uint32_t color[4] = COLOR9;
            _view.ChangeInputFileColor(path, color[0], color[1], color[2], color[3]);
            break;
        }

        case ZrawProcessingModel::ConversionFailed:
        {
            uint32_t color[4] = COLOR8;
            _view.ChangeInputFileColor(path, color[0], color[1], color[2], color[3]);
            break;
        }

        case ZrawProcessingModel::ConversionInterrupted:
        {
            uint32_t color[4] = COLOR2;
            _view.ChangeInputFileColor(path, color[0], color[1], color[2], color[3]);
            break;
        }

        case ZrawProcessingModel::NotConverted:
        default:
        {
            uint32_t color[4] = COLOR7;
            _view.ChangeInputFileColor(path, color[0], color[1], color[2], color[3]);
            break;
        }
        }
    }

    std::map<std::string, IProgressBar*> _progressBars0;
    std::map<IProgressBar*, std::string> _progressBars1;

    ZrawProcessingModel& _model;
    IBatchConversionListView& _view;
    IConsoleOutput& _debug_visitor;
};
