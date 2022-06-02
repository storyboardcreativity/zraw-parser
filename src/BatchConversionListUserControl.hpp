#pragma once

#include <memory>

#include <nana/gui/place.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/progress.hpp>

#include <IBatchConversionListView.hpp>

#define BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PATH 3
#define BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PROGRESS 1
#define BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__TYPE 2

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
            if (it->text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PATH) == fspath.string())
            {
                it->check(checked, false);
                return;
            }
        }

        cat.append({
            fspath.filename().string(),
            "0",
            (type == "") ? "---" : type,
            fspath.string()
        });
        cat.back().check((type == "") ? false : checked, false);
	}

    void RemovePath(std::string path) override
	{
        throw "Not implemented yet.";
	}

    void SetPathEnabled(std::string path, bool checked) override
    {
        const std::filesystem::path fspath(path);

        const auto cat = _listboxAddedFiles.at(0);
        for (auto it = cat.begin(); it != cat.end(); ++it)
        {
            if (it->text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PATH) == fspath.string())
            {
                if (it->checked() != checked)
                    it->check(checked, false);
            }
        }
	}

    void ChangeInputFilePercent(std::string path, unsigned int percent) override
    {
        const std::filesystem::path fspath(path);

        auto cat = _listboxAddedFiles.at(0);
        for (auto it = cat.begin(); it != cat.end(); ++it)
        {
            if (it->text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PATH) == fspath.string())
            {
                it->text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PROGRESS, std::to_string(percent));
                return;
            }
        }
	}

    void ChangeInputFileColor(std::string path, unsigned int r, unsigned int g, unsigned int b, double alpha) override
    {
        const std::filesystem::path fspath(path);

        auto cat = _listboxAddedFiles.at(0);
        for (auto it = cat.begin(); it != cat.end(); ++it)
        {
            if (it->text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PATH) == fspath.string())
            {
                it->fgcolor(nana::color(r, g, b, alpha));
                return;
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
            if (args.item.checked() != false)
                if (args.item.text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__TYPE) == "---")
                {
                    args.item.check(false);
                    return;
                }
            
            TRIGGER_EVENT(EventInputFileEnabled, args.item.text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PATH), args.item.checked());
        });
        _listboxAddedFiles.events().selected([&](const nana::arg_listbox& args)
        {
            TRIGGER_EVENT(EventInputFileSelected, args.item.text(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PATH));
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
        _listboxAddedFiles.append_header("Progress", 120);
        _listboxAddedFiles.append_header("Type", 120);
        _listboxAddedFiles.append_header("File Path", 550 - 240);

        _listboxAddedFiles.at(0).inline_factory(BATCH_CONVERSION_LIST_USER_CONTROL__COL_NUM__PROGRESS, nana::pat::make_factory<InlineProgressBarWidget>());
	}

protected:
	nana::place place_;
	nana::listbox _listboxAddedFiles;
	nana::button _buttonAddFiles;

    class InlineProgressBarWidget : public nana::listbox::inline_notifier_interface
    {
    private:
        void create(nana::window wd) override
        {
            _progressBarControl.create(wd);
            _progressBarControl.events().click([this]
            {
                //Select the item when clicks the textbox
                indicator_->selected(pos_);
            });
            _progressBarControl.events().mouse_move([this]
            {
                //Highlight the item when hovers the textbox
                indicator_->hovered(pos_);
            });

            _percentLabel.create(wd);
            _percentLabel.events().click([this]
            {
                //Select the item when clicks the textbox
                indicator_->selected(pos_);
            });
            _percentLabel.events().mouse_move([this]
            {
                //Highlight the item when hovers the textbox
                indicator_->hovered(pos_);
            });
            _percentLabel.caption("0%");
            _percentLabel.transparent(true);
            _percentLabel.text_align(nana::align::center, nana::align_v::center);
        }

        void activate(inline_indicator& ind, index_type pos) override
        {
            indicator_ = &ind;
            pos_ = pos;
        }

        void notify_status(status_type status, bool status_on) override {}

        void resize(const nana::size& dimension) override
        {
            auto sz = dimension;
            //sz.width -= (sz.width < 50 ? 0 : 50);
            _progressBarControl.size(sz);
            _percentLabel.size(sz);
        }

        virtual void set(const value_type& value)
        {
            unsigned int val = std::atoi(value.c_str());
            if (val > 100)
                val = 100;

            if (_progressBarControl.value() != val)
                _progressBarControl.value(val);

            auto str = value + "%";
            if (_percentLabel.caption() != str)
                _percentLabel.caption(str);
        }

        bool whether_to_draw() const override
        {
            return false;
        }

    private:
        inline_indicator * indicator_{ nullptr };
        index_type pos_;
        nana::progress _progressBarControl;
        nana::label _percentLabel;
    };
};
