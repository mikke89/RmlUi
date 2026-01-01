#pragma once

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

class HighScores {
public:
	static void Initialise(Rml::Context* context);
	static void Shutdown();

	static int GetHighScore();

	/// Two functions to add a score to the chart.
	/// Adds a full score, including a name. This won't prompt the user to enter their name.
	static void SubmitScore(const Rml::String& name, const Rml::Colourb& colour, int wave, int score);
	/// Adds a score, and causes an input field to appear to request the user for their name.
	static void SubmitScore(const Rml::Colourb& colour, int wave, int score);
	/// Sets the name of the last player to submit their score.
	static void SubmitName(const Rml::String& name);

private:
	HighScores(Rml::Context* context);
	~HighScores();

	static HighScores* instance;

	void SubmitScore(const Rml::String& name, const Rml::Colourb& colour, int wave, int score, bool name_required);
	void LoadScores();
	void SaveScores();

	struct Score {
		Rml::String name;
		bool name_required;
		Rml::Colourb colour;
		int score;
		int wave;

		Rml::String GetColour() { return Rml::ToString(colour); }
	};
	using ScoreList = Rml::Vector<Score>;
	ScoreList scores;

	Rml::DataModelHandle model_handle;
};
