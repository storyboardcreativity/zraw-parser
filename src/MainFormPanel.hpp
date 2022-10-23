#pragma once

#include <cstdarg>

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>

#include <IMainFormPanelView.hpp>

#include "theme/conv_button.hpp"
#include "theme/conv_label.hpp"
#include "theme/conv_tabbar.hpp"
#include "theme/conv_progress.hpp"

#include "ViewThemeSingleton.hpp"

class MainFormPanel : public nana::panel<true>, public IMainFormPanelView
{
public:
    MainFormPanel() : initialized_(false), current_user_control_(nullptr),
        _progressBarInterface(ProgressBarInterface(*this)) {}

    MainFormPanel(nana::window wd, const nana::rectangle& r = {}, bool visible = true): nana::panel<true>(wd, r, visible),
        initialized_(false), current_user_control_(nullptr),
        _progressBarInterface(ProgressBarInterface(*this))
    {
        this->Create(wd, r, visible);

        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &MainFormPanel::OnThemeChanged);
    }

    ~MainFormPanel()
    {
        ViewThemeSingleton::Instance().EventThemeChanged -= MakeDelegate(this, &MainFormPanel::OnThemeChanged);
    }

    bool Create(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
    {
        if(!nana::panel<true>::create(wd, r, visible))
            return false;

        Init();

        return true;
    }

    void Init() override
    {
        if (!initialized_)
        {
            place_.bind(*this);
            place_.div("margin=[5,5,5,5] <vert <vert <weight=25 gap=2 field4><margin=[5,5,5,5] gap=2 field41>><weight=60 margin=[5,5,5,5] gap=2 arrange=[variable,100] field3>>");
            // TabBar
            TabBar.create(*this);
            place_["field4"] << TabBar;
            // _panel1
            _panel1.create(*this);
            _panel1_place_.bind(_panel1);
            _panel1_place_.div("vert margin=[0,5,0,5] gap=2 _field_");
            place_["field3"] << _panel1;
            // _panel2
            _panel2.create(_panel1);
            _panel2_place_.bind(_panel2);
            _panel2_place_.div("margin=[5,5,5,5] gap=2 arrange=[30,variable,30] _field_");
            _panel1_place_["_field_"] << _panel2;
            // _label1
            _label1.create(_panel2);
            _panel2_place_["_field_"] << _label1;
            _label1.caption("Now:");
            // ProgressDescriptionLabel
            ProgressDescriptionLabel.create(_panel2);
            _panel2_place_["_field_"] << ProgressDescriptionLabel;
            ProgressDescriptionLabel.caption("Idle.");
            // PercentLabel
            PercentLabel.create(_panel2);
            _panel2_place_["_field_"] << PercentLabel;
            PercentLabel.caption("100%");
            PercentLabel.text_align(static_cast<nana::align>(2), static_cast<nana::align_v>(0));
            // ProgressBar
            ProgressBarControl.create(_panel1);
            _panel1_place_["_field_"] << ProgressBarControl;
            ProgressBarControl.value(0);
            // ProcessButton
            ProcessButton.create(*this);
            place_["field3"] << ProcessButton;
            ProcessButton.caption("Cancel");
            ProcessButton.events().click([&]()
            {
                TRIGGER_EVENT(EventProcessButtonClick);
            });

            initialized_ = true;
        }

        place_.collocate();
        _panel1_place_.collocate();
        _panel2_place_.collocate();

        OnThemeChanged(ViewThemeSingleton::Instance());
    }

    void AddTab(std::string title, IUserControl& userControl) override
    {
        // Prepare new UserControl
        auto& panel = dynamic_cast<nana::panel<true>&>(userControl);
        panel.create(*this);
        userControl.Init();

        TabBar.append(title, panel);
        place_.field("field41").fasten(panel);
        place_.collocate();
    }

    IProgressBar& ProgressBar() override
    {
        return _progressBarInterface;
    }

    void ChangeProcessButtonText(std::string text) override
    {
        ProcessButton.caption(text);
    }

    void ChangeProcessButtonActivity(bool isActive) override
    {
        ProcessButton.enabled(isActive);
    }

protected:
    nana::place place_;
    conv_tabbar TabBar;
    nana::panel<true> _panel1;
    nana::place _panel1_place_;
    nana::panel<true> _panel2;
    nana::place _panel2_place_;
    conv_label _label1;
    conv_label ProgressDescriptionLabel;
    conv_label PercentLabel;
    conv_progress ProgressBarControl;
    conv_button ProcessButton;

    // ===

    void OnThemeChanged(IViewTheme& theme) const
    {
        const auto bgColor = theme.Background();
        this->scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        TabBar.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _panel1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _panel2.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _label1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        ProgressDescriptionLabel.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        PercentLabel.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        ProgressBarControl.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);

        const auto btnBgColor = theme.ButtonBackground();
        ProcessButton.scheme().background = nana::color(btnBgColor.r, btnBgColor.g, btnBgColor.b, btnBgColor.alpha);

        // Refresh control to apply changes
        nana::API::refresh_window(*this);
    }

    // ===

    class ProgressBarInterface : public IProgressBar
    {
    public:
        ProgressBarInterface(MainFormPanel& control) : _control(control) {}

        void ChangePercent(unsigned int percent) override
        {
            if (_control.ProgressBarControl.value() == percent)
                return;

            _control.ProgressBarControl.value(percent);
            _control.PercentLabel.caption(std::to_string(percent) + "%");

            TRIGGER_EVENT(EventPercentUpdate, this, percent);
        }

        void SetDescription(std::string format, ...) override
        {
            // 1. Calculate buffer length
            va_list args;
            va_start(args, format);
            auto bufsz = vsnprintf(NULL, 0, format.c_str(), args);
            va_end(args);

            // 2. Create buffer
            char *buffer = new char[bufsz + 1];

            // 3. Print to buffer
            va_start(args, format);
            vsprintf(buffer, format.c_str(), args);
            va_end(args);

            // 4. Set caption
            const auto str = std::string(buffer);
            _control.ProgressDescriptionLabel.caption(str);
            TRIGGER_EVENT(EventDescriptionUpdate, this, str);

            // 5. Remove buffer
            delete[] buffer;
        }

    private:
        MainFormPanel& _control;
    };

    bool initialized_;
    IUserControl* current_user_control_;
    ProgressBarInterface _progressBarInterface;
};
