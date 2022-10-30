#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/listbox.hpp>

#include "conv_common.hpp"

class conv_listbox : public nana::listbox
{
public:
    conv_listbox() : f{ FONT_NAME, FONT_SIZE } {}

    void setFont(nana::paint::font &ft) { f = ft; }
private:
    nana::paint::font f;
    void Set()
    {
        bgcolor(COLOR11_NANA);
        fgcolor(COLOR1_NANA);
        typeface(f);

        scheme().header_bgcolor = COLOR4_NANA;
        scheme().header_fgcolor = COLOR7_NANA;

        scheme().foreground = COLOR1_NANA;
        scheme().background = COLOR11_NANA;

        scheme().item_highlighted = COLOR4_NANA;
        scheme().item_selected = COLOR10_NANA;

        scheme().selection_box = COLOR10_NANA;

        scheme().max_fit_content = 1;

        this->borderless(true);

        nana::API::refresh_window(*this);
    }

    void _m_complete_creation() override
    {
        Set();
    }
};
