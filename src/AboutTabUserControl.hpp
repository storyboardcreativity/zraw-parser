#pragma once

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/system/dataexch.hpp>

#ifdef _MSC_VER
#include <Windows.h>
#else

#endif

#include "theme/conv_button.hpp"
#include "theme/conv_label.hpp"

#include "IUserControl.hpp"
#include "Version.hpp"

#include "ViewThemeSingleton.hpp"

class AboutTabUserControl : public nana::panel<true>, public IUserControl
{
public:
    AboutTabUserControl() : initialized_(false)
    {
        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &AboutTabUserControl::OnThemeChanged);
    }

    ~AboutTabUserControl()
    {
        ViewThemeSingleton::Instance().EventThemeChanged -= MakeDelegate(this, &AboutTabUserControl::OnThemeChanged);

        delete drawing_;
    }

private:

    void OnThemeChanged(IViewTheme& theme) const
    {
        const auto bgColor = theme.Background();
        this->scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel2.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel3.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel31.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel311.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel21.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel4.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel5.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        panel6.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        picture1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        label1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        label4.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);

        const auto btnBgColor = theme.ButtonBackground();
        //linkButton_.scheme().background = nana::color(btnBgColor.r, btnBgColor.g, btnBgColor.b, btnBgColor.alpha);

        // Refresh control to apply changes
        nana::API::refresh_window(*this);
    }

    void Init() override
    {
        if (!initialized_)
        {
            place_.bind(*this);

            // 146 is a vertical image control size
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[0,146,variable] field1>");

            // panel1
            panel1.create(*this);
            place_["field1"] << panel1;

            // panel2
            panel2.create(*this);
            panel2_place_.bind(panel2);

            // 488 is a horizontal image control size
            // 146 is a vertical image control size
            panel2_place_.div("weight=146 margin=[5,5,5,5] <margin=[0,0,0,0] gap=2 arrange=[variable,488,variable] field2>");

            place_["field1"] << panel2;
            // panel3
            panel3.create(panel2);
            panel2_place_["field2"] << panel3;
            // panel31
            panel31.create(panel2);
            panel31_place_.bind(panel31);
            panel31_place_.div("margin=[2,2,2,2] gap=2 _field_");
            panel2_place_["field2"] << panel31;
            // picture1
            picture1.create(panel31);
            panel31_place_["_field_"] << picture1;
            //img_.open(about_logo_bmp_v.data(), about_logo_bmp_v.size());
            img_.open(ABOUT_LOGO);
            picture1.load(img_);
            picture1.align(static_cast<nana::align>(0), static_cast<nana::align_v>(0));
            picture1.stretchable(true);

            // panel311
            panel311.create(panel2);
            panel2_place_["field2"] << panel311;
            // panel21
            panel21.create(*this);
            panel21_place_.bind(panel21);

            // 30 is a bold title vertical size
            panel21_place_.div("vert margin=[0,0,0,0] <vert margin=[0,0,0,0] gap=2 arrange=[30,variable] field3><margin=[0,0,0,0] btn_place>");

            place_["field1"] << panel21;
            // label1
            label1.create(panel21);
            panel21_place_["field3"] << label1;
            label1.caption(std::string("ZRAW Video Converter v") + std::string(ZRAW_PARSER_VERSION_STRING) + " by Zaripov R.");
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
            //label4.typeface(nana::paint::font("", 9, { 1000, false, false, false }));
            label4.caption("ZRAW Video Converter is a free, open-source solution that allows raw data extraction from ZRAW video files.\nIf you like it, please consider contributing to my open-source development efforts with a donation.");
            label4.text_align(static_cast<nana::align>(1), static_cast<nana::align_v>(1));
            // panel6
            panel6.create(panel4);
            panel4_place_["field4"] << panel6;

            drawing_ = new nana::drawing(panel31);
            drawing_->draw([&](nana::paint::graphics& graph)
            {
                unsigned int w = picture1.size().width + 4;
                unsigned int h = picture1.size().height + 4;

                //graph.rectangle(nana::rectangle{0, 0, w, h}, true, nana::color(255, 254, 250));

                //graph.line(nana::point(1, 1), nana::point(w - 2, 1), nana::color(116, 116, 116));
                //graph.line(nana::point(1, 1), nana::point(1, h - 2), nana::color(116, 116, 116));

                //graph.line(nana::point(0, 0), nana::point(w, 0), nana::color(166, 166, 166));
                //graph.line(nana::point(0, 0), nana::point(0, h), nana::color(166, 166, 166));

                graph.line(nana::point(w - 2, h - 2), nana::point(2, h - 2), COLOR2_NANA);
                //graph.line(nana::point(w - 2, h - 2), nana::point(w - 2, 2), nana::color(209, 208, 204));
            });

            _initButtonsPanel(panel21, panel21_place_);

            initialized_ = true;
        }

        drawing_->update();
        place_.collocate();
        panel2_place_.collocate();
        panel31_place_.collocate();
        panel21_place_.collocate();
        panel4_place_.collocate();

        OnThemeChanged(ViewThemeSingleton::Instance());
    }

