#pragma once

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/filebox.hpp>

#include <IFileSelectionView.hpp>

class FileSelectionUserControl : public nana::panel<true>, public IFileSelectionView
{
public:
    FileSelectionUserControl() : initialized_(false) {}

    FileSelectionUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
        : nana::panel<true>(wd, r, visible), initialized_(false)
    {
        this->Create(wd, r, visible);
    }

    ~FileSelectionUserControl() = default;

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
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[20,130,variable] field1>");
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
            group1.caption("In/Out settings");
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
            panel5_place_.div("margin=[5,5,5,5] <margin=[5,5,5,5] gap=2 arrange=[100,variable,100] field5>");
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
    }

    void SetSelectedOutputPathFieldText(std::string path) override
    {
        if (OutputPathTextBox.text() != path)
            OutputPathTextBox.reset(path);
    }

    void SetStatusText(std::string text, bool isOk) override
    {
        label3.caption(text);
        label3.fgcolor(nana::color(isOk ? 0 : 100, isOk ? 100 : 0, 0));
    }

    void SetActivity(bool isActive) override
    {
        OutputPathTextBox.enabled(isActive);
        SelectOutputPathButton.enabled(isActive);
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
    nana::label label11;
    nana::textbox OutputPathTextBox;
    nana::button SelectOutputPathButton;
    nana::panel<true> panel6;
    nana::place panel6_place_;
    nana::label label2;
    nana::label label3;
    nana::panel<true> panel31;
    nana::panel<true> panel21;

    // ===

    bool initialized_;
    std::function<void(std::string)> file_selection_handler_;
};
