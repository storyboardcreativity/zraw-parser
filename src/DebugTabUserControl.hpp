#pragma once

#include <cstdarg>
#include <map>
#include <thread>
#include <fstream>
#include <mutex>

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/label.hpp>

#include <IConsoleView.hpp>

#include "theme/conv_common.hpp"
#include "theme/conv_label.hpp"
#include "theme/conv_listbox.hpp"

class DebugTabUserControl : public nana::panel<true>, public IConsoleView
{
    class _console_output_class : public IConsoleOutput
    {
        const std::string _log_file_path = "debug_log.log";
    public:
        _console_output_class(DebugTabUserControl& control)
            : _userControl(control), _start(std::chrono::system_clock::now()) {}

        void printf(const char* format, ...) override
        {
            //_mutex.lock();

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

            auto msg = std::string("Thread #") + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + " | " + std::to_string(elapsed_seconds.count()) + "s | " + std::string(buffer);
            _appendLineToFile(_log_file_path, msg);

            // 6. Append string
            _threads[std::this_thread::get_id()].append({ std::to_string(elapsed_seconds.count()), std::string(buffer) });

            // 7. Remove buffer
            delete[] buffer;

            //_mutex.unlock();
        }

    private:
        DebugTabUserControl& _userControl;

        std::chrono::time_point<std::chrono::system_clock> _start;

        std::map<std::thread::id, nana::drawerbase::listbox::cat_proxy> _threads;

        std::mutex _mutex;

        // Following code is from: https://stackoverflow.com/a/39462191
        void _appendLineToFile(std::string filepath, std::string line)
        {
            std::ofstream file;

            // can't enable exception now because of gcc bug that raises ios_base::failure with useless message
            // file.exceptions(file.exceptions() | std::ios::failbit);

            file.open(filepath, std::ios::out | std::ios::app);
            if (file.fail())
                throw std::ios_base::failure(std::strerror(errno));

            // make sure write fails with exception if something is wrong
            file.exceptions(file.exceptions() | std::ios::failbit | std::ifstream::badbit);

            file << line << std::endl;
        }
    };

    _console_output_class _console_output_object;

public:
    DebugTabUserControl() : _console_output_object(_console_output_class(*this)), initialized_(false)
    {
        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &DebugTabUserControl::OnThemeChanged);
    }

    DebugTabUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
        : nana::panel<true>(wd, r, visible), _console_output_object(_console_output_class(*this)), initialized_(false)
    {
        this->Create(wd, r, visible);

        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &DebugTabUserControl::OnThemeChanged);
    }

    ~DebugTabUserControl()
    {
        ViewThemeSingleton::Instance().EventThemeChanged -= MakeDelegate(this, &DebugTabUserControl::OnThemeChanged);
    }

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
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[variable,25] _field1>");
            // _panel2
            _panel2.create(*this);
            _panel2_place_.bind(_panel2);
            _panel2_place_.div("margin=[0,0,0,0] gap=2 _field_");
            _panel2.transparent(true);
            place_["_field1"] << _panel2;
            // ConsoleBox
            ConsoleBox.create(_panel2);
            ConsoleBox.append_header("Time (seconds)", 120);
            ConsoleBox.append_header("Message", 550 - 120);
            ConsoleBox.typeface(nana::paint::font{ FONT0_NAME, FONT0_SIZE });
            ConsoleBox.scheme().cat_fgcolor = COLOR12_NANA;
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
            //_statusHeaderLabel.caption("Status:");
            _statusHeaderLabel.transparent(true);
            // StatusLabel
            StatusLabel.create(_panel3);
            _panel3_place_["_field2"] << StatusLabel;
            StatusLabel.typeface(nana::paint::font("", 9, { 400, true, false, false }));
            //StatusLabel.caption("STATUS_LABEL");
            StatusLabel.transparent(true);

            initialized_ = true;
        }

        place_.collocate();
        _panel2_place_.collocate();
        _panel3_place_.collocate();

        OnThemeChanged(ViewThemeSingleton::Instance());
    }

    IConsoleOutput& Console() override
    {
        if (!initialized_)
            throw std::runtime_error("Control is not initialized yet!");
        return _console_output_object;
    }

protected:
    nana::place place_;
    nana::panel<true> _panel2;
    nana::place _panel2_place_;
    conv_listbox ConsoleBox;
    nana::panel<true> _panel3;
    nana::place _panel3_place_;
    conv_label _statusHeaderLabel;
    conv_label StatusLabel;

    // ===

    void OnThemeChanged(IViewTheme& theme) const
    {
        const auto bgColor = theme.Background();
        this->scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _panel2.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        //ConsoleBox.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _panel3.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _statusHeaderLabel.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        StatusLabel.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);

        const auto textColor = theme.TextStandard();
        _statusHeaderLabel.scheme().foreground = nana::color(textColor.r, textColor.g, textColor.b, textColor.alpha);
        StatusLabel.scheme().foreground = nana::color(textColor.r, textColor.g, textColor.b, textColor.alpha);

        // Refresh control to apply changes
        nana::API::refresh_window(*this);
    }

    // ===

    bool initialized_;
};
