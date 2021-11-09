#pragma once

#include <FileSelectionPresenter.hpp>
#include <InputFileInfoPresenter.hpp>
#include <IConsoleOutput.hpp>
#include <ZrawProcessingModel.hpp>
#include <IMainFormPanelView.hpp>
#include <ZrawConverter.hpp>

class MainFormPanelPresenter
{
public:
    MainFormPanelPresenter(
        std::unique_ptr<FileSelectionPresenter> fsp,
        std::unique_ptr<InputFileInfoPresenter> ifip,
        IMainFormPanelView& mfp,
        IConsoleOutput& debug_visitor
    ) : _fsp(std::move(fsp)), _ifip(std::move(ifip)), _mfp(mfp), _debug_visitor(debug_visitor), _converter(_model, debug_visitor, mfp.ProgressBar())
    {
        _fsp->EventInputFileSelection += MakeDelegate(this, &MainFormPanelPresenter::OnInputFileSelection);
        _fsp->EventOutputPathSelection += MakeDelegate(this, &MainFormPanelPresenter::OnOutputPathSelection);

        _mfp.EventProcessButtonClick += MakeDelegate(this, &MainFormPanelPresenter::OnProcessButtonClick);

        _model.EventValidityUpdate += MakeDelegate(this, &MainFormPanelPresenter::OnModelValidityUpdate);

        _converter.EventConversionFinished += MakeDelegate(this, &MainFormPanelPresenter::OnConversionProcessFinish);

        _mfp.ChangeProcessButtonText("Convert");
        _mfp.ChangeProcessButtonActivity(false);
        _mfp.ProgressBar().ChangePercent(0);

        // Force model validity update
        OnModelValidityUpdate(_model.IsValid(), _model.ValidityDescriprion());
    }

    ~MainFormPanelPresenter()
    {
        _converter.EventConversionFinished -= MakeDelegate(this, &MainFormPanelPresenter::OnConversionProcessFinish);

        _model.EventValidityUpdate -= MakeDelegate(this, &MainFormPanelPresenter::OnModelValidityUpdate);

        _mfp.EventProcessButtonClick -= MakeDelegate(this, &MainFormPanelPresenter::OnProcessButtonClick);

        _fsp->EventOutputPathSelection -= MakeDelegate(this, &MainFormPanelPresenter::OnOutputPathSelection);
        _fsp->EventInputFileSelection -= MakeDelegate(this, &MainFormPanelPresenter::OnInputFileSelection);
    }

protected:

    void OnInputFileSelection(std::string path)
    {
        _model.InputFilePath_set(path);
    }

    void OnOutputPathSelection(std::string path)
    {
        _model.OutputFolderPath_set(path);
    }

    void OnModelValidityUpdate(bool isValid, std::string descriprion)
    {
        _fsp->SetStatusText(descriprion, isValid);

        _mfp.ChangeProcessButtonActivity(isValid);

        if (isValid)
        {
            _mfp.ChangeProcessButtonText("Convert");
            _ifip->UpdateInfo(_model);
        }
    }

    void OnProcessButtonClick()
    {
        if (_converter.IsProcessing())
        {
            _converter.InterruptProcess();
            return;
        }

        _mfp.ChangeProcessButtonText("Cancel");

        _fsp->SetActivity(false);
        _fsp->SetStatusText("Converting...", true);

        _converter.StartProcess();
    }

    void OnConversionProcessFinish()
    {
        _mfp.ChangeProcessButtonText("Convert");
        _fsp->SetActivity(true);

        // Force model validity update
        OnModelValidityUpdate(_model.IsValid(), _model.ValidityDescriprion());
    }

    std::unique_ptr<FileSelectionPresenter> _fsp;
    std::unique_ptr<InputFileInfoPresenter> _ifip;
    IMainFormPanelView& _mfp;

    IConsoleOutput& _debug_visitor;

    ZrawProcessingModel _model;
    ZrawConverter _converter;

    std::string _inputFilePath;
    std::string _outputFolderPath;
};
