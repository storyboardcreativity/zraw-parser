#pragma once

#include <string>

#include <IUserControl.hpp>
#include <IProgressBar.hpp>

#include <Event.h>

class IMainFormPanelView : public IUserControl
{
public:
    virtual IProgressBar& ProgressBar() = 0;

    // Adds UserControl as a tab with a title
    virtual void AddTab(std::string title, IUserControl& userControl) = 0;

    // Changes text on the button near the progress bar
    virtual void ChangeProcessButtonText(std::string text) = 0;

    // Enables/disables the button near the progress bar
    virtual void ChangeProcessButtonActivity(bool isActive) = 0;

    // Triggers on the button click near the progress bar
    DECLARE_EVENT(void) EventProcessButtonClick;
};
