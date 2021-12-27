#pragma once

#include <string>

#include "IUserControl.hpp"

class IInputFileInfoView : public IUserControl
{
public:
    class ICategory
    {
    public:
        virtual ~ICategory() = default;

        virtual void SetProperty(std::string name, std::string format, ...) = 0;

        virtual void Lock() = 0;

        virtual void Unlock() = 0;
    };

    virtual ICategory& CreateCategory(std::string name) = 0;

    virtual void Clear() = 0;
};
