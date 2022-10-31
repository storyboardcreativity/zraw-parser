#pragma once

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/filebox.hpp>

#include <IFileSelectionView.hpp>

#include "theme/conv_button.hpp"
#include "theme/conv_label.hpp"
#include "theme/conv_textbox.hpp"

#include "ViewThemeSingleton.hpp"

class FileSelectionUserControl : public nana::panel<true>, public IFileSelectionView
{
public:
    FileSelectionUserControl() : initialized_(false) {}

    FileSelectionUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
        : nana::panel<true>(wd, r, visible), initialized_(false)
    {
        this->Create(wd, r, visible);

        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &FileSelectionUserControl::OnThemeChanged);
    }

    ~FileSelectionUserControl()
    {
        ViewThemeSingleton::Instance().EventThemeChanged -= MakeDelegate(this, &FileSelectionUserControl::OnThemeChanged);
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
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[20,150,variable] field1>");
            // panel1
            panel1.create(*this);
            place_["field1"] << panel1;
            // panel2
            panel2.create(*this);
            panel2_place_.bind(panel2);
            panel2_place_.div("weight=130 margin=[5,5,5,5] <margin=[5,5,5,5] gap=2 arrange=[40,variable,40] field2>");
            place_["field1"] << panel2;
            // panel3
            panel3.create(panel2);
            panel2_place_["field2"] << panel3;
            // group1
            group1.create(panel2);
            group1.caption("<bold=true, color=0xF1F1F1>In/Out settings</>");
            group1.enable_format_caption(true);
            group1.div("gap=2 _field_");
            panel2_place_["field2"] << group1;
            // panel32
            panel32.create(group1);
            panel32_place_.bind(panel32);
            panel32_place_.div("<vert gap=2 arrange=[variable,variable,25] field3>");
            panel32.transparent(true);
            group1["_field_"] << panel32;

            // panel5
            panel5.create(panel32);
            panel5_place_.bind(panel5);
            panel5_place_.div("margin=[5,5,5,5] <margin=[5,5,5,5] gap=2 arrange=[120,variable,100] field5>");
            panel5.transparent(true);
            panel32_place_["field3"] << panel5;
            // label11
            label11.create(panel5);
            panel5_place_["field5"] << label11;
            label11.caption("Output path:");
            label11.transparent(true);
            label11.text_align(static_cast<nana::align>(1), static_cast<nana::align_v>(1));
            // OutputPathTextBox
            OutputPathTextBox.create(panel5);
            panel5_place_["field5"] << OutputPathTextBox;
            OutputPathTextBox.multi_lines(false);
            OutputPathTextBox.tip_string("enter path to the output folder or choose it by clicking a button");
            OutputPathTextBox.events().key_release([&](const nana::arg_keyboard& arg)
            {
                if (arg.key == nana::keyboard::enter)
                    TRIGGER_EVENT(EventOutputPathSelection, OutputPathTextBox.text());
            });
            OutputPathTextBox.events().focus([&](const nana::arg_focus& arg)
            {
                if (arg.getting == false)
                    TRIGGER_EVENT(EventOutputPathSelection, OutputPathTextBox.text());
            });
            // SelectOutputPathButton
            SelectOutputPathButton.create(panel5);
            panel5_place_["field5"] << SelectOutputPathButton;
            SelectOutputPathButton.caption("Select");
            SelectOutputPathButton.events().click([&]()
            {
                nana::folderbox fb(*this);
                fb.title("Chose Output Folder");
                fb.allow_multi_select(false);

                auto files = fb();
                if (files.size() == 1)
                {
                    TRIGGER_EVENT(EventOutputPathSelection, files[0].string());
                }
            });

            // _compressionModeComboxPanel

            _compressionModeComboxPanel.create(panel32);
            _compressionModeComboxPlace.bind(_compressionModeComboxPanel);
            _compressionModeComboxPlace.div("margin=[5,5,5,5] <margin=[5,5,5,5] gap=2 arrange=[120,150] _field_>");
            _compressionModeComboxPanel.transparent(true);
            panel32_place_["field3"] << _compressionModeComboxPanel;

            _compressionModeLabel.create(_compressionModeComboxPanel);
            _compressionModeComboxPlace["_field_"] << _compressionModeLabel;
            _compressionModeLabel.transparent(true);
            _compressionModeLabel.caption("DNG Compression:");

            _compressionModeCombox.create(_compressionModeComboxPanel);
            _compressionModeCombox.push_back("None");
            _compressionModeCombox.push_back("Lossless JPEG");
            _compressionModeCombox.events().selected([&](const arg_combox& arg)
            {
                auto cap = arg.widget.caption();
                if (cap == "None")
                {
                    TRIGGER_EVENT(EventCompressionModeSelection, None);
                }
                else if (cap == "Lossless JPEG")
                {
                    TRIGGER_EVENT(EventCompressionModeSelection, LosslessJPEG);
                }
            });
            _compressionModeCombox.option(1);
            _compressionModeComboxPlace["_field_"] << _compressionModeCombox;

            // panel6
            panel6.create(panel32);
            panel6_place_.bind(panel6);
            panel6_place_.div("weight=25 margin=[5,5,5,20] gap=2 arrange=[40,variable] _field_");
            panel6.transparent(true);
            panel32_place_["field3"] << panel6;

            // label2
            label2.create(panel6);
            panel6_place_["_field_"] << label2;
            label2.transparent(true);
            label2.caption("Status:");
            // label3
            label3.create(panel6);
            panel6_place_["_field_"] << label3;
            label3.transparent(true);
            label3.caption("EMPTY");
            label3.typeface(nana::paint::font{ FONT_NAME, FONT_SIZE });
            label3.typeface(nana::paint::font("", 9, { 1000, false, false, false }));
            // panel31
            panel31.create(panel2);
            panel2_place_["field2"] << panel31;
            // panel21
            panel21.create(*this);
            place_["field1"] << panel21;

            initialized_ = true;
        }

        place_.collocate();
        panel2_place_.collocate();
        group1.collocate();
        panel32_place_.collocate();
        panel5_place_.collocate();
        _compressionModeComboxPlace.collocate();

        OnThemeChanged(ViewThemeSingleton::Instance());
    }

    void SetSelectedOutputPathFieldText(std::string path) override
    {
        if (OutputPathTextBox.text() != path)
            OutputPathTextBox.reset(path);
    }

    void SetStatusText(std::string text, bool isOk) override
    {
        label3.caption(text);
        if (isOk)
            label3.fgcolor(COLOR9_NANA);
        else
            label3.fgcolor(COLOR8_NANA);
    }

    void SetActivity(bool isActive) override
    {
        OutputPathTextBox.enabled(isActive);
        SelectOutputPathButton.enabled(isActive);
    }

    void SetCompressionMode(CompressionMode_t mode) override
    {
    }

