#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>

#include "conv_common.hpp"

using namespace nana;

class conv_button : public button
{
public:
    conv_button() : button(), f{ FONT_SIZE, FONT } {}

    conv_button(nana::window& af, std::string text, const rectangle& r) : button(af, r), f{ FONT_SIZE, FONT }
    {
        caption(text);
        Set();
    }

    void setFont(paint::font &ft) { f = ft; }
private:
    paint::font f;
    bool _pressed;
    bool _mouse_on;
    void OnMouseDown(const arg_mouse& e)
    {
        _pressed = true;
        OnPaint();
    }
    void OnMouseUp(const arg_mouse& e)
    {
        _pressed = false;
        OnPaint();
    }
    void OnMouseEnter(const arg_mouse& e)
    {
        _mouse_on = true;
        OnPaint();
    }
    void OnMouseLeave(const arg_mouse& e)
    {
        _mouse_on = false;
        OnPaint();
    }

    void OnPaint()
    {
        drawing dw(*this);
        dw.clear();
        dw.draw([this](paint::graphics& g)
        {
            unsigned int w = g.size().width;
            unsigned int h = g.size().height;
            unsigned int p = (_pressed) ? 1 : 0;
            nana::size captionSize = g.bidi_extent_size(caption());

            g.rectangle(rectangle{ 0, 0, w, h }, true, COLOR4_NANA);

            if (_mouse_on && enabled())
                g.rectangle(rectangle{ 0, 0, w, h }, true, COLOR3_NANA);

            if (_pressed && enabled())
                g.rectangle(rectangle{ 0, 0, w, h }, true, COLOR5_NANA);

            if (!enabled())
                g.rectangle(rectangle{ 0, 0, w, h }, true, COLOR4_NANA);

            g.string({ static_cast <int>((w - captionSize.width) / 2 + p), static_cast <int>((h - captionSize.height) / 2 + p) }, caption(), enabled() ? scheme().foreground : COLOR2_NANA);

        });
        dw.update();
    }
    void Set()
    {
        typeface(f);
        enable_focus_color(false);
        edge_effects(false);
        fgcolor(COLOR1_NANA);
        OnPaint();
        events().mouse_down([&](const arg_mouse& mp) {conv_button::OnMouseDown(mp); });
        events().mouse_up([&](const arg_mouse& mp) {conv_button::OnMouseUp(mp); });
        events().mouse_enter([&](const arg_mouse& mp) {conv_button::OnMouseEnter(mp); });
        events().mouse_leave([&](const arg_mouse& mp) {conv_button::OnMouseLeave(mp); });
        _pressed = false;
        _mouse_on = false;
    }

    void _m_complete_creation() override
    {
        Set();
    }
};
