#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/Interpreter.h>
#include <RmlUi/Lua/Lua.h>
#include <doctest.h>

using namespace Rml;

static bool Contains(const String& haystack, const char* needle)
{
	return haystack.find(needle) != String::npos;
}

static int error_handler_calls = 0;
static String error_handler_message;
static String error_handler_lines;

static int RecordError(lua_State* L)
{
	error_handler_calls++;
	const char* message = lua_tostring(L, 1);
	error_handler_message = message ? message : "";

	// Record the current line of each Lua function on the call stack, to check that the handler runs at the point of
	// the error.
	error_handler_lines.clear();
	lua_Debug ar;
	for (int level = 1; lua_getstack(L, level, &ar); level++)
	{
		lua_getinfo(L, "l", &ar);
		if (ar.currentline > 0)
			error_handler_lines += std::to_string(ar.currentline) + " ";
	}

	lua_pushstring(L, "handled");
	return 1;
}

static int ThrowError(lua_State* L)
{
	return luaL_error(L, "error handler failed");
}

static const String lua_error_rml = R"RML(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body { font-family: LatoLatin; }
	</style>
</head>
<body data-model="error_model">
	<div id="button" onclick="error('from handler')"/>
	<div id="model_button" data-event-click="fail()"/>
</body>
</rml>
)RML";

static const String lua_file_error_rml = R"RML(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<script src="/../Tests/Data/UnitTests/lua_syntax_error.lua"/>
</head>
<body/>
</rml>
)RML";

TEST_CASE("lua.options.error_handler")
{
	TestsShell::ShutdownShell();

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	Lua::Options options;
	options.error_handler = RecordError;
	Lua::Initialise(L, options);

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	error_handler_calls = 0;
	error_handler_message.clear();
	error_handler_lines.clear();

	// The handler replaces the default logging, so none of these errors log a warning.
	SUBCASE("runtime_error")
	{
		CHECK_FALSE(Lua::Interpreter::DoString("local function inner()\n"
											   "  error('boom')\n"
											   "end\n"
											   "inner()\n",
			"test"));
		CHECK(error_handler_calls == 1);
		CHECK(Contains(error_handler_message, "boom"));
		// The stack still holds 'inner' at line 2, called from line 4.
		CHECK(error_handler_lines == "2 4 ");
	}
	SUBCASE("syntax_error")
	{
		CHECK_FALSE(Lua::Interpreter::DoString("this is not lua", "test"));
		CHECK(error_handler_calls == 1);
	}
	SUBCASE("file_syntax_error")
	{
		ElementDocument* document = context->LoadDocumentFromMemory(lua_file_error_rml);
		REQUIRE(document);
		CHECK(error_handler_calls == 1);
		CHECK(Contains(error_handler_message, "lua_syntax_error.lua"));
		document->Close();
	}
	SUBCASE("event_handler_error")
	{
		REQUIRE(Lua::Interpreter::DoString(R"(
			rmlui.contexts["main"]:OpenDataModel("error_model", {
				fail = function() error('from model') end,
			})
		)",
			"test"));

		ElementDocument* document = context->LoadDocumentFromMemory(lua_error_rml);
		REQUIRE(document);
		document->Show();
		TestsShell::RenderLoop();

		document->GetElementById("button")->Click();
		CHECK(error_handler_calls == 1);
		CHECK(Contains(error_handler_message, "from handler"));

		// Data model event callbacks go through a different call path.
		document->GetElementById("model_button")->Click();
		CHECK(error_handler_calls == 2);
		CHECK(Contains(error_handler_message, "from model"));

		document->Close();
		TestsShell::RenderLoop();
		REQUIRE(Lua::Interpreter::DoString("rmlui.contexts['main']:CloseDataModel('error_model')", "test"));
	}

	TestsShell::ShutdownShell();
	lua_close(L);
}

TEST_CASE("lua.options.error_handler_fails")
{
	TestsShell::ShutdownShell();

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	Lua::Options options;
	options.error_handler = ThrowError;
	Lua::Initialise(L, options);

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	const StringList& warnings = TestsShell::GetTestsSystemInterface()->GetLoggedWarnings();

	// Lua can't report an error through a handler that fails itself, so the plugin logs it instead.
	SUBCASE("runtime_error")
	{
		TestsShell::SetNumExpectedWarnings(1);
		CHECK_FALSE(Lua::Interpreter::DoString("error('boom')", "test"));
		REQUIRE(warnings.size() == 1);
		CHECK(Contains(warnings[0], "error in error handling"));
		TestsShell::SetNumExpectedWarnings(0);
	}
	SUBCASE("syntax_error")
	{
		// Both the syntax error and the handler's own error are logged.
		TestsShell::SetNumExpectedWarnings(1);
		CHECK_FALSE(Lua::Interpreter::DoString("this is not lua", "test"));
		REQUIRE(warnings.size() == 1);
		CHECK(Contains(warnings[0], "expected"));
		CHECK(Contains(warnings[0], "error handler failed"));
		TestsShell::SetNumExpectedWarnings(0);
	}

	TestsShell::ShutdownShell();
	lua_close(L);
}

TEST_CASE("lua.options.error_handler_unset")
{
	TestsShell::ShutdownShell();

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	Lua::Initialise(L);

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	const StringList& warnings = TestsShell::GetTestsSystemInterface()->GetLoggedWarnings();

	// Without a handler, each error is logged as a single warning.
	SUBCASE("runtime_error")
	{
		TestsShell::SetNumExpectedWarnings(1);
		CHECK_FALSE(Lua::Interpreter::DoString("error('boom')", "test"));
		REQUIRE(warnings.size() == 1);
		CHECK(Contains(warnings[0], "boom"));
		CHECK(Contains(warnings[0], "stack traceback"));
		TestsShell::SetNumExpectedWarnings(0);
	}
	SUBCASE("syntax_error")
	{
		TestsShell::SetNumExpectedWarnings(1);
		CHECK_FALSE(Lua::Interpreter::DoString("this is not lua", "test"));
		REQUIRE(warnings.size() == 1);
		CHECK(Contains(warnings[0], "expected"));
		TestsShell::SetNumExpectedWarnings(0);
	}

	TestsShell::ShutdownShell();
	lua_close(L);
}
