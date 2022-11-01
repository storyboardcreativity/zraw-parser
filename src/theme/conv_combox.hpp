#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/panel.hpp>

#include "conv_common.hpp"

class conv_combox : public nana::combox
{
public:
    conv_combox() : combox(), f{ FONT_NAME, FONT_SIZE } {}

    void setFont(nana::paint::font &ft)
    {
        f = ft;
        typeface(f);
    }
private:
    nana::paint::font f;
    void Set()
    {
        bgcolor(COLOR4_NANA);
        fgcolor(COLOR1_NANA);

        typeface(f);
    }

    void _m_complete_creation() override
    {
        Set();
    }
};
