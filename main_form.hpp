#pragma once

#include <stdarg.h>

#include <nana/system/platform.hpp>

#include <nana/gui.hpp>
#include <nana/gui/filebox.hpp>

// Widgets
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/group.hpp>

#include "MovAvInfoDetect.hpp"
#include "IConsoleOutput.hpp"
#include "ZrawNewExtractor.hpp"

class UserInterfaceConsole : public IConsoleOutput
{
public:
    UserInterfaceConsole(nana::textbox &textbox_console)
        : _textbox_console(textbox_console)
    {
        _textbox_console.reset("Storyboard Creativity ZRAW Converter.\nConsole started.\n");
    }

    void printf(char *format, ...)
    {
        // 1. Calculate buffer length
        va_list args;
        va_start(args, format);
        ssize_t bufsz = vsnprintf(NULL, 0, format, args);
        va_end(args);

        // 2. Create buffer
        char *buffer = new char[bufsz + 1];

        // 3. Print to buffer
        va_start(args, format);
        vsprintf(buffer, format, args);
        va_end(args);

        // 4. Append string
        _textbox_console.append(std::string(buffer), false);

        // 5. Remove buffer
        delete buffer;
    }

protected:
    nana::textbox &_textbox_console;
};

class main_form : public nana::form
{
public:
    main_form() : nana::form(nana::API::make_center(640, 480), appear::decorate<appear::taskbar>())
    {
        this->caption(("ZRAW Video Converter"));

        //this->div("<main margin=10 grid=[3,3] gap=5 collapse(2,0,1,3)>");

        // pathboxes - boxes like {textbox button}
        // consolebox - console group widget
        this->div("<>"
                  "<weight=100% vertical"
                        "<weight=5%>"
                        "<weight=90% vertical"
                            "<vertical gap=10"
                                "<weight=25 horizontal pA"
                                    "<>"
                                    "<weight=60% pA_textbox>"
                                    "<weight=20% pA_button>"
                                    "<>"
                                ">"
                                "<weight=25 horizontal pB"
                                    "<>"
                                    "<weight=60% pB_textbox>"
                                    "<weight=20% pB_button>"
                                    "<>"
                                ">"
                                "<weight=25>"
                                "<<> <weight=90% zrawinfobox> <>>"
                                "<weight=50 <> <weight=200 convert_button> <>>"
                                "<<> <weight=90% consolebox> <>>"
                            ">"
                        ">"
                        "<>"
                    ">"
                    "<>");

        // Current file textbox
        _textbox__current_file.create(*this);
        _textbox__current_file.multi_lines(false);
        this->operator[]("pA_textbox") << _textbox__current_file;

        // "Open File" button
        _button__open_file.create(*this);
        _button__open_file.caption("Open File");
        _button__open_file.events().click([this](const nana::arg_click &ei) { __button_handler__open_file(ei); });
        this->operator[]("pA_button") << _button__open_file;

        // ZRAW info panel and group widget
        _group_console.create(*this);

        _textbox_console.create(_group_console);
        _textbox_console.editable(false);
        _textbox_console.reset("");
        _textbox_console.bgcolor(nana::color(200, 200, 200));

        _group_console.caption("Console");
        _group_console.div("margin=10 <_textbox_console>");
        _group_console["_textbox_console"] << _textbox_console;
        _group_console.collocate();
        this->operator[]("consolebox") << _group_console;

        // Output folder textbox
        _textbox__output_folder.create(*this);
        _textbox__output_folder.multi_lines(false);
        this->operator[]("pB_textbox") << _textbox__output_folder;

        // "Output Folder" button
        _button__output_path.create(*this);
        _button__output_path.caption("Output Folder");
        _button__output_path.events().click([this](const nana::arg_click &ei) { __button_handler__choose_output_path(ei); });
        this->operator[]("pB_button") << _button__output_path;

        // ZRAW info panel and group widget
        _zraw_group.create(*this);

        _textbox_zraw_info.create(_zraw_group);
        _textbox_zraw_info.editable(false);
        _textbox_zraw_info.reset("No info.");
        _textbox_zraw_info.bgcolor(nana::color(200, 200, 200));

        _zraw_group.caption("ZRAW file info");
        _zraw_group.div("margin=10 <_textbox_zraw_info>");
        _zraw_group["_textbox_zraw_info"] << _textbox_zraw_info;
        _zraw_group.collocate();
        this->operator[]("zrawinfobox") << _zraw_group;

        // "Convert" button
        _button__convert.create(*this);
        _button__convert.caption("Convert");
        _button__convert.events().click([this](const nana::arg_click &ei) { __button_handler__convert(ei); });
        _button__convert.enabled(false);
        this->operator[]("convert_button") << _button__convert;

        this->collocate();

        _console = new UserInterfaceConsole(_textbox_console);
    }

    ~main_form()
    {
        delete _console;
    }
private:
    void __button_handler__open_file(const nana::arg_click &ei)
    {
        nana::filebox fb(*this, true);
        fb.title("Open ZRAW Video File");
        fb.add_filter("ZRAW Video File (*.ZRAW)", "*.ZRAW");
        fb.add_filter("All Files", "*.*");
        fb.allow_multi_select(false);

        auto files = fb();
        if (files.size() == 1)
        {
            _textbox__current_file.reset(files[0]);
            _textbox_zraw_info.reset(MovDetectInfo(files[0].c_str()));

            // FIXME: check ZRAW file
            _button__convert.enabled(true);
        }
    }

    void __button_handler__choose_output_path(const nana::arg_click &ei)
    {
        nana::folderbox fb(*this);
        fb.title("Chose Output Folder");
        fb.allow_multi_select(false);

        auto files = fb();
        if (files.size() == 1)
            _textbox__output_folder.reset(files[0]);
    }

    void __button_handler__convert(const nana::arg_click &ei)
    {
        ZrawNewExtractor extractor;
        auto result = extractor.ProcessConversion(*_console, _textbox__current_file.text(), _textbox__output_folder.text());
        _console->printf("Finished conversion process\n");
    }

private:
    nana::button _button__open_file;
    nana::button _button__output_path;
    nana::button _button__convert;
    nana::textbox _textbox__current_file;
    nana::textbox _textbox__output_folder;

    nana::group _zraw_group;
    nana::textbox _textbox_zraw_info;

    nana::group _group_console;
    nana::textbox _textbox_console;

    IConsoleOutput* _console;
};