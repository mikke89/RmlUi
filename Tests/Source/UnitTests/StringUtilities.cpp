#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("StringUtilities::TrimTrailingDotZeros")
{
	auto RunTrimTrailingDotZeros = [](String string) {
		StringUtilities::TrimTrailingDotZeros(string);
		return string;
	};

	CHECK(RunTrimTrailingDotZeros("0.1") == "0.1");
	CHECK(RunTrimTrailingDotZeros("0.10") == "0.1");
	CHECK(RunTrimTrailingDotZeros("0.1000") == "0.1");
	CHECK(RunTrimTrailingDotZeros("0.01") == "0.01");
	CHECK(RunTrimTrailingDotZeros("0.") == "0");
	CHECK(RunTrimTrailingDotZeros("5.") == "5");
	CHECK(RunTrimTrailingDotZeros("5.5") == "5.5");
	CHECK(RunTrimTrailingDotZeros("5.50") == "5.5");
	CHECK(RunTrimTrailingDotZeros("5.501") == "5.501");
	CHECK(RunTrimTrailingDotZeros("10.0") == "10");
	CHECK(RunTrimTrailingDotZeros("11.0") == "11");

	// Some test cases for behavior that are probably not what you want.
	WARN(RunTrimTrailingDotZeros("test0") == "test");
	WARN(RunTrimTrailingDotZeros("1000") == "1");
	WARN(RunTrimTrailingDotZeros(".") == "");
	WARN(RunTrimTrailingDotZeros("0") == "");
	WARN(RunTrimTrailingDotZeros(".0") == "");
	WARN(RunTrimTrailingDotZeros(" 11 2121 3.00") == " 11 2121 3");
	WARN(RunTrimTrailingDotZeros("11") == "11");
}

TEST_CASE("StringUtilities::StartsWith")
{
	using namespace Rml::StringUtilities;

	CHECK(StartsWith("abc", "abc"));
	CHECK(StartsWith("abc", "ab"));
	CHECK(StartsWith("abc", "a"));
	CHECK(StartsWith("abc", ""));

	CHECK(!StartsWith("abc", "abcd"));
	CHECK(!StartsWith("abc", "abd"));
	CHECK(!StartsWith("abc", "bbc"));
	CHECK(!StartsWith("abc", "bc"));
	CHECK(!StartsWith("abc", "x"));
}

TEST_CASE("StringUtilities::EndsWith")
{
	using namespace Rml::StringUtilities;

	CHECK(EndsWith("abc", "abc"));
	CHECK(EndsWith("abc", "bc"));
	CHECK(EndsWith("abc", "c"));
	CHECK(EndsWith("abc", ""));

	CHECK(!EndsWith("abc", "abcd"));
	CHECK(!EndsWith("abc", "abd"));
	CHECK(!EndsWith("abc", "bbc"));
	CHECK(!EndsWith("abc", "ab"));
	CHECK(!EndsWith("abc", "x"));
}

TEST_CASE("StringView")
{
	const char abc[] = "abc";
	const String str_abc = abc;

	CHECK(StringView("abc") == StringView("abc"));
	CHECK(StringView("abc") == String("abc"));
	CHECK(StringView("abc") == "abc");
	CHECK(StringView("abc") == abc);

	CHECK(StringView(String(abc)) == abc);
	CHECK(StringView(abc) == abc);
	CHECK(StringView(abc, abc + 3) == abc);
	CHECK(StringView(str_abc, 1) == "bc");
	CHECK(StringView(str_abc, 1, 1) == "b");

	CHECK(StringView("abcd") != abc);
	CHECK(StringView("ab") != abc);
	CHECK(StringView() != abc);

	CHECK(StringView() == String());
	CHECK(StringView() == "");
}

