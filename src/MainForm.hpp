#pragma once

#include <nana/gui.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/panel.hpp>

#include <IFormView.hpp>


class MainForm
    : public nana::form, public IFormView
{
public:
    MainForm() : nana::form(nana::API::make_center(640, 480),
        nana::appearance{true, true, false, false, false, false, false}),
        current_user_control_(nullptr)
    {
        _init();
    }

    ~MainForm() = default;

protected:
    void _init()
    {
        place_.bind(*this);
        place_.div("margin=[5,5,5,5] gap=2 _field_");
        caption("ZRAW Video Converter");

        place_.collocate();
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
