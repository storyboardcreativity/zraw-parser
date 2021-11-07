#pragma once

#include "IUserControl.hpp"
#include "IConsoleOutput.hpp"

class IConsoleView : public IUserControl
{
public:
    virtual IConsoleOutput& Console() = 0;
};
