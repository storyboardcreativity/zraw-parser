#pragma once

#include <cstdarg>
#include <map>
#include <thread>

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/label.hpp>

#include <IConsoleView.hpp>

class DebugTabUserControl : public nana::panel<true>, public IConsoleView
{
    class _console_output_class : public IConsoleOutput
    {
    public:
        _console_output_class(DebugTabUserControl& control)
            : _userControl(control), _start(std::chrono::system_clock::now()) {}

        void printf(const char* format, ...) override
        {
            // 1. Calculate buffer length
            va_list args;
            va_start(args, format);
            auto bufsz = vsnprintf(NULL, 0, format, args);
            va_end(args);

            // 2. Create buffer
            char *buffer = new char[bufsz + 1];

            // 3. Print to buffer
            va_start(args, format);
            vsprintf(buffer, format, args);
            va_end(args);

            // 4. Get current thread ID and it's category
            if (_threads.find(std::this_thread::get_id()) == _threads.end())
            {
                auto thread_str = std::string("Thread #") + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
                auto cat = _userControl.ConsoleBox.append(thread_str);
                _threads[std::this_thread::get_id()] = cat;
            }
            
            // 5. Calculate time offset
            const std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - _start;

            // 6. Append string
            _threads[std::this_thread::get_id()].append({ std::to_string(elapsed_seconds.count()), std::string(buffer) });

            // 7. Remove buffer
            delete[] buffer;
        }

    private:
        DebugTabUserControl& _userControl;

        std::chrono::time_point<std::chrono::system_clock> _start;

        std::map<std::thread::id, nana::drawerbase::listbox::cat_proxy> _threads;
    };

    _console_output_class _console_output_object;

public:
    DebugTabUserControl() : _console_output_object(_console_output_class(*this)), initialized_(false) {}

    DebugTabUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
        : nana::panel<true>(wd, r, visible), _console_output_object(_console_output_class(*this)), initialized_(false)
    {
        this->Create(wd, r, visible);
    }

    ~DebugTabUserControl() = default;

    bool Create(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
    {
        if(!nana::panel<true>::create(wd, r, visible))
            return false;

        Init();

        return true;
    }

    void Init() override
    {
        if (!initialized_)
        {
            place_.bind(*this);
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[10,variable,25] _field1>");
            // _panel1
            _panel1.create(*this);
            _panel1.transparent(true);
            place_["_field1"] << _panel1;
            // _panel2
            _panel2.create(*this);
            _panel2_place_.bind(_panel2);
            _panel2_place_.div("margin=[5,5,5,5] gap=2 _field_");
            _panel2.transparent(true);
            place_["_field1"] << _panel2;
            // ConsoleBox
            ConsoleBox.create(_panel2);
            ConsoleBox.append_header("Time (seconds)", 120);
            ConsoleBox.append_header("Message", 550 - 120);
            _panel2_place_["_field_"] << ConsoleBox;
            // _panel3
            _panel3.create(*this);
            _panel3_place_.bind(_panel3);
            _panel3_place_.div("weight=25 margin=[5,5,5,5] <gap=2 arrange=[40,variable] _field2>");
            _panel3.transparent(true);
            place_["_field1"] << _panel3;
            // _statusHeaderLabel
            _statusHeaderLabel.create(_panel3);
            _panel3_place_["_field2"] << _statusHeaderLabel;
            _statusHeaderLabel.caption("Status:");
            _statusHeaderLabel.transparent(true);
            // StatusLabel
            StatusLabel.create(_panel3);
            _panel3_place_["_field2"] << StatusLabel;
            StatusLabel.typeface(nana::paint::font("", 9, { 400, true, false, false }));
            StatusLabel.caption("STATUS_LABEL");
            StatusLabel.transparent(true);

            initialized_ = true;
        }

        place_.collocate();
        _panel2_place_.collocate();
        _panel3_place_.collocate();
    }

    IConsoleOutput& Console() override
    {
        if (!initialized_)
            throw std::runtime_error("Control is not initialized yet!");
        return _console_output_object;
    }

protected:
    nana::place place_;
    nana::panel<true> _panel1;
    nana::panel<true> _panel2;
    nana::place _panel2_place_;
    nana::listbox ConsoleBox;
    nana::panel<true> _panel3;
    nana::place _panel3_place_;
    nana::label _statusHeaderLabel;
    nana::label StatusLabel;

    // ===

    bool initialized_;
};
