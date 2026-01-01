#pragma once

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

class DebuggerSystemInterface;

class ElementLog : public ElementDebugDocument, public Rml::EventListener {
public:
	RMLUI_RTTI_DefineWithParent(ElementLog, ElementDebugDocument)

	ElementLog(const String& tag);
	~ElementLog();

	/// Initialises the log element.
	/// @return True if the element initialised successfully, false otherwise.
	bool Initialise();

	/// Adds a log message to the debug log.
	void AddLogMessage(Log::Type type, const String& message);

protected:
	void OnUpdate() override;
	void ProcessEvent(Event& event) override;

private:
	struct LogMessage {
		unsigned int index;
		String message;
	};
	using LogMessageList = Vector<LogMessage>;

	struct LogType {
		bool visible;
		String class_name;
		String alert_contents;
		String button_name;
		LogMessageList log_messages;
	};
	LogType log_types[Log::LT_MAX];

	int FindNextEarliestLogType(unsigned int log_pointers[Log::LT_MAX]);

	unsigned int current_index;
	bool dirty_logs;
	bool auto_scroll;
	Element* message_content;
	ElementDocument* beacon;
	int current_beacon_level;
};

} // namespace Debugger
} // namespace Rml