TEST_CASE("StringUtilities::ConvertByteOffsetToCharacterOffset")
{
	using namespace Rml::StringUtilities;

	// clang-format off
	CHECK(ConvertByteOffsetToCharacterOffset("", 0) == 0);
	CHECK(ConvertByteOffsetToCharacterOffset("", 1) == 0);
    CHECK(ConvertByteOffsetToCharacterOffset("a", 0) == 0);
    CHECK(ConvertByteOffsetToCharacterOffset("a", 1) == 1);
	CHECK(ConvertByteOffsetToCharacterOffset("ab", 1) == 1);
	CHECK(ConvertByteOffsetToCharacterOffset("ab", 2) == 2);

	CHECK(ConvertByteOffsetToCharacterOffset("a\xC2\xA3" "b", 1) == 1);
	CHECK(ConvertByteOffsetToCharacterOffset("a\xC2\xA3" "b", 2) == 2);
	CHECK(ConvertByteOffsetToCharacterOffset("a\xC2\xA3" "b", 3) == 2);
	CHECK(ConvertByteOffsetToCharacterOffset("a\xC2\xA3" "b", 4) == 3);

	CHECK(ConvertByteOffsetToCharacterOffset("a\xE2\x82\xAC" "b", 2) == 2);
	CHECK(ConvertByteOffsetToCharacterOffset("a\xE2\x82\xAC" "b", 3) == 2);
	CHECK(ConvertByteOffsetToCharacterOffset("a\xE2\x82\xAC" "b", 4) == 2);
	CHECK(ConvertByteOffsetToCharacterOffset("a\xE2\x82\xAC" "b", 5) == 3);
	// clang-format on
}

TEST_CASE("StringUtilities::ConvertCharacterOffsetToByteOffset")
{
	using namespace Rml::StringUtilities;

	// clang-format off
	CHECK(ConvertCharacterOffsetToByteOffset("", 0) == 0);
	CHECK(ConvertCharacterOffsetToByteOffset("", 1) == 0);
    CHECK(ConvertCharacterOffsetToByteOffset("a", 0) == 0);
    CHECK(ConvertCharacterOffsetToByteOffset("a", 1) == 1);
	CHECK(ConvertCharacterOffsetToByteOffset("ab", 1) == 1);
	CHECK(ConvertCharacterOffsetToByteOffset("ab", 2) == 2);

	CHECK(ConvertCharacterOffsetToByteOffset("a\xC2\xA3" "b", 1) == 1);
	CHECK(ConvertCharacterOffsetToByteOffset("a\xC2\xA3" "b", 2) == 3);
	CHECK(ConvertCharacterOffsetToByteOffset("a\xC2\xA3" "b", 3) == 4);
	CHECK(ConvertCharacterOffsetToByteOffset("a\xC2\xA3" "b", 4) == 4);

	CHECK(ConvertCharacterOffsetToByteOffset("a\xE2\x82\xAC" "b", 1) == 1);
	CHECK(ConvertCharacterOffsetToByteOffset("a\xE2\x82\xAC" "b", 2) == 4);
	CHECK(ConvertCharacterOffsetToByteOffset("a\xE2\x82\xAC" "b", 3) == 5);
	CHECK(ConvertCharacterOffsetToByteOffset("a\xE2\x82\xAC" "b", 4) == 5);
	// clang-format on
}

TEST_CASE("CreateString")
{
	CHECK(Rml::CreateString("Hello %s!", "world") == "Hello world!");
	CHECK(Rml::CreateString("%g, %d, %.2f", 0.5f, 5, 2.f) == "0.5, 5, 2.00");

	constexpr int InternalBufferSize = 256;
	for (int string_size : {InternalBufferSize - 1, InternalBufferSize, InternalBufferSize + 1})
	{
		Rml::String large_string(string_size, 'x');
		CHECK(Rml::CreateString("%s", large_string.c_str()) == large_string);
	}
}

TEST_CASE("FormatString")
{
	{
		Rml::String result;
		int length = Rml::FormatString(result, "Hello %s!", "world");
		CHECK(result == "Hello world!");
		CHECK(length == 12);
	}

	constexpr int InternalBufferSize = 256;
	for (int string_size : {InternalBufferSize - 1, InternalBufferSize, InternalBufferSize + 1})
	{
		const Rml::String large_string(string_size, 'x');
		Rml::String result;
		int length = Rml::FormatString(result, "%s", large_string.c_str());
		CHECK(result == large_string);
		CHECK(length == string_size);
	}
}
