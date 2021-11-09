#pragma once

#include <MainFormPresenter.hpp>

class MainFormPresenterFactory
{
public:
    static std::unique_ptr<MainFormPresenter> Create()
    {
        return std::make_unique<MainFormPresenter>(std::make_unique<MainForm>());
    }
};
