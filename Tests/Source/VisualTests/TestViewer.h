#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

namespace Rml {
class Context;
class ElementDocument;
} // namespace Rml

enum class SourceType { None, Test, Reference };

class TestViewer {
public:
	TestViewer(Rml::Context* context);
	~TestViewer();

	void ShowSource(SourceType type);
	void ShowHelp(bool show);
	bool IsHelpVisible() const;
	bool IsNavigationLocked() const;

	bool LoadTest(const Rml::String& directory, const Rml::String& filename, int test_index, int number_of_tests, int filtered_test_index,
		int filtered_number_of_tests, int suite_index, int number_of_suites, bool keep_scroll_position = false);

	void SetGoToText(const Rml::String& rml);
	Rml::Rectanglef GetGoToArea() const;

	void SetAttention(bool active);

private:
	Rml::Context* context;

	Rml::ElementDocument* document_test = nullptr;
	Rml::ElementDocument* document_description = nullptr;
	Rml::ElementDocument* document_source = nullptr;
	Rml::ElementDocument* document_reference = nullptr;
	Rml::ElementDocument* document_help = nullptr;

	Rml::String source_test;
	Rml::String source_reference;

	Rml::String reference_filename;
};
