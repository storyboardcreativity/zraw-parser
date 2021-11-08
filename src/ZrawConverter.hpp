#pragma once

#include <thread>

#include <ZrawProcessingModel.hpp>
#include <ZrawNewExtractor.hpp>
#include <IProgressBar.hpp>

#include <Event.h>
#include <atomic>

class ZrawConverter
{
public:
    ZrawConverter(ZrawProcessingModel& model, IConsoleOutput& console, IProgressBar& progressBar)
        : _model(model), _console(console), _progressBar(progressBar), _isStarted(false), _itsTimeToStopOkay(false) {}

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

        _itsTimeToStopOkay = false;

        _taskThread = std::thread(
            &ZrawConverter::TaskThreadMethod, this,
            std::ref(_itsTimeToStopOkay),
            std::ref(_console),
            std::ref(_progressBar),
            _model.InputFilePath_get(),
            _model.OutputFolderPath_get());
    }

    void InterruptProcess()
    {
        _itsTimeToStopOkay = true;
        _progressBar.SetDescription("Stopping...");
        _console.printf("Stopping conversion process...\n");
    }

    bool IsProcessing()
    {
        return _isStarted;
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

    std::atomic<bool> _isStarted;
    std::atomic<bool> _itsTimeToStopOkay;

    void TaskThreadMethod(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput& console, IProgressBar& progressBar, std::string inputFilePath, std::string outputFilePath)
    {
        progressBar.ChangePercent(0);
        progressBar.SetDescription("Starting conversion process...");

        ZrawNewExtractor extractor;
        switch (extractor.ProcessConversion(itsTimeToStopOkay, console, progressBar, inputFilePath, outputFilePath))
        {
        case ZrawNewExtractor::Done:
            console.printf("Finished conversion process\n");
            progressBar.SetDescription("Finished conversion process!");
            progressBar.ChangePercent(100);
            break;

        case ZrawNewExtractor::InputFileOpenFailed:
            console.printf("Failed to open input file!\n");
            progressBar.SetDescription("Failed to open input file! Stopped.");
            progressBar.ChangePercent(0);
            break;

        case ZrawNewExtractor::Interrupted:
            console.printf("Interrupted! Stopped.\n");
            progressBar.SetDescription("Interrupted! Stopped.");
            progressBar.ChangePercent(0);
            break;

        case ZrawNewExtractor::OutputFileOpenFailed:
            console.printf("Failed to open output file! Stopped.\n");
            progressBar.SetDescription("Failed to open output file! Stopped.");
            progressBar.ChangePercent(0);
            break;

        case ZrawNewExtractor::NotImplemented:
            console.printf("Feature is not implemented yet! Stopped.\n");
            progressBar.SetDescription("Feature is not implemented yet! Stopped.");
            progressBar.ChangePercent(0);
            break;
        }
        
        TRIGGER_EVENT(EventConversionFinished);

        _isStarted = false;
    }
};
