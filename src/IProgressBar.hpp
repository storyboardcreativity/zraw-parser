#pragma once

#include <string>

class IProgressBar
{
public:
    virtual ~IProgressBar() = default;

    // Sets current percent
    virtual void ChangePercent(unsigned int percent) = 0;

    // Sets current step description
    virtual void SetDescription(std::string format, ...) = 0;
};
