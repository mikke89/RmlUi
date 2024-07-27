/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_DEBUGGER_ELEMENTLOG_H
#define RMLUI_DEBUGGER_ELEMENTLOG_H

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

class DebuggerSystemInterface;

/**
    @author Robert Curry
 */

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

#endif
