#pragma once

#include "IView.hpp"

class IFormView
{
public:
    virtual ~IFormView() = default;

    virtual void UserControl__set(IView& userControl) = 0;
    virtual void Show() = 0;
};
