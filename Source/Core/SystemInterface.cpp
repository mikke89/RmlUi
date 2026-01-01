#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/URL.h"
#include "LogDefault.h"
#include <chrono>

namespace Rml {

static String& GlobalClipBoardText()
{
	static String clipboard_text;
	return clipboard_text;
}

SystemInterface::SystemInterface() {}

SystemInterface::~SystemInterface() {}

double SystemInterface::GetElapsedTime()
{
	static const auto start = std::chrono::steady_clock::now();
	const auto current = std::chrono::steady_clock::now();
	std::chrono::duration<double> diff = current - start;
	return diff.count();
}

bool SystemInterface::LogMessage(Log::Type type, const String& message)
{
	return LogDefault::LogMessage(type, message);
}

void SystemInterface::SetMouseCursor(const String& /*cursor_name*/) {}

void SystemInterface::SetClipboardText(const String& text)
{
	// The default implementation will only copy and paste within the application
	GlobalClipBoardText() = text;
}

void SystemInterface::GetClipboardText(String& text)
{
	text = GlobalClipBoardText();
}

int SystemInterface::TranslateString(String& translated, const String& input)
{
	translated = input;
	return 0;
}

void SystemInterface::JoinPath(String& translated_path, const String& document_path, const String& path)
{
	// If the path is absolute, strip the leading / and return it.
	if (path.size() > 0 && path[0] == '/')
	{
		translated_path = path.substr(1);
		return;
	}

	// If the path is a Windows-style absolute path, return it directly.
	size_t drive_pos = path.find(':');
	size_t slash_pos = Math::Min(path.find('/'), path.find('\\'));
	if (drive_pos != String::npos && drive_pos < slash_pos)
	{
		translated_path = path;
		return;
	}

	using StringUtilities::Replace;

	// Strip off the referencing document name.
	translated_path = document_path;
	translated_path = Replace(translated_path, '\\', '/');
	size_t file_start = translated_path.rfind('/');
	if (file_start != String::npos)
		translated_path.resize(file_start + 1);
	else
		translated_path.clear();

	// Append the paths and send through URL to removing any '..'.
	URL url(Replace(translated_path, ':', '|') + Replace(path, '\\', '/'));
	translated_path = Replace(url.GetPathedFileName(), '|', ':');
}

void SystemInterface::ActivateKeyboard(Rml::Vector2f /*caret_position*/, float /*line_height*/) {}

void SystemInterface::DeactivateKeyboard() {}

} // namespace Rml
