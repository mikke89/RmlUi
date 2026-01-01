#include "HighScores.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/TypeConverter.h>
#include <algorithm>
#include <stdio.h>

HighScores* HighScores::instance = nullptr;

HighScores::HighScores(Rml::Context* context)
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;

	Rml::DataModelConstructor constructor = context->CreateDataModel("high_scores");
	if (!constructor)
		return;

	if (auto score_handle = constructor.RegisterStruct<Score>())
	{
		score_handle.RegisterMember("name_required", &Score::name_required);
		score_handle.RegisterMember("name", &Score::name);
		score_handle.RegisterMember("colour", &Score::GetColour);
		score_handle.RegisterMember("wave", &Score::wave);
		score_handle.RegisterMember("score", &Score::score);
	}

	constructor.RegisterArray<ScoreList>();

	constructor.Bind("scores", &scores);

	model_handle = constructor.GetModelHandle();

	LoadScores();
}

HighScores::~HighScores()
{
	RMLUI_ASSERT(instance == this);

	SaveScores();

	instance = nullptr;
}

void HighScores::Initialise(Rml::Context* context)
{
	new HighScores(context);
}

void HighScores::Shutdown()
{
	delete instance;
}

int HighScores::GetHighScore()
{
	if (instance->scores.empty())
		return 0;

	return instance->scores[0].score;
}

void HighScores::SubmitScore(const Rml::String& name, const Rml::Colourb& colour, int wave, int score)
{
	instance->SubmitScore(name, colour, wave, score, false);
}

void HighScores::SubmitScore(const Rml::Colourb& colour, int wave, int score)
{
	instance->SubmitScore("", colour, wave, score, true);
}

void HighScores::SubmitName(const Rml::String& name)
{
	for (Score& score : instance->scores)
	{
		if (score.name_required)
		{
			score.name = name;
			score.name_required = false;

			instance->model_handle.DirtyVariable("scores");
		}
	}
}

void HighScores::SubmitScore(const Rml::String& name, const Rml::Colourb& colour, int wave, int score, bool name_required)
{
	Score entry;
	entry.name = name;
	entry.colour = colour;
	entry.wave = wave;
	entry.score = score;
	entry.name_required = name_required;

	auto it = std::find_if(scores.begin(), scores.end(), [score](const Score& other) { return score > other.score; });

	scores.insert(it, std::move(entry));

	constexpr size_t MaxNumberScores = 10;

	if (scores.size() > MaxNumberScores)
		scores.pop_back();

	model_handle.DirtyVariable("scores");
}

void HighScores::LoadScores()
{
	// Open and read the high score file.
	FILE* scores_file = fopen("scores.txt", "rt");

	if (scores_file)
	{
		char buffer[1024];
		while (fgets(buffer, 1024, scores_file))
		{
			Rml::StringList score_parts;
			Rml::StringUtilities::ExpandString(score_parts, Rml::String(buffer), '\t');
			if (score_parts.size() == 4)
			{
				Rml::Colourb colour;
				int wave;
				int score;

				if (Rml::TypeConverter<Rml::String, Rml::Colourb>::Convert(score_parts[1], colour) &&
					Rml::TypeConverter<Rml::String, int>::Convert(score_parts[2], wave) &&
					Rml::TypeConverter<Rml::String, int>::Convert(score_parts[3], score))
				{
					SubmitScore(score_parts[0], colour, wave, score);
				}
			}
		}

		fclose(scores_file);
	}
}

void HighScores::SaveScores()
{
	FILE* scores_file = fopen("scores.txt", "wt");

	if (scores_file)
	{
		for (const Score& score : scores)
		{
			Rml::String colour_string;
			Rml::TypeConverter<Rml::Colourb, Rml::String>::Convert(score.colour, colour_string);

			Rml::String score_str = Rml::CreateString("%s\t%s\t%d\t%d\n", score.name.c_str(), colour_string.c_str(), score.wave, score.score);
			fputs(score_str.c_str(), scores_file);
		}

		fclose(scores_file);
	}
}
