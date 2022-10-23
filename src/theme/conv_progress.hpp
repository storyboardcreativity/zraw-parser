#pragma once

#include <nana/gui/widgets/progress.hpp>

#include "conv_common.hpp"

class conv_substance
{
public:
    static const unsigned border_px = 1;

    void set_widget(nana::widget& wdg)
    {
        widget_ = static_cast<nana::progress*>(&wdg);
    }

    nana::progress* widget_ptr() const
    {
        return widget_;
    }

    unsigned inc()
    {
        auto val = value(nullptr) + 1;
        return value(&val);
    }

    unsigned value(const unsigned* value_ptr)
    {
        //Sets new value if value_ptr is not a nullptr
        if (value_ptr)
        {
            if (unknown_)
                value_ += 5;
            else
                value_ = (std::min)(max_, *value_ptr);

            _m_try_refresh();
        }
        return value_;
    }

    void reset_value()
    {
        value_ = 0;
    }

    unsigned maximum(const unsigned * value_ptr)
    {
        //Sets new maximum if value_ptr is not a nullptr
        if (value_ptr)
        {
            max_ = (*value_ptr > 0 ? *value_ptr : 1);
            _m_try_refresh();
        }
        return max_;
    }

    bool unknown(const bool* state_ptr)
    {
        if (state_ptr)
        {
            unknown_ = *state_ptr;
            if (unknown_)
                value_px_ = 0;
            else
                value_ = (std::min)(value_, max_);
        }
        return unknown_;
    }

    unsigned value_px() const
    {
        return value_px_;
    }

    bool value_px_sync()
    {
        if (widget_)
        {
            unsigned value_px = (widget_->size().width - border_px * 2);

            //avoid overflow
            if (unknown_ || (value_ < max_))
                value_px = unsigned(double(value_px) * (double(value_) / double(max_)));

            if (value_px != value_px_)
            {
                value_px_ = value_px;
                return true;
            }
        }
        return false;
    }
private:
    void _m_try_refresh()
    {
        if (value_px_sync())
            nana::API::refresh_window(*widget_);
    }
private:
    nana::progress * widget_{ nullptr };
    unsigned max_{ 100 };
    unsigned value_{ 0 };
    unsigned value_px_{ 0 };
    bool unknown_{ false };
};

class conv_progress_trigger : public nana::drawer_trigger
{
public:
    conv_progress_trigger()
        : progress_(new conv_substance) {}
    ~conv_progress_trigger()
    {
        delete progress_;
    }

    conv_substance* progress() const
    {
        return progress_;
    }
private:
    void attached(widget_reference wdg, graph_reference)
    {
        progress_->set_widget(wdg);
    }
    void refresh(graph_reference graph)
    {
        const unsigned border_px = conv_substance::border_px;

        nana::rectangle rt_val{ graph.size() };
        auto const width = rt_val.width - border_px * 2;

        rt_val.pare_off(static_cast<int>(border_px));

        auto rt_bground = rt_val;
        if (false == progress_->unknown(nullptr))
        {
            //Sync the value_px otherwise the progress is incorrect when it is resized.
            progress_->value_px_sync();

            rt_bground.x = static_cast<int>(progress_->value_px()) + static_cast<int>(border_px);
            rt_bground.width -= progress_->value_px();

            rt_val.width = progress_->value_px();
        }
        else
        {
            auto const block = width / 3;

            auto const value = progress_->value(nullptr);

            auto left = (std::max)(0, static_cast<int>(value - block)) + static_cast<int>(border_px);
            auto right = static_cast<int>((std::min)(value, width + border_px - 1));

            if (right > left)
            {
                rt_val.x = left;
                rt_val.width = static_cast<unsigned>(right - left + 1);
            }
            else
                rt_val.width = 0;

            if (value >= width + block)
                progress_->reset_value();
        }

        auto & sch = progress_->widget_ptr()->scheme();

        // Draw the background

        auto bgcolor = sch.background.get_color();
        graph.rectangle(rt_bground, true, bgcolor);

        // Draw the fgcolor

        auto fgcolor = sch.foreground.get_color();
        if (rt_val.width > 0)
            graph.rectangle(rt_val, true, fgcolor);

        if (width == rt_val.width)
            return;

        // Draw fg border

        if (rt_val.width > 0)
        {
            rt_val.x += rt_val.width - 2;
            rt_val.width = rt_val.width > 2 ? 2 : rt_val.width;
            graph.rectangle(rt_val, true, COLOR_PR1_NANA);
        }
    }
private:
    conv_substance* const progress_;
};

class conv_progress
    : public nana::widget_object<nana::category::widget_tag, conv_progress_trigger, nana::general_events, nana::drawerbase::progress::scheme>
{
public:
    conv_progress() {}
    conv_progress(nana::window wd, bool visible)
    {
        create(wd, nana::rectangle(), visible);
    }
    conv_progress(nana::window wd, const nana::rectangle & r, bool visible)
    {
        create(wd, r, visible);
    }

    unsigned value() const
    {
        return get_drawer_trigger().progress()->value(nullptr);
    }
    unsigned value(unsigned val)
    {
        nana::internal_scope_guard lock;
        if (nana::API::empty_window(this->handle()) == false)
            return get_drawer_trigger().progress()->value(&val);
        return 0;
    }
    unsigned inc()
    {
        nana::internal_scope_guard lock;
        return get_drawer_trigger().progress()->inc();
    }
    unsigned amount() const
    {
        return get_drawer_trigger().progress()->maximum(nullptr);
    }
    unsigned amount(unsigned value)
    {
        return get_drawer_trigger().progress()->maximum(&value);
    }
    void unknown(bool enb)
    {
        nana::internal_scope_guard lock;
        get_drawer_trigger().progress()->unknown(&enb);
    }
    bool unknown() const
    {
        return get_drawer_trigger().progress()->unknown(nullptr);
    }

protected:
    void Set()
    {
        bgcolor(COLOR11_NANA);
        fgcolor(COLOR_PR0_NANA);

        //scheme()//.gradient_bgcolor = nana::color(0, 0, 0, 0.0);
        //scheme()//.gradient_fgcolor = nana::color(0, 0, 0, 0.0);

        this->borderless(true);

        nana::API::refresh_window(*this);
    }

    void _m_complete_creation() override
    {
        Set();
    }
};
