#pragma once

#include "SandboxLogger.h"
#include <RmlUi/Core/SystemInterfaceProxy.h>

/**
    Custom interface to intercept logs, acts as proxy to upstream system interface.
 */
class SandboxSystemInterface : public Rml::SystemInterfaceProxy {
public:
	SandboxSystemInterface(Rml::SystemInterface* upstream, SandboxLogger* logger) : Rml::SystemInterfaceProxy(upstream), logger(logger) {}

	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override
	{
		logger->Log(type, message);
		return upstream->LogMessage(type, message);
	}

private:
	SandboxLogger* logger;
};
