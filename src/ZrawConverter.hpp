#pragma once

#include <thread>

#include <ZrawProcessingModel.hpp>
#include <ZrawNewExtractor.hpp>
#include <IProgressBar.hpp>

#include <Event.h>

class ZrawConverter
{
public:
    ZrawConverter(ZrawProcessingModel& model, IConsoleOutput& console, IProgressBar& progressBar)
        : _model(model), _console(console), _isStarted(false), _progressBar(progressBar) {}

    void StartProcess()
    {
        if (_isStarted)
        {
            _console.printf("Warning! Attempt to start conversion thread when previous task is not finished yet! Skipped.");
            return;
        }

        _isStarted = true;

        if (_taskThread.joinable())
            _taskThread.join();

        _taskThread = std::thread(
            &ZrawConverter::TaskThreadMethod, this,
            std::ref(_console),
            std::ref(_progressBar),
            _model.InputFilePath_get(),
            _model.OutputFolderPath_get());
    }

    ~ZrawConverter()
    {
        if (_taskThread.joinable())
            _taskThread.join();
    }

    DECLARE_EVENT(void) EventConversionFinished;

protected:
    std::thread _taskThread;
    ZrawProcessingModel& _model;
    IConsoleOutput& _console;
    IProgressBar& _progressBar;

    bool _isStarted;

    void TaskThreadMethod(IConsoleOutput& console, IProgressBar& progressBar, std::string inputFilePath, std::string outputFilePath)
    {
        progressBar.ChangePercent(0);
        progressBar.SetDescription("Starting conversion process...");

        ZrawNewExtractor extractor;
        auto result = extractor.ProcessConversion(console, progressBar, inputFilePath, outputFilePath);
        console.printf("Finished conversion process\n");

        progressBar.SetDescription("Finished conversion process!");
        progressBar.ChangePercent(100);

        TRIGGER_EVENT(EventConversionFinished);

        _isStarted = false;
    }
};
