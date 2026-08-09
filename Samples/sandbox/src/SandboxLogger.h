#pragma once

#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Types.h>
#include <utility>

class SandboxLogger {
public:
	void Log(Rml::Log::Type type, const Rml::String& message)
	{
		static const char* message_type_str[Rml::Log::Type::LT_MAX] = {"Always", "Error", "Assert", "Warning", "Info", "Debug"};
		Rml::String formatted_message = '[' + Rml::String(message_type_str[type]) + "]: " + message;
		messages.push_back(std::move(formatted_message));
	}

	Rml::Vector<Rml::String> PopMessages() { return std::exchange(messages, {}); }

private:
	Rml::Vector<Rml::String> messages;
};
