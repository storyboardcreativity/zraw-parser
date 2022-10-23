#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/panel.hpp>

#include "conv_common.hpp"

class conv_label : public nana::label
{
public:
    conv_label() : label(), f{ FONT_SIZE, FONT } {}

    void setFont(nana::paint::font &ft) { f = ft; }
private:
    nana::paint::font f;
    void Set()
    {
        bgcolor(COLOR4_NANA);
        fgcolor(COLOR1_NANA);
        typeface(f);
        transparent(true);
    }

    void _m_complete_creation() override
    {
        Set();
    }
};
