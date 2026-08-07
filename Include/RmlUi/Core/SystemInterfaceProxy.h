#pragma once

#include "Debug.h"
#include "SystemInterface.h"

namespace Rml {

/**
    Header-only helper class which forwards system interface calls to an upstream system interface.

    Derive from this class to intercept individual system interface calls, such as logging, without having to
    reimplement the full interface.
 */

class SystemInterfaceProxy : public SystemInterface {
public:
	SystemInterfaceProxy(SystemInterface* upstream) : upstream(upstream) { RMLUI_ASSERT(upstream); }
	virtual ~SystemInterfaceProxy() { upstream = nullptr; }

	double GetElapsedTime() override { return upstream->GetElapsedTime(); }
	int TranslateString(String& translated, const String& input) override { return upstream->TranslateString(translated, input); }
	void JoinPath(String& translated_path, const String& document_path, const String& path) override
	{
		upstream->JoinPath(translated_path, document_path, path);
	}
	bool LogMessage(Log::Type type, const String& message) override { return upstream->LogMessage(type, message); }
	void SetMouseCursor(const String& cursor_name) override { upstream->SetMouseCursor(cursor_name); }
	void SetClipboardText(const String& text) override { upstream->SetClipboardText(text); }
	void GetClipboardText(String& text) override { upstream->GetClipboardText(text); }
	void ActivateKeyboard(Vector2f caret_position, float line_height) override { upstream->ActivateKeyboard(caret_position, line_height); }
	void DeactivateKeyboard() override { upstream->DeactivateKeyboard(); }

protected:
	SystemInterface* upstream;
};

} // namespace Rml
