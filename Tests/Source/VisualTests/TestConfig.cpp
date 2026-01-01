#include "TestConfig.h"
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/Types.h>
#include <PlatformExtensions.h>
#include <Shell.h>
#include <cstdlib>

Rml::String GetCompareInputDirectory()
{
	Rml::String input_directory;

	if (const char* env_variable = std::getenv("RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY"))
		input_directory = env_variable;
	else
		input_directory = PlatformExtensions::FindSamplesRoot() + "../Tests/Output";

	return input_directory;
}

Rml::String GetCaptureOutputDirectory()
{
	Rml::String output_directory;

	if (const char* env_variable = std::getenv("RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY"))
		output_directory = env_variable;
	else
		output_directory = PlatformExtensions::FindSamplesRoot() + "../Tests/Output";

	return output_directory;
}

Rml::StringList GetTestInputDirectories()
{
	const Rml::String samples_root = PlatformExtensions::FindSamplesRoot();

	Rml::StringList directories = {samples_root + "../Tests/Data/VisualTests"};

	if (const char* env_variable = std::getenv("RMLUI_VISUAL_TESTS_RML_DIRECTORIES"))
		Rml::StringUtilities::ExpandString(directories, env_variable, ',');

	return directories;
}
