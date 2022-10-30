#pragma once

#include <iostream>
#include <nana/gui.hpp>
#include <nana/gui/screen.hpp>
#include <nana/gui/dragger.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/picture.hpp>

#include "conv_common.hpp"

#include "ViewThemeSingleton.hpp"

#define    BUTTON_SZ_X    34
#define    BUTTON_SZ_Y    26
#define WINDOW_MAIN_PIC_SIZE_X (90 / 2)
#define WINDOW_MAIN_PIC_SIZE_Y (64 / 2)

using namespace nana;

/// Create an appearance of a window without "decoration" with no titlebar and no 3D-look borders.
template < typename Floating = null_type,
    typename NoActive = null_type,
    typename Minimize = null_type,
    typename Maximize = null_type,
    typename Sizable = null_type>
    struct bald0
{
    typedef meta::fixed_type_set<Floating, NoActive, Minimize, Maximize, Sizable> set_type;

    operator appearance() const
    {
        return appearance(false,
            true,    // Taskbar
            set_type::template count<nana::appear::floating>::value,
            set_type::template count<nana::appear::no_activate>::value,
            set_type::template count<nana::appear::minimize>::value,
            set_type::template count<nana::appear::maximize>::value,
            false    // Sizeable
        );
    }
};

class conv_form : public form
{
public:
    conv_form() : form(API::make_center(920, 600), bald0()),//appear::optional<false, appear::sizable, appear::taskbar, appear::floating, appear::minimize>()),
        f{ FONT_NAME, FONT_SIZE }
    {
        bgcolor(BLACK);
        typeface(f);

        w = this->size().width;
        h = this->size().height;

        // dragBar
        dragBar.create(*this, rectangle(WINDOW_MAIN_PIC_SIZE_X + 14, 4, w - 3 * BUTTON_SZ_X - WINDOW_MAIN_PIC_SIZE_X, WINDOW_MAIN_PIC_SIZE_Y));
        auto c = ViewThemeSingleton::Instance().Background();
        dragBar.bgcolor(nana::color(c.r, c.g, c.b, c.alpha));
        dragBar.typeface(f);
        dragBar.text_align(align::left, align_v::center);
        dragBar.fgcolor(COLOR6_NANA);
        dragBar.events().mouse_enter([&](const arg_mouse& mp) {conv_form::OnMouseEnter(mp); });
        dragBar.events().mouse_leave([&](const arg_mouse& mp) {conv_form::OnMouseLeave(mp); });
        dragBar.events().dbl_click([&](const arg_mouse& mp) {conv_form::maximizeGadgetOnClicked(); });

        // minimizeGadget
        minimizeGadget.create(*this, rectangle(w - BUTTON_SZ_X * 3 - 1, 1, BUTTON_SZ_X, BUTTON_SZ_Y));
        minimizeGadget.set_bground(getBground(0));
        minimizeGadget.enable_focus_color(false);
        minimizeGadget.edge_effects(false);

        minimizeGadget.events().click([this]() {conv_form::minimizeGadgetOnClicked(); });
        minimizeGadget.events().mouse_enter([&](const arg_mouse& mp) {conv_form::OnMouseEnter(mp); });
        minimizeGadget.events().mouse_leave([&](const arg_mouse& mp) {conv_form::OnMouseLeave(mp); });

        // closeGadget
        closeGadget.create(*this, rectangle(w - BUTTON_SZ_X - 1, 1, BUTTON_SZ_X, BUTTON_SZ_Y));
        closeGadget.set_bground(getBground(2));
        closeGadget.enable_focus_color(false);
        closeGadget.edge_effects(false);

        closeGadget.events().click([this]() {conv_form::closeGadgetOnClicked(); });
        closeGadget.events().mouse_enter([&](const arg_mouse& mp) {conv_form::OnMouseEnter(mp); });
        closeGadget.events().mouse_leave([&](const arg_mouse& mp) {conv_form::OnMouseLeave(mp); });

        // maximizeGadget
        maximizeGadget.create(*this, rectangle(w - BUTTON_SZ_X * 2 - 1, 1, BUTTON_SZ_X, BUTTON_SZ_Y));
        maximizeGadget.set_bground(getBground(1));
        maximizeGadget.enable_focus_color(false);
        maximizeGadget.edge_effects(false);

        maximizeGadget.events().click([this]() {conv_form::maximizeGadgetOnClicked(); });
        maximizeGadget.events().mouse_enter([&](const arg_mouse& mp) {conv_form::OnMouseEnter(mp); });
        maximizeGadget.events().mouse_leave([&](const arg_mouse& mp) {conv_form::OnMouseLeave(mp); });

        // form events
        events().resized([this]() {conv_form::OnResized(); });
        events().mouse_enter([&](const arg_mouse& mp) {conv_form::OnMouseEnter(mp); });
        events().mouse_leave([&](const arg_mouse& mp) {conv_form::OnMouseLeave(mp); });

        auto icon_data = g_res_icon.Data();
        paint::image icon_img;
        icon_img.open(icon_data.ptr, icon_data.size_bytes);

        _pic.create(*this, rectangle(4, 4, WINDOW_MAIN_PIC_SIZE_X, WINDOW_MAIN_PIC_SIZE_Y));
        _pic.load(icon_img);
        _pic.stretchable(true);
        _pic.events().dbl_click([&](const arg_mouse& mp) {conv_form::maximizeGadgetOnClicked(); });

        // paint
        OnPaint();
        is_zoomed = false;

        _dg.trigger(dragBar);
        _dg.trigger(_pic);
        _dg.target(*this);
    }
    void setTitle(std::string title)
    {
        dragBar.caption(title);
        drawing dw(dragBar);
        dw.clear();
        dw.draw([&](paint::graphics& g)
        {
            w = g.size().width;
            h = g.size().height;

        });
        dw.update();
    }
    void setActive()
    {
        setTitle(dragBar.caption());
    }
    void setPreventResize()
    {
        minimizeGadget.enabled(false);
    }
    void setPreventZoom()
    {
        maximizeGadget.enabled(false);
    }
    label dragBar;
    void setFont(paint::font &ft) { f = ft; }
private:
    paint::font f;

