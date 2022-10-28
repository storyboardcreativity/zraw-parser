#pragma once

#include <thread>

#include <MainForm.hpp>
#include <MainFormPanel.hpp>
#include <MainFormPresenter.hpp>
#include <FileSelectionUserControl.hpp>
#include <DebugTabUserControl.hpp>
#include <AboutTabUserControl.hpp>
#include <FileSelectionPresenter.hpp>
#include <MainFormPanelPresenter.hpp>
#include <InputFileInfoPresenter.hpp>
#include <InputFileInfoUserControl.hpp>
#include <BatchConversionListUserControl.hpp>
#include <BatchConversionListPresenter.hpp>

class MainFormPresenter
{
public:
    MainFormPresenter(std::unique_ptr<MainForm> mainForm)
        : _view(std::move(mainForm)),
        _mainPanel(std::make_unique<MainFormPanel>()),
        _debugView(std::make_unique<DebugTabUserControl>()),
        _aboutView(std::make_unique<AboutTabUserControl>()),
        _fsuc0(std::make_unique<FileSelectionUserControl>()),
        _ifiuc0(std::make_unique<InputFileInfoUserControl>()),
        _bcluc(std::make_unique<BatchConversionListUserControl>())
    {

        _view->UserControl__set(*_mainPanel);
        _mainPanel->AddTab("Batch File List", *_bcluc);
        _mainPanel->AddTab("Settings", *_fsuc0);
        _mainPanel->AddTab("File Info", *_ifiuc0);
        _mainPanel->AddTab("Debug", *_debugView);
        _mainPanel->AddTab("About", *_aboutView);

        _mainPanelPresenter = std::make_unique<MainFormPanelPresenter>(
            _model,
            std::make_unique<FileSelectionPresenter>(*_fsuc0, _debugView->Console()),
            std::make_unique<InputFileInfoPresenter>(_model, *_ifiuc0, _debugView->Console()),
            std::make_unique<BatchConversionListPresenter>(_model, *_bcluc, _debugView->Console()),
            *_mainPanel,
            _debugView->Console());
        
        _debugView->Console().printf("--------------------------------------");
        _debugView->Console().printf("ZRAW Video Converter v%s started!", ZRAW_PARSER_VERSION_STRING);
        _debugView->Console().printf("Threads available: %d", std::thread::hardware_concurrency());
        _debugView->Console().printf("--------------------------------------");
    }

    void Run() const
    {
        _view->Show();
    }

protected:
    ZrawProcessingModel _model;

    std::unique_ptr<MainForm> _view;
    std::unique_ptr<MainFormPanel> _mainPanel;

    std::unique_ptr<DebugTabUserControl> _debugView;
    std::unique_ptr<AboutTabUserControl> _aboutView;

    std::unique_ptr<FileSelectionUserControl> _fsuc0;
    std::unique_ptr<InputFileInfoUserControl> _ifiuc0;
    std::unique_ptr<BatchConversionListUserControl> _bcluc;

    std::unique_ptr<MainFormPanelPresenter> _mainPanelPresenter;
};