#pragma once

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/listbox.hpp>

#include <IInputFileInfoView.hpp>


class InputFileInfoUserControl : public nana::panel<true>, public IInputFileInfoView
{
public:
    InputFileInfoUserControl() : initialized_(false) {}

    InputFileInfoUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
        : nana::panel<true>(wd, r, visible), initialized_(false)
    {
        this->Create(wd, r, visible);
    }

    ~InputFileInfoUserControl() = default;

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
            place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[20,variable,10] _field1>");
            // _panel1
            _panel1.create(*this);
            place_["_field1"] << _panel1;
            // InfoListBox
            InfoListBox.create(*this);
            InfoListBox.append_header("Property");
            InfoListBox.append_header("Info");
            place_["_field1"] << InfoListBox;
            // _panel2
            _panel2.create(*this);
            place_["_field1"] << _panel2;

            initialized_ = true;
        }

        place_.collocate();
    }

    void Clear() override
    {
        InfoListBox.erase();
    }

    ICategory& CreateCategory(std::string name) override
    {
        _categories.push_back(std::make_unique<Category>(Category(InfoListBox.append(name))));
        return *_categories[_categories.size() - 1];
    }

protected:
    nana::place place_;
    nana::panel<true> _panel1;
    nana::listbox InfoListBox;
    nana::panel<true> _panel2;

    // ===

    class Category : public ICategory
    {
    public:
        Category(nana::drawerbase::listbox::cat_proxy category) : _category(category) {}

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

    private:
        nana::drawerbase::listbox::cat_proxy _category;
    };

    std::vector<std::unique_ptr<Category>> _categories;
    bool initialized_;
};
