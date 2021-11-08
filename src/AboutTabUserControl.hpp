#pragma once

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/label.hpp>

#ifdef _MSC_VER
#include <Windows.h>
#else

#endif

#include "IUserControl.hpp"
#include "Version.hpp"

class AboutTabUserControl : public nana::panel<true>, public IUserControl
{
public:
    AboutTabUserControl() : initialized_(false) {}

    AboutTabUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
        : nana::panel<true>(wd, r, visible), initialized_(false)
    {
        this->create(wd, r, visible);
    }

    ~AboutTabUserControl() = default;

    bool create(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
    {
        if(!nana::panel<true>::create(wd, r, visible))
            return false;

        Init();

        return true;
    }

private:

    void Init() override
    {
        if (!initialized_)
        {
            place_.bind(*this);
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[10,160,variable] field1>");
            // panel1
            panel1.create(*this);
            place_["field1"] << panel1;
            // panel2
            panel2.create(*this);
            panel2_place_.bind(panel2);
            panel2_place_.div("weight=146 margin=[5,5,5,5] <margin=[0,0,0,0] gap=2 arrange=[9%,variable,9%] field2>");
            place_["field1"] << panel2;
            // panel3
            panel3.create(panel2);
            panel2_place_["field2"] << panel3;
            // panel31
            panel31.create(panel2);
            panel31_place_.bind(panel31);
            panel31_place_.div("margin=[0,0,0,0] gap=2 _field_");
            panel2_place_["field2"] << panel31;
            // picture1
            picture1.create(panel31);
            panel31_place_["_field_"] << picture1;
            picture1.load(nana::paint::image("res/about_logo.bmp"));
            picture1.align(static_cast<nana::align>(0), static_cast<nana::align_v>(0));
            picture1.stretchable(true);

            // panel311
            panel311.create(panel2);
            panel2_place_["field2"] << panel311;
            // panel21
            panel21.create(*this);
            panel21_place_.bind(panel21);
            panel21_place_.div("margin=[0,0,0,0] <vert margin=[0,0,0,0] gap=2 arrange=[30,variable,variable] field3>");
            place_["field1"] << panel21;
            // label1
            label1.create(panel21);
            panel21_place_["field3"] << label1;
            label1.caption(std::string("ZRAW Parser for Windows v") + std::string(ZRAW_PARSER_VERSION_STRING) + "\nby Zaripov R.");
            label1.text_align(static_cast<nana::align>(1), static_cast<nana::align_v>(1));
            // panel4
            panel4.create(panel21);
            panel4_place_.bind(panel4);
            panel4_place_.div("margin=[5,5,5,5] <margin=[5,5,5,5] gap=2 arrange=[50,variable,50] field4>");
            panel21_place_["field3"] << panel4;
            // panel5
            panel5.create(panel4);
            panel4_place_["field4"] << panel5;
            // textbox1
            label4.create(panel4);
            panel4_place_["field4"] << label4;
            label4.typeface(nana::paint::font("", 9, { 1000, false, false, false }));
            label4.caption("ZRAW Parser is a free, open-source solution that allows raw data extraction from ZRAW video files. If you like it, please consider contributing to my open-source development efforts with a donation.");
            label4.text_align(static_cast<nana::align>(1), static_cast<nana::align_v>(1));
            // panel6
            panel6.create(panel4);
            panel4_place_["field4"] << panel6;
            // label3
            linkButton_.create(panel21);
            panel21_place_["field3"] << linkButton_;
            linkButton_.caption("Click here to support ZRAW tools development");
            linkButton_.events().click([]()
            {
#ifdef _MSC_VER
                ShellExecute(NULL, L"open", L"https://ko-fi.com/storyboardcreativity", NULL, NULL, SW_SHOWNORMAL);
#else
                system("xdg-open https://ko-fi.com/storyboardcreativity");
#endif
                
            });

            initialized_ = true;
        }

        place_.collocate();
        panel2_place_.collocate();
        panel31_place_.collocate();
        panel21_place_.collocate();
        panel4_place_.collocate();
    }

protected:
    nana::place place_;
    nana::panel<true> panel1;
    nana::panel<true> panel2;
    nana::place panel2_place_;
    nana::panel<true> panel3;
    nana::panel<true> panel31;
    nana::place panel31_place_;
    nana::picture picture1;
    nana::panel<true> panel311;
    nana::panel<true> panel21;
    nana::place panel21_place_;
    nana::label label1;
    nana::panel<true> panel4;
    nana::place panel4_place_;
    nana::panel<true> panel5;
    nana::label label4;
    nana::panel<true> panel6;
    nana::button linkButton_;

    // ===

    bool initialized_;
};
