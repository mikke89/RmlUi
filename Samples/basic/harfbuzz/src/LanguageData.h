#ifndef LANGUAGEDATA_H
#define LANGUAGEDATA_H

#include <RmlUi/Core.h>

enum class TextFlowDirection {
	LeftToRight,
	RightToLeft,
};

struct LanguageData {
	Rml::String script_code;
	TextFlowDirection text_flow_direction;
};

using LanguageDataMap = Rml::UnorderedMap<Rml::String, LanguageData>;

#endif
