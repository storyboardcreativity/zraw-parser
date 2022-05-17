#pragma once

#include <memory>

#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/button.hpp>

#include <IBatchConversionListView.hpp>

class BatchConversionListUserControl : public nana::panel<true>, public IBatchConversionListView
{
public:
	BatchConversionListUserControl() = default;

	BatchConversionListUserControl(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
		: nana::panel<true>(wd, r, visible)
	{
		this->create(wd, r, visible);
	}

    ~BatchConversionListUserControl() = default;

	bool Create(nana::window wd, const nana::rectangle& r = {}, bool visible = true)
	{
		if(!nana::panel<true>::create(wd, r, visible))
			return false;

        Init();

		return true;
	}

    void Lock() override
	{
        _buttonAddFiles.enabled(false);
        _listboxAddedFiles.enabled(false);
	}

    void Unlock() override
	{
        _buttonAddFiles.enabled(true);
        _listboxAddedFiles.enabled(true);
	}

    // Use this function only when control is locked!
    std::vector<std::string> GetCheckedPathsList() override
	{
        std::vector<std::string> res;
        for (auto it = _items.begin(); it != _items.end(); ++it)
            if (it->second)
                res.push_back(it->first);
        return res;
	}

private:
	void Init()
	{
		place_.bind(*this);
		place_.div("margin=[5,5,5,5] <vert margin=[5,5,5,5] gap=2 arrange=[variable,40] _field>");

		// _listboxAddedFiles
		_listboxAddedFiles.create(*this);
		place_["_field"] << _listboxAddedFiles;
		_listboxAddedFiles.checkable(true);
		_listboxAddedFiles.sortable(false);
        _listboxAddedFiles.events().checked([&](const nana::arg_listbox& args)
        {
            _items[args.item.text(2)] = args.item.checked();
        });
        _listboxAddedFiles.events().selected([&](const nana::arg_listbox& args)
        {
            // TODO: ...
        });
        _listboxAddedFiles.events().mouse_dropfiles([&](const nana::arg_dropfiles& arg)
        {
            _addFiles(arg.files);
        });
        _listboxAddedFiles.enable_dropfiles(true);

		// _buttonAddFiles
		_buttonAddFiles.create(*this);
		place_["_field"] << _buttonAddFiles;
		_buttonAddFiles.caption("Add Files");
        _buttonAddFiles.events().click([&]()
        {
            nana::filebox fb(*this, true);
            fb.title("Open ZRAW Video File");
            fb.add_filter("ZRAW Video File (*.ZRAW)", "*.ZRAW");
            fb.add_filter("All Files", "*.*");
            fb.allow_multi_select(true);

            _addFiles(fb());
        });

        _initCategories();

		place_.collocate();
	}

    void _addFiles(std::vector<std::filesystem::path> files)
	{
        auto cat = _listboxAddedFiles.at(0);

        for (auto it = files.begin(); it != files.end(); ++it)
        {
            auto item_it = _items.find(it->string());
            if (item_it == _items.end())
            {
                cat.append({ it->filename().string(), "unknown", it->string() });
                cat.back().check(true, false);
                _items[it->string()] = true;
            }
        }
	}

    void _initCategories()
	{
        _listboxAddedFiles.append_header("File Name", 120);
        _listboxAddedFiles.append_header("Type", 120);
        _listboxAddedFiles.append_header("File Path", 550 - 240);
	}


protected:
	nana::place place_;
	nana::listbox _listboxAddedFiles;
	nana::button _buttonAddFiles;

    std::map<std::string, bool> _items;
};
