#pragma once

#include <nana/gui.hpp>
#include <nana/gui/element.hpp>
#include <nana/gui/widgets/tabbar.hpp>

#include "conv_common.hpp"

class conv_tabbar : public nana::tabbar<size_t>
{
public:
    conv_tabbar() : tabbar(), f{ FONT_NAME, FONT_SIZE } {}

    void setFont(nana::paint::font &ft) { f = ft; }
private:
    nana::paint::font f;
    void Set()
    {
        bgcolor(WHITE);
        fgcolor(COLOR1_NANA);
        typeface(f);

        renderer(_renderer);
    }

    void _m_complete_creation() override
    {
        Set();
    }

    class : public nana::drawerbase::tabbar::item_renderer
    {
    public:
        void background(graph_reference graph, const nana::rectangle&, const ::nana::color& bgcolor) override
        {
            if (bgcolor_ != bgcolor)
            {
                bgcolor_ = bgcolor;

                dark_bgcolor_ = bgcolor.blend(nana::colors::black, 0.1);
                blcolor_ = bgcolor.blend(nana::colors::black, 0.5);
                ilcolor_ = bgcolor.blend(nana::colors::white, 0.1);
            }

            graph.rectangle(true, bgcolor);
        }
        void item(graph_reference graph, const item_t& m, bool active, state_t sta) override
        {
            const nana::rectangle & r = m.r;
            nana::color bgcolor;
            nana::color blcolor;
            nana::color dark_bgcolor;

            if (m.bgcolor.invisible())
            {
                bgcolor = bgcolor_;
                blcolor = blcolor_;
                dark_bgcolor = dark_bgcolor_;
            }
            else
            {
                bgcolor = m.bgcolor;
                blcolor = m.bgcolor.blend(nana::colors::black, 0.5);
                dark_bgcolor = m.bgcolor.blend(nana::colors::black, 0.1);
            }

            auto round_r = r;
            round_r.height += 2;

            //graph.round_rectangle(round_r, 3, 3, blcolor, true, colors::white);

            graph.rectangle(nana::rectangle{ round_r.x, round_r.y, round_r.width, round_r.height }, true, active ? COLOR3_NANA : COLOR0_NANA);
            graph.rectangle(nana::rectangle{ round_r.x + 2, round_r.y + 2, round_r.width - 4, round_r.height - 4 }, true, active ? COLOR3_NANA : COLOR0_NANA);

            auto beg = bgcolor;
            auto end = dark_bgcolor;

            if (active)
            {
                if (m.bgcolor.invisible())
                    beg = ilcolor_;
                else
                    beg = m.bgcolor.blend(COLOR5_NANA, 0.5);
                end = bgcolor;
            }

            if (sta == item_renderer::highlight)
                beg = beg.blend(COLOR5_NANA, 0.5);

            graph.gradual_rectangle(round_r.pare_off(2), beg, end, true);
        }
        void close_fly(graph_reference graph, const nana::rectangle& r, bool active, state_t sta) override
        {
            using namespace nana::paint;
            ::nana::color clr{ nana::colors::black };

            if (sta == item_renderer::highlight)
            {
                ::nana::color bgcolor{ static_cast<nana::color_rgb>(0xCCD2DD) };
                ::nana::color rect_clr{ static_cast<nana::color_rgb>(0x9da3ab) };
                graph.round_rectangle(r, 1, 1, rect_clr, false, {});
                nana::rectangle draw_r(r);
                graph.rectangle(draw_r.pare_off(1), false, rect_clr.blend(bgcolor, 0.2));
                graph.rectangle(draw_r.pare_off(1), false, rect_clr.blend(bgcolor, 0.6));
                graph.rectangle(draw_r.pare_off(1), false, rect_clr.blend(bgcolor, 0.8));
            }
            else if (!active)
                clr = static_cast<nana::color_rgb>(0x9299a4);

            nana::facade<nana::element::x_icon> x_icon;
            x_icon.draw(graph, {}, nana::colors::black, { r.x + static_cast<int>(r.width - 16) / 2, r.y + static_cast<int>(r.height - 16) / 2, 16, 16 }, nana::element_state::normal);
        }

        void add(graph_reference graph, const nana::rectangle& r, state_t sta) override
        {
            int x = r.x + (static_cast<int>(r.width) - 14) / 2;
            int y = r.y + (static_cast<int>(r.height) - 14) / 2;

            ::nana::color clr;

            switch (sta)
            {
            case item_renderer::highlight:
                clr = nana::colors::white; break;
            case item_renderer::press:
                clr = static_cast<nana::color_rgb>(0xA0A0A0); break;
            case item_renderer::disable:
                clr = static_cast<nana::color_rgb>(0x808080); break;
            default:
                clr = static_cast<nana::color_rgb>(0xF0F0F0);
            }
            graph.rectangle(r, true, bgcolor_);
            nana::facade<nana::element::cross> cross;
            cross.draw(graph, {}, clr, { x, y, 14, 6 }, nana::element_state::normal);
        }
        void close(graph_reference graph, const nana::rectangle& r, state_t sta) override
        {
            nana::facade<nana::element::x_icon> x_icon;
            x_icon.draw(graph, {}, nana::colors::black, { r.x + static_cast<int>(r.width - 16) / 2, r.y + static_cast<int>(r.height - 16) / 2, 16, 16 }, nana::element_state::normal);
            if (item_renderer::highlight == sta)
                graph.rectangle(r, false, static_cast<nana::color_rgb>(0xa0a0a0));
        }
        void back(graph_reference graph, const nana::rectangle& r, state_t sta) override
        {
            _m_draw_arrow(graph, r, sta, nana::direction::west);
        }
        void next(graph_reference graph, const nana::rectangle& r, state_t sta) override
        {
            _m_draw_arrow(graph, r, sta, nana::direction::east);
        }
        void list(graph_reference graph, const nana::rectangle& r, state_t sta) override
        {
            _m_draw_arrow(graph, r, sta, nana::direction::south);
        }

    private:
        void _m_draw_arrow(graph_reference graph, const nana::rectangle& r, state_t sta, ::nana::direction dir)
        {
            nana::facade<nana::element::arrow> arrow("solid_triangle");
            arrow.direction(dir);
            nana::colors fgcolor = nana::colors::black;
            if (item_renderer::disable == sta)
            {
                arrow.switch_to("hollow_triangle");
                fgcolor = nana::colors::gray;
            }
            auto arrow_r = r;
            arrow_r.x += static_cast<int>(arrow_r.width - 16) / 2;
            arrow_r.y += static_cast<int>(arrow_r.height - 16) / 2;
            arrow_r.width = arrow_r.height = 16;
            arrow.draw(graph, bgcolor_, fgcolor, arrow_r, nana::element_state::normal);

            if (item_renderer::highlight == sta)
                graph.rectangle(r, false, nana::colors::dark_gray);
        }

        ::nana::color bgcolor_;
        ::nana::color dark_bgcolor_;
        ::nana::color blcolor_;
        ::nana::color ilcolor_;
    } _renderer;
};
