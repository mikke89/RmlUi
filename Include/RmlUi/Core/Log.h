#pragma once

#include "Header.h"
#include "Types.h"

namespace Rml {

/**
    RmlUi logging API.
 */

class RMLUICORE_API Log {
public:
	enum Type {
		LT_ALWAYS = 0,
		LT_ERROR,
		LT_ASSERT,
		LT_WARNING,
		LT_INFO,
		LT_DEBUG,
		LT_MAX,
	};

	/// Log the specified message via the registered log interface
	/// @param[in] type Type of message.
	/// @param[in] format The message, with sprintf-style parameters.
	static void Message(Type type, const char* format, ...) RMLUI_ATTRIBUTE_FORMAT_PRINTF(2, 3);

	/// Log a parse error on the specified file and line number.
	/// @param[in] filename Name of the file with the parse error.
	/// @param[in] line_number Line the error occurred on.
	/// @param[in] format The error message, with sprintf-style parameters.
	static void ParseError(const String& filename, int line_number, const char* format, ...) RMLUI_ATTRIBUTE_FORMAT_PRINTF(3, 4);
};

} // namespace Rml