protected:
    nana::place place_;
    nana::panel<true> panel1;
    nana::panel<true> panel2;
    nana::place panel2_place_;
    nana::panel<true> panel3;
    nana::group group1;
    nana::panel<true> panel32;
    nana::place panel32_place_;
    nana::panel<true> panel5;
    nana::place panel5_place_;
    conv_label label11;
    conv_textbox OutputPathTextBox;
    conv_button SelectOutputPathButton;
    nana::panel<true> panel6;
    nana::place panel6_place_;
    conv_label label2;
    conv_label label3;
    nana::panel<true> panel31;
    nana::panel<true> panel21;

    nana::panel<true> _compressionModeComboxPanel;
    nana::place _compressionModeComboxPlace;
    conv_label _compressionModeLabel;
    nana::combox _compressionModeCombox;

    // ===

    void OnThemeChanged(IViewTheme& theme) const
    {
        const auto bgColor = theme.Background();
        this->scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel2.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel3.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        group1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel32.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel5.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        label11.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        OutputPathTextBox.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel6.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        label2.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        label3.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel31.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel21.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);

        const auto textColor = theme.TextStandard();
        label11.scheme().foreground = nana::color(textColor.r, textColor.g, textColor.b, textColor.alpha);
        label2.scheme().foreground = nana::color(textColor.r, textColor.g, textColor.b, textColor.alpha);
        label3.scheme().foreground = nana::color(textColor.r, textColor.g, textColor.b, textColor.alpha);

        const auto btnBgColor = theme.ButtonBackground();
        SelectOutputPathButton.scheme().background = nana::color(btnBgColor.r, btnBgColor.g, btnBgColor.b, btnBgColor.alpha);

        // Refresh control to apply changes
        nana::API::refresh_window(*this);
    }

    // ===

    bool initialized_;
    std::function<void(std::string)> file_selection_handler_;
};
