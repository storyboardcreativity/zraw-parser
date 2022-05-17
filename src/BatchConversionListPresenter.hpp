#pragma once

#include <IBatchConversionListView.hpp>
#include <IConsoleOutput.hpp>

class BatchConversionListPresenter
{
public:
    BatchConversionListPresenter(IBatchConversionListView& view, IConsoleOutput& debug_visitor) : _view(view), _debug_visitor(debug_visitor)
    {
    }

    ~BatchConversionListPresenter()
    {
    }

    void Lock() const
    {
        _view.Lock();
    }

    void Unlock() const
    {
        _view.Unlock();
    }

    std::vector<std::string> GetCheckedPathsList() const
    {
        return _view.GetCheckedPathsList();
    }

protected:

    IBatchConversionListView& _view;
    IConsoleOutput& _debug_visitor;
};
