#pragma once

#include <FileSelectionPresenter.hpp>
#include <InputFileInfoPresenter.hpp>
#include <IConsoleOutput.hpp>
#include <ZrawProcessingModel.hpp>
#include <IMainFormPanelView.hpp>
#include <ZrawConverter.hpp>
#include <BatchConversionListPresenter.hpp>

class MainFormPanelPresenter
{
public:
    MainFormPanelPresenter(
        ZrawProcessingModel& model,
        std::unique_ptr<FileSelectionPresenter> fsp,
        std::unique_ptr<InputFileInfoPresenter> ifip,
        std::unique_ptr<BatchConversionListPresenter> bclp,
        IMainFormPanelView& mfp,
        IConsoleOutput& debug_visitor
    ) : _fsp(std::move(fsp)), _ifip(std::move(ifip)), _bclp(std::move(bclp)), _mfp(mfp), _debug_visitor(debug_visitor), _model(model), _converter(_model, debug_visitor, mfp.ProgressBar())
    {
        _fsp->EventOutputPathSelection += MakeDelegate(this, &MainFormPanelPresenter::OnOutputPathSelection);
        _fsp->EventCompressionModeSelection += MakeDelegate(this, &MainFormPanelPresenter::OnCompressionModeSelection);

        _mfp.EventProcessButtonClick += MakeDelegate(this, &MainFormPanelPresenter::OnProcessButtonClick);

        _model.EventValidityUpdate += MakeDelegate(this, &MainFormPanelPresenter::OnModelValidityUpdate);
        _model.EventMovContainerLogUpdate += MakeDelegate(this, &MainFormPanelPresenter::OnMovContainerLogUpdate);

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

        _model.EventMovContainerLogUpdate -= MakeDelegate(this, &MainFormPanelPresenter::OnMovContainerLogUpdate);
        _model.EventValidityUpdate -= MakeDelegate(this, &MainFormPanelPresenter::OnModelValidityUpdate);

        _mfp.EventProcessButtonClick -= MakeDelegate(this, &MainFormPanelPresenter::OnProcessButtonClick);

        _fsp->EventCompressionModeSelection -= MakeDelegate(this, &MainFormPanelPresenter::OnCompressionModeSelection);
        _fsp->EventOutputPathSelection -= MakeDelegate(this, &MainFormPanelPresenter::OnOutputPathSelection);
    }

protected:

    void OnOutputPathSelection(std::string path)
    {
        _model.OutputFolderPath_set(path);
    }

    void OnCompressionModeSelection(IFileSelectionView::CompressionMode_t mode)
    {
        switch (mode)
        {
        case IFileSelectionView::CompressionMode_t::None:
            _model.RawCompression_set(ZrawProcessingModel::RawCompression_t::None);
            break;

        case IFileSelectionView::CompressionMode_t::LosslessJPEG:
            _model.RawCompression_set(ZrawProcessingModel::RawCompression_t::LosslessJPEG);
            break;

        default:
            break;
        }
    }

    void OnModelValidityUpdate(bool isValid, std::string descriprion)
    {
        _fsp->SetStatusText(descriprion, isValid);

        _mfp.ChangeProcessButtonActivity(isValid);

        if (isValid)
        {
            _mfp.ChangeProcessButtonText("Convert");
        }
    }

    void OnMovContainerLogUpdate(std::string log)
    {
        _debug_visitor.printf(log.c_str());
    }

    void OnProcessButtonClick()
    {
        if (_converter.IsProcessing())
        {
            _converter.InterruptProcess();
            return;
        }

        _bclp->Lock();
        _mfp.ChangeProcessButtonText("Cancel");

        _fsp->SetActivity(false);
        _fsp->SetStatusText("Converting...", true);

        _converter.StartProcess();
    }

    void OnConversionProcessFinish()
    {
        _mfp.ChangeProcessButtonText("Convert");
        _fsp->SetActivity(true);
        _bclp->Unlock();

        // Force model validity update
        OnModelValidityUpdate(_model.IsValid(), _model.ValidityDescriprion());
    }

    std::unique_ptr<FileSelectionPresenter> _fsp;
    std::unique_ptr<InputFileInfoPresenter> _ifip;
    std::unique_ptr<BatchConversionListPresenter> _bclp;

    IMainFormPanelView& _mfp;

    IConsoleOutput& _debug_visitor;

    ZrawProcessingModel& _model;
    ZrawConverter _converter;

    std::string _inputFilePath;
    std::string _outputFolderPath;
};
