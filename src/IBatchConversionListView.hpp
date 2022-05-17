#pragma once

#include <string>
#include <vector>

#include <Event.h>
#include <IUserControl.hpp>

class IBatchConversionListView : public IUserControl
{
public:
    DECLARE_EVENT(void, std::vector<std::string>&) EventAddFilesButtonClick;

    virtual void Lock() = 0;
    virtual void Unlock() = 0;

    virtual std::vector<std::string> GetCheckedPathsList() = 0;
};
