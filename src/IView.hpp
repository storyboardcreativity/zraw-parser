#pragma once

class IView
{
public:
    virtual ~IView() = default;

    virtual void Init() = 0;
};
