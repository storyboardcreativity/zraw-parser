#pragma once

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/listbox.hpp>

#include <IInputFileInfoView.hpp>

#include "theme/conv_common.hpp"
#include "theme/conv_listbox.hpp"

class InputFileInfoUserControl : public nana::panel<true>, public IInputFileInfoView
{
public:
    InputFileInfoUserControl() : initialized_(false)
    {
        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &InputFileInfoUserControl::OnThemeChanged);
    }

    InputFileInfoUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
        : nana::panel<true>(wd, r, visible), initialized_(false)
    {
        this->Create(wd, r, visible);

        ViewThemeSingleton::Instance().EventThemeChanged += MakeDelegate(this, &InputFileInfoUserControl::OnThemeChanged);
    }

    ~InputFileInfoUserControl()
    {
        ViewThemeSingleton::Instance().EventThemeChanged -= MakeDelegate(this, &InputFileInfoUserControl::OnThemeChanged);
    }

    bool Create(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
    {
        if(!nana::panel<true>::create(wd, r, visible))
            return false;

        Init();

        return true;
    }

    void Init()
    {
        if (!initialized_)
        {
            place_.bind(*this);
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[variable,10] _field1>");
            // _panel1
            //_panel1.create(*this);
            //place_["_field1"] << _panel1;
            // InfoListBox
            InfoListBox.create(*this);
            InfoListBox.append_header("Property", 120);
            InfoListBox.append_header("Info", 550 - 120);
            InfoListBox.typeface(nana::paint::font{ FONT_NAME, FONT_SIZE });
            InfoListBox.fgcolor(COLOR1_NANA);
            InfoListBox.scheme().cat_fgcolor = COLOR12_NANA;
            place_["_field1"] << InfoListBox;
            // _panel2
            _panel2.create(*this);
            place_["_field1"] << _panel2;

            initialized_ = true;
        }

        place_.collocate();

        OnThemeChanged(ViewThemeSingleton::Instance());
    }

    void Clear() override
    {
        InfoListBox.erase();
    }

    ICategory& CreateCategory(std::string name) override
    {
        _categories.push_back(std::make_unique<Category>(Category(InfoListBox, InfoListBox.append(name))));
        return *_categories[_categories.size() - 1];
    }

protected:
    nana::place place_;
    nana::panel<true> _panel1;
    conv_listbox InfoListBox;
    nana::panel<true> _panel2;

    // ===

    void OnThemeChanged(IViewTheme& theme) const
    {
        const auto bgColor = theme.Background();
        this->scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _panel1.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        //InfoListBox.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        _panel2.scheme().background = nana::color(bgColor.r, bgColor.g, bgColor.b, bgColor.alpha);
        
        // Refresh control to apply changes
        nana::API::refresh_window(*this);
    }

    // ===

    class Category : public ICategory
    {
    public:
        Category(nana::listbox& listBox, nana::drawerbase::listbox::cat_proxy category) : _listBox(listBox), _category(category) {}

        void SetProperty(std::string name, std::string format, ...) override
        {
            // 1. Calculate buffer length
            va_list args;
            va_start(args, format);
            auto bufsz = vsnprintf(NULL, 0, format.c_str(), args);
            va_end(args);

            // 2. Create buffer
            char *buffer = new char[bufsz + 1];

            // 3. Print to buffer
            va_start(args, format);
            vsprintf(buffer, format.c_str(), args);
            va_end(args);

            // 4. Append string
            _category.append({name, std::string(buffer)});

            // 5. Remove buffer
            delete[] buffer;
        }

        void Lock() override
        {
            _listBox.auto_draw(false);
        }

        void Unlock() override
        {
            _listBox.auto_draw(true);
        }
    private:
        nana::listbox& _listBox;
        nana::drawerbase::listbox::cat_proxy _category;
    };

    std::vector<std::unique_ptr<Category>> _categories;
    bool initialized_;
};
