/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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


#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/StringUtilities.h>

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


