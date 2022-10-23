#pragma once

#include <Event.h>

class IViewTheme
{
public:
    DECLARE_EVENT(void, IViewTheme&) EventThemeChanged;

    virtual ~IViewTheme() = default;

    typedef struct
    {
        unsigned int r;
        unsigned int g;
        unsigned int b;
        double alpha;
    } color_t;

    virtual color_t ButtonBackground() = 0;

    virtual color_t Background() = 0;

    virtual color_t TextStandard() = 0;
};

class ViewThemeSingleton : public IViewTheme
{
    ViewThemeSingleton() = default;
public:
    static IViewTheme& Instance()
    {
        static IViewTheme* localInstance = nullptr;

        if (localInstance == nullptr)
            localInstance = new ViewThemeSingleton();
        return *localInstance;
    }

    color_t ButtonBackground() override
    {
        return {100, 100, 100, 1.0};
    }

    color_t Background() override
    {
        return { 45, 45, 48, 1.0 };
    }

    color_t TextStandard() override
    {
        return { 241, 241, 241, 1.0 };
    }
};