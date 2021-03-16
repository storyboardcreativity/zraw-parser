#pragma once

class IConsoleOutput
{
public:
    virtual void printf(char *format, ...) = 0;
};