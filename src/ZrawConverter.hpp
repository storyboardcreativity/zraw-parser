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
            std::ref(_progressBar));
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

    void TaskThreadMethod(std::atomic<bool>& itsTimeToStopOkay, IConsoleOutput& console, IProgressBar& progressBarGlobal)
    {
        auto vec = _model.InputFilePathsEnabled_get();
        auto outputFilePath = _model.OutputFolderPath_get();

        console.printf("ZRAW Converter - Task Started!");

        progressBarGlobal.ChangePercent(0);
        progressBarGlobal.SetDescription("Starting conversion process...");

        if (vec.empty())
        {
            progressBarGlobal.ChangePercent(100);
            progressBarGlobal.SetDescription("Input file queue is empty!");
            console.printf("Input file queue is empty!");
        }
        else
        {
            float step = vec.empty() ? 0.0f : 100.0f / vec.size();
            float pos = 0.0f;
            int i = 0;

            auto it = vec.begin();
            for (; it != vec.end(); ++it)
            {
                auto* progressBarLocalPtr = _model.InputFilePathProgressBar_get(*it);
                if (progressBarLocalPtr == nullptr)
                {
                    console.printf("Warning! Could not get progress bar from model for path \"%s\" - skipping!\n", *it);
                    progressBarGlobal.SetDescription("Failed! Stopped. (watch debug log)");
                    progressBarGlobal.ChangePercent(0);
                    break;
                }
                auto& progressBarLocal = *progressBarLocalPtr;

                if (itsTimeToStopOkay)
                {
                    console.printf("Interrupted! Stopped.\n");

                    progressBarLocal.SetDescription("Interrupted! Stopped.");
                    progressBarLocal.ChangePercent(0);

                    progressBarGlobal.SetDescription("Interrupted! Stopped.");
                    progressBarGlobal.ChangePercent(0);

                    break;
                }

                progressBarGlobal.SetDescription("Processing %d file of %d...", i, vec.size());
                _model.InputFilePathConversionState_set(*it, ZrawProcessingModel::InputFileInfoState_e::InProcess);

                ZrawNewExtractor extractor;
                switch (extractor.ProcessConversion(itsTimeToStopOkay, console, progressBarLocal, *it, outputFilePath, _model.RawCompression_get(), _model.RawScale_get()))
                {
                case ZrawNewExtractor::Done:
                    console.printf("Finished conversion process.\n");
                    progressBarLocal.SetDescription("Finished conversion process!");
                    progressBarLocal.ChangePercent(100);

                    _model.InputFilePath_enable(*it, false);
                    _model.InputFilePathConversionState_set(*it, ZrawProcessingModel::InputFileInfoState_e::ConversionOk);
                    break;

                case ZrawNewExtractor::InputFileOpenFailed:
                    console.printf("Failed to open input file!\n");
                    progressBarLocal.SetDescription("Failed to open input file! Stopped.");
                    progressBarLocal.ChangePercent(0);

                    _model.InputFilePath_enable(*it, false);
                    _model.InputFilePathConversionState_set(*it, ZrawProcessingModel::InputFileInfoState_e::ConversionFailed);
                    break;

                case ZrawNewExtractor::DirectoryCreationFailed:
                    console.printf("Failed to create output directory!\n");
                    progressBarLocal.SetDescription("Failed to create output directory! Stopped.");
                    progressBarLocal.ChangePercent(0);

                    _model.InputFilePathConversionState_set(*it, ZrawProcessingModel::InputFileInfoState_e::ConversionFailed);
                    break;

                case ZrawNewExtractor::Interrupted:
                    console.printf("Interrupted! Stopped.\n");
                    progressBarLocal.SetDescription("Interrupted! Stopped.");
                    progressBarLocal.ChangePercent(0);

                    _model.InputFilePathConversionState_set(*it, ZrawProcessingModel::InputFileInfoState_e::ConversionInterrupted);
                    break;

                case ZrawNewExtractor::OutputFileOpenFailed:
                    console.printf("Failed to open output file! Stopped.\n");
                    progressBarLocal.SetDescription("Failed to open output file! Stopped.");
                    progressBarLocal.ChangePercent(0);

                    _model.InputFilePathConversionState_set(*it, ZrawProcessingModel::InputFileInfoState_e::ConversionFailed);
                    break;

                default:
                case ZrawNewExtractor::NotImplemented:
                    console.printf("Feature is not implemented yet! Stopped.\n");
                    progressBarLocal.SetDescription("Feature is not implemented yet! Stopped.");
                    progressBarLocal.ChangePercent(0);

                    _model.InputFilePathConversionState_set(*it, ZrawProcessingModel::InputFileInfoState_e::ConversionFailed);
                    break;
                }

                pos += step;
                ++i;
                progressBarGlobal.ChangePercent((unsigned int)pos);
            }

            if (it == vec.end())
            {
                console.printf("Finished conversion process.\n");
                progressBarGlobal.SetDescription("Finished conversion process!");
                progressBarGlobal.ChangePercent(100);
            }
        }

        console.printf("ZRAW Converter - Task Finished!");

        TRIGGER_EVENT(EventConversionFinished);
        _isStarted = false;
    }
};