#define _BTC_ADDRESS "bc1q6pxsepxekzhfecuw0wshmwx4wqwdnlppefwvff"
#define _ETH_ADDRESS "0x90261E953c1E5Bfd6C54A0a725172b156dDeFBD3"
#define _MIR_ADDRESS "2200 1502 3712 4620"

    void _initButtonsPanel(nana::panel<true>& parent, nana::place& place)
    {
        // Create panel
        _buttons_panel.create(parent);
        place["btn_place"] << _buttons_panel;

        // Attach field to panel
        _buttons_place.bind(_buttons_panel);
        _buttons_place.div("<vert margin=[0,100,0,100] btn_field>");

        _btn_btc.create(_buttons_panel);
        _buttons_place["btn_field"] << _btn_btc;
        _btn_btc.caption(std::string("BTC: ") + _BTC_ADDRESS);
        _btn_btc.events().click([&]()
        {
            nana::system::dataexch().set(_BTC_ADDRESS, api::root(*this));

            _btn_btc.caption(std::string("BTC: ") + _BTC_ADDRESS + " (copied to clipboard)");
            _btn_eth.caption(std::string("ETH: ") + _ETH_ADDRESS);
            _btn_mir.caption(std::string("MIR: ") + _MIR_ADDRESS);
        });

        _btn_eth.create(_buttons_panel);
        _buttons_place["btn_field"] << _btn_eth;
        _btn_eth.caption(std::string("ETH: ") + _ETH_ADDRESS);
        _btn_eth.events().click([&]()
        {
            nana::system::dataexch().set(_ETH_ADDRESS, api::root(*this));

            _btn_btc.caption(std::string("BTC: ") + _BTC_ADDRESS);
            _btn_eth.caption(std::string("ETH: ") + _ETH_ADDRESS + " (copied to clipboard)");
            _btn_mir.caption(std::string("MIR: ") + _MIR_ADDRESS);
        });
        
        _btn_mir.create(_buttons_panel);
        _buttons_place["btn_field"] << _btn_mir;
        _btn_mir.caption(std::string("MIR: ") + _MIR_ADDRESS);
        _btn_mir.events().click([&]()
        {
            nana::system::dataexch().set(_MIR_ADDRESS, api::root(*this));

            _btn_btc.caption(std::string("BTC: ") + _BTC_ADDRESS);
            _btn_eth.caption(std::string("ETH: ") + _ETH_ADDRESS);
            _btn_mir.caption(std::string("MIR: ") + _MIR_ADDRESS + " (copied to clipboard)");
        });

        _buttons_place.collocate();
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
    conv_label label1;
    nana::panel<true> panel4;
    nana::place panel4_place_;
    nana::panel<true> panel5;
    conv_label label4;
    nana::panel<true> panel6;
    //conv_button linkButton_;
    conv_textbox _support_textbox;

    nana::panel<true> _buttons_panel;
    nana::place _buttons_place;

    // buttons
    conv_button _btn_btc;
    conv_button _btn_eth;
    conv_button _btn_mir;

    nana::drawing* drawing_;

    // ===

    bool initialized_;
    nana::paint::image img_;
};