    button minimizeGadget;
    button closeGadget;
    button maximizeGadget;

    void OnResized()
    {
        w = this->size().width;
        h = this->size().height;
        closeGadget.move(rectangle(w - BUTTON_SZ_X - 1, 1, BUTTON_SZ_X, BUTTON_SZ_Y));
        maximizeGadget.move(rectangle(w - BUTTON_SZ_X * 2 - 1, 1, BUTTON_SZ_X, BUTTON_SZ_Y));
        minimizeGadget.move(rectangle(w - BUTTON_SZ_X * 3 - 1, 1, BUTTON_SZ_X, BUTTON_SZ_Y));
        dragBar.size(nana::size(w - 3 * BUTTON_SZ_X - WINDOW_MAIN_PIC_SIZE_X, WINDOW_MAIN_PIC_SIZE_Y));
        OnPaint();
    }
    void OnPaint()
    {
        drawing dw(*this);
        dw.clear();
        dw.draw([&](paint::graphics& g)
        {
            w = g.size().width;
            h = g.size().height;

            // top info rect
            g.rectangle(rectangle{ 0, 0, w, h }, true, COLOR0);

            g.frame_rectangle(rectangle{ g.size() }, COLOR2_NANA, COLOR2_NANA, COLOR2_NANA, COLOR2_NANA);
            g.frame_rectangle(rectangle{ g.size() }, COLOR3_NANA, COLOR3_NANA, COLOR3_NANA, COLOR3_NANA);
        });
        dw.update();
    }
    void OnMouseEnter(const arg_mouse& e)
    {
        setTitle(dragBar.caption());
    }
    void OnMouseLeave(const arg_mouse& e)
    {
        setTitle(dragBar.caption());
    }

    void closeGadgetOnClicked()
    {
        close();
    }
    void maximizeGadgetOnClicked()
    {
        if (!is_zoomed)
            nana::API::zoom_window(*this, true);
        else
            API::restore_window(*this);

        is_zoomed = !is_zoomed;
    }
    void minimizeGadgetOnClicked()
    {
        nana::API::zoom_window(*this, false);
    }

    // Button image
    // index:value
    // -----------
    // 0:minimize
    // 1:maximize
    // 2:close

    element::bground getBground(unsigned char index)
    {
        element::bground bg;
        bg.states({ element_state::normal, element_state::hovered, element_state::pressed });
        bg.join(element_state::normal, element_state::focus_normal);
        bg.join(element_state::hovered, element_state::focus_hovered);
        bg.join(element_state::normal, element_state::disabled);

        auto vsbuttons_data = g_res_vsbuttons.Data();
        paint::image img;
        img.open(vsbuttons_data.ptr, vsbuttons_data.size_bytes);

        bg.image(img, true, rectangle{ index * BUTTON_SZ_X, 0, BUTTON_SZ_X, BUTTON_SZ_Y * 3 });
        return bg;
    }
    bool is_zoomed;
    unsigned int w, h;

    nana::dragger _dg;
    nana::picture _pic;
};
