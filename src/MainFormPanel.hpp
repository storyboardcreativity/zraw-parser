#pragma once

#include <cstdarg>

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>

#include <IMainFormPanelView.hpp>


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
    }

    ~MainFormPanel() = default;

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
    nana::tabbar<size_t> TabBar;
    nana::panel<true> _panel1;
    nana::place _panel1_place_;
    nana::panel<true> _panel2;
    nana::place _panel2_place_;
    nana::label _label1;
    nana::label ProgressDescriptionLabel;
    nana::label PercentLabel;
    nana::progress ProgressBarControl;
    nana::button ProcessButton;

    // ===

    class ProgressBarInterface : public IProgressBar
    {
    public:
        ProgressBarInterface(MainFormPanel& control) : _control(control) {}

        void ChangePercent(unsigned int percent) override
        {
            _control.ProgressBarControl.value(percent);
            _control.PercentLabel.caption(std::to_string(percent) + "%");
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
            _control.ProgressDescriptionLabel.caption(std::string(buffer));

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