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

    void AddPath(std::string path, std::string type, bool checked) override
	{
        const std::filesystem::path fspath(path);

        auto cat = _listboxAddedFiles.at(0);
        for (auto it = cat.begin(); it != cat.end(); ++it)
        {
            if (it->text(2) == fspath.string())
            {
                it->check(checked, false);
                return;
            }
        }

        cat.append({ fspath.filename().string(), (type == "") ? "---" : type, fspath.string() });
        cat.back().check(checked, false);
	}

    void RemovePath(std::string path) override
	{
        throw "Not implemented yet.";
	}

    void SetPathEnabled(std::string path, bool checked)
	{
        const std::filesystem::path fspath(path);

        const auto cat = _listboxAddedFiles.at(0);
        for (auto it = cat.begin(); it != cat.end(); ++it)
        {
            if (it->text(2) == fspath.string())
            {
                if (it->checked() != checked)
                    it->check(checked, false);
            }
        }
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
            // Notify owner about changes
            TRIGGER_EVENT(EventInputFileEnabled, args.item.text(2), args.item.checked());
        });
        _listboxAddedFiles.events().selected([&](const nana::arg_listbox& args)
        {
            TRIGGER_EVENT(EventInputFileSelected, args.item.text(2));
        });
        _listboxAddedFiles.events().mouse_dropfiles([&](const nana::arg_dropfiles& arg)
        {
            std::vector<std::string> paths;
            for (auto it = arg.files.begin(); it != arg.files.end(); ++it)
                paths.push_back(it->string());
            TRIGGER_EVENT(EventInputFilesAdded, paths);
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

            std::vector<std::string> paths;
            auto files = fb();
            for (auto it = files.begin(); it != files.end(); ++it)
                paths.push_back(it->string());

            TRIGGER_EVENT(EventInputFilesAdded, paths);
        });

        _initCategories();

		place_.collocate();
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
};
