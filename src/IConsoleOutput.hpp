#pragma once

class IConsoleOutput
{
public:
    virtual void printf(const char *format, ...) = 0;
};