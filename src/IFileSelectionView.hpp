#pragma once

#include <string>

#include <Event.h>
#include <IUserControl.hpp>

class IFileSelectionView : public IUserControl
{
public:
    DECLARE_EVENT(void, std::string path) EventOutputPathSelection;

    virtual void SetSelectedOutputPathFieldText(std::string path) = 0;

    virtual void SetStatusText(std::string text, bool isOk) = 0;

    virtual void SetActivity(bool isActive) = 0;
};
