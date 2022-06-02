#pragma once

#include <string>

#include <Event.h>

class IProgressBar
{
public:
    // Triggered on each percent change
    DECLARE_EVENT(void, IProgressBar*, unsigned int) EventPercentUpdate;

    // Triggered on each description change
    DECLARE_EVENT(void, IProgressBar*, std::string) EventDescriptionUpdate;

    virtual ~IProgressBar() = default;

    // Sets current percent
    virtual void ChangePercent(unsigned int percent) = 0;

    // Sets current step description
    virtual void SetDescription(std::string format, ...) = 0;
};
