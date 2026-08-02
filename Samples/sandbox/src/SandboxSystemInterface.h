#pragma once

#include <RmlUi/Core/SystemInterfaceProxy.h>

/**
    Custom interface to intercept logs, acts as proxy to upstream system interface.
 */
class SandboxSystemInterface : public Rml::SystemInterfaceProxy {
public:
	SandboxSystemInterface(Rml::SystemInterface* upstream) : Rml::SystemInterfaceProxy(upstream) {}

	struct Message {
		Rml::Log::Type type;
		Rml::String text;
	};

	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override
	{
		if (messages.size() >= max_messages)
			messages.erase(messages.begin(), messages.begin() + (messages.size() - max_messages) + 1);

		messages.push_back(Message{type, message});

		return upstream->LogMessage(type, message);
	}

	Rml::Vector<Message> PopMessages() { return std::exchange(messages, {}); }

private:
	static constexpr size_t max_messages = 200;

	Rml::Vector<Message> messages;
};
