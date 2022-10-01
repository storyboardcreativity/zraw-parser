#pragma once

#include <IInputFileInfoView.hpp>
#include <IConsoleOutput.hpp>

#include <ZrawProcessingModel.hpp>

class InputFileInfoPresenter
{
public:
    InputFileInfoPresenter(ZrawProcessingModel& model, IInputFileInfoView& view, IConsoleOutput& debug_visitor) : _model(model), _view(view), _debug_visitor(debug_visitor)
    {
        _model.EventInputFilePathSelectedUpdate += MakeDelegate(this, &InputFileInfoPresenter::OnInputPathSelectionChangedInModel);
    }

    ~InputFileInfoPresenter()
    {
        _model.EventInputFilePathSelectedUpdate -= MakeDelegate(this, &InputFileInfoPresenter::OnInputPathSelectionChangedInModel);
    }

    void OnInputPathSelectionChangedInModel(std::string path)
    {
        _view.Clear();

        auto info = _model.ValidInfo_get();
        if (info == nullptr)
        {
            _debug_visitor.printf("Warning! Attempt to show info of null model. Skipped.");
            return;
        }

        // Paths category
        auto& cat0 = _view.CreateCategory("Paths");
        cat0.SetProperty("Output folder", info->OutputPath);

        cat0.SetProperty("Input file", path);

        // Tracks category
        auto& cat = _view.CreateCategory("Tracks");
        for (int i = 0; i < info->FileInfo.Tracks().size(); ++i)
        {
			cat.SetProperty("Track #" + std::to_string(i), "Track #" + std::to_string(i));//info->TracksInfo.tracks[i].codec_name);

			auto& track = info->FileInfo.Tracks()[i];

			IInputFileInfoView::ICategory* trackCat = nullptr;

			switch (track.Media().Type())
			{
			case TinyMovTrackMedia::Type_t::Video:
			{
				auto& desc_table = track.Media().Info().DescriptionTable().VideoDescriptionTable();
				if (desc_table.size() != 1)
					continue;

				auto& desc = desc_table[0];

				trackCat = &_view.CreateCategory("Video (Track #" + std::to_string(i) + ")");

				trackCat->SetProperty("Frame Width", "%d", desc.Width());
				trackCat->SetProperty("Frame Height", "%d", desc.Height());

				auto zraw_ext = desc.Ext_ZRAW();
				if (zraw_ext.exists)
				{
					trackCat->SetProperty("ZRAW Version", "0x%X", zraw_ext.version);
					trackCat->SetProperty("ZRAW unk0 value", "0x%X", zraw_ext.unk0);

					switch (zraw_ext.version)
					{
					case 0x12EA78D2:
						trackCat->SetProperty("Type", "Compressed RAW CFA");
						break;

					case 0x45A32DEF:
						trackCat->SetProperty("Type", "HEVC bitstream");
						{
							switch (zraw_ext.unk1)
							{
							case 0:
								trackCat->SetProperty("Encryption", "AES");
								break;

							case 1:
								trackCat->SetProperty("Encryption", "XOR");
								break;

							default:
								trackCat->SetProperty("Encryption", "Unknown");
								break;
							}
						}
						break;

					default:
						trackCat->SetProperty("Type", "Unknown");
						break;
					}
				}

				break;
			}
			case TinyMovTrackMedia::Type_t::Audio:
			{
				auto& desc_table = track.Media().Info().DescriptionTable().AudioDescriptionTable();
				if (desc_table.size() != 1)
					continue;

				auto& desc = desc_table[0];

				trackCat = &_view.CreateCategory("Audio (Track #" + std::to_string(i) + ")");

				break;
			}
			case TinyMovTrackMedia::Type_t::Timecode:
			{
				auto& desc_table = track.Media().Info().DescriptionTable().TimecodeDescriptionTable();
				if (desc_table.size() != 1)
					continue;

				auto& desc = desc_table[0];

				trackCat = &_view.CreateCategory("Timecode (Track #" + std::to_string(i) + ")");
				break;
			}
			default:
			{
				trackCat = &_view.CreateCategory("Unknown (Track #" + std::to_string(i) + ")");
				break;
			}
			}

			if (track.Media().Info().ForcedSampleSize() != 0)
				trackCat->SetProperty("Universal Sample Size", "%d", track.Media().Info().ForcedSampleSize());

			auto& sampleSizesCat = _view.CreateCategory("Track #" + std::to_string(i) + " sample sizes");
			auto& chunkOffsetsCat = _view.CreateCategory("Track #" + std::to_string(i) + " chunk offsets");

			sampleSizesCat.Lock();
			chunkOffsetsCat.Lock();

			auto& ssizes = track.Media().Info().SampleSizes();
			for (uint64_t p = 0; p < ssizes.size(); ++p)
				sampleSizesCat.SetProperty("Sample #" + std::to_string(p), "%lld (0x%llX)", ssizes[p], ssizes[p]);

			auto& coffsets = track.Media().Info().ChunkOffsets();
			for (uint64_t p = 0; p < coffsets.size(); ++p)
				chunkOffsetsCat.SetProperty("Chunk #" + std::to_string(p), "%lld (0x%llX)", coffsets[p], coffsets[p]);

			sampleSizesCat.Unlock();
			chunkOffsetsCat.Unlock();
        }
    }

protected:

    ZrawProcessingModel& _model;
    IInputFileInfoView& _view;
    IConsoleOutput& _debug_visitor;
};
