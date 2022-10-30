#pragma once

#include "conv_common.hpp"

#include <nana/gui.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/panel.hpp>

class conv_textbox : public nana::panel<true>
{
public:
    conv_textbox() : panel<true>(), f{ FONT_NAME, FONT_SIZE } {}

    void setFont(nana::paint::font &ft) { f = ft; }
    std::string caption()
    {
        return tbox.caption();
    }
    void caption(std::string text)
    {
        tbox.caption(text);
    }
    void tip_string(std::string text)
    {
        tbox.tip_string(text);
    }

    void reset(std::string text)
    {
        tbox.reset(text);
    }
    std::string text()
    {
        return tbox.text();
    }

    void multi_lines(bool is_enabled)
    {
        tbox.multi_lines(is_enabled);
    }

    bool editable() const
    {
        return tbox.editable();
    }
    void editable(const bool enable)
    {
        tbox.editable(enable);
    }

    bool enabled() const
    {
        return tbox.enabled();
    }
    void enabled(const bool enable)
    {
        tbox.enabled(enable);
    }

private:
    nana::paint::font f;
    nana::textbox tbox;
    void OnPaint()
    {
        nana::drawing dw(*this);
        dw.clear();
        dw.draw([this](nana::paint::graphics& g)
        {
            unsigned int w = g.size().width;
            unsigned int h = g.size().height;

            g.rectangle(nana::rectangle{ 0, 0, w, h }, true, BLACK);
            g.rectangle(nana::rectangle{ 2, 2, w - 4, h - 4 }, true, WHITE);

        });
        dw.update();
    }
    void Set()
    {
        tbox.create(*this, nana::rectangle(2, 2, 10, 10), true);
        tbox.typeface(f);
        tbox.multi_lines(false);
        tbox.borderless(true);
        tbox.bgcolor(COLOR4_NANA);
        tbox.fgcolor(COLOR1_NANA);

        events().resized([&](const nana::arg_resized& ei)
        {
            int w = ei.width - 4;
            int h = ei.height - 4;
            if (w < 0)
                w = 0;
            if (h < 0)
                h = 0;
            tbox.size(nana::size(w, h));
        });

        OnPaint();
    }

    void _m_complete_creation() override
    {
        Set();
    }
};
