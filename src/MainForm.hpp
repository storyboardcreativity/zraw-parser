#pragma once

#include <nana/gui.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/panel.hpp>

#include <IFormView.hpp>

#include "ViewThemeSingleton.hpp"

#include "theme/conv_form.hpp"

class MainForm : public conv_form, public IFormView
{
public:
    MainForm() : current_user_control_(nullptr)
    {
        _init();

        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &MainForm::OnThemeChanged);
    }

    ~MainForm()
    {
        ViewThemeSingleton::Instance().EventThemeChanged -= MakeDelegate(this, &MainForm::OnThemeChanged);
    }

protected:
    void _init()
    {
        place_.bind(*this);
        place_.div("margin=[42,2,2,2] gap=2 _field_");
        setTitle("ZRAW Video Converter");

        place_.collocate();

        OnThemeChanged(ViewThemeSingleton::Instance());
    }

    void OnThemeChanged(IViewTheme& theme) const
    {
        const auto bgColor = theme.Background();
        this->scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);

        // Refresh control to apply changes
        nana::API::refresh_window(*this);
    }

public:
    void UserControl__set(IView& userControl) override
    {
        // Detach previous UserControl if exists
        if (current_user_control_ != nullptr)
            place_.erase(dynamic_cast<nana::panel<true>&>(*current_user_control_));

        // Save new UserControl
        current_user_control_ = &userControl;

        // Prepare new UserControl
        auto& panel = dynamic_cast<nana::panel<true>&>(userControl);
        panel.create(*this);
        userControl.Init();

        // Attach new UserControl
        place_["_field_"] << panel;
        place_.collocate();
    }

    void Show() override
    {
        show();

        // Here we fall into inf. loop
        nana::exec();
    }
protected:
    nana::place place_;

    // ===

    IView* current_user_control_;
};
