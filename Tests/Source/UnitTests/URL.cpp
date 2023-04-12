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

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/URL.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

class BasicSystemInterface : public Rml::SystemInterface {
public:
	double GetElapsedTime() override { return 0.0; }
	bool LogMessage(Log::Type type, const String& message) override
	{
		CHECK(type > Log::LT_WARNING);
		return Rml::SystemInterface::LogMessage(type == Log::LT_ASSERT ? Log::LT_ERROR : type, message);
	}
};

static BasicSystemInterface basic_system_interface;

static inline String Normalize(const char* url_str)
{
	URL url(url_str);
	const String rml_path = url.GetURL();
	return rml_path;
}

static inline String JoinPath(const char* document_path, const char* path)
{
	String result;
	basic_system_interface.JoinPath(result, document_path, path);
	return result;
}

TEST_CASE("url.normalize")
{
	SystemInterface* old_system_interface = Rml::GetSystemInterface();
	Rml::SetSystemInterface(&basic_system_interface);

	CHECK(Normalize("blue") == "blue");
	CHECK(Normalize("blue.png") == "blue.png");
	CHECK(Normalize("/blue.png") == "/blue.png");
	CHECK(Normalize("/data/blue.png") == "/data/blue.png");
	CHECK(Normalize("/data/gui/../../data/images/icons/blue.png") == "/data/images/icons/blue.png");

	CHECK(Normalize("../data/blue.png") == "../data/blue.png");
	CHECK(Normalize("data/blue.png") == "data/blue.png");
	CHECK(Normalize("data/../blue.png") == "blue.png");

	CHECK(Normalize("x./data/blue.png") == "x./data/blue.png");
	CHECK(Normalize("x./data/../blue.png") == "x./blue.png");
	CHECK(Normalize("/x./data/blue.png") == "/x./data/blue.png");
	CHECK(Normalize("/x./data/../blue.png") == "/x./blue.png");

	CHECK(Normalize("/data/blue.png") == "/data/blue.png");
	CHECK(Normalize("/data/../blue.png") == "/blue.png");

	CHECK(Normalize("test/data/../blue.png") == "test/blue.png");
	CHECK(Normalize("data/../blue.png") == "blue.png");
	CHECK(Normalize("data/gui/../../data/images/icons/blue.png") == "data/images/icons/blue.png");
	CHECK(Normalize("data/gui/images/../../blue.png") == "data/blue.png");

	CHECK(Normalize("file://data/blue.png") == "file://data/blue.png");

	/*** These ones are supported now, but we may want to revise them later ***/

	CHECK(Normalize("") == "");
	CHECK(Normalize("./") == "./");
	CHECK(Normalize("../") == "../");
	CHECK(Normalize("/") == "/");
	CHECK(Normalize("/../") == "/../");

	/*** We may want to support these later. ***/

	// CHECK(Normalize("./data/blue.png") == "data/blue.png");
	// CHECK(Normalize("./data/../blue.png") == "blue.png");

	// CHECK(Normalize("/./data/blue.png") == "/data/blue.png");
	// CHECK(Normalize("/./data/../blue.png") == "/blue.png");

	// CHECK(Normalize("data/../../blue.png") == "../blue.png");
	// CHECK(Normalize("data/../../../blue.png") == "../../blue.png");
	// CHECK(Normalize("./data/../../blue.png") == "../blue.png");
	// CHECK(Normalize("./data/../../../blue.png") == "../../blue.png");

	// CHECK(Normalize("data/gui/../../data/images/icons/blue.png") == "data/images/icons/blue.png");

	// CHECK(Normalize("data//blue.png") == "data/blue.png");
	// CHECK(Normalize("data//./blue.png") == "data/blue.png");
	// CHECK(Normalize("data//../blue.png") == "blue.png");

	// CHECK(Normalize("file://data/../blue.png") == "file://data/blue.png");
	// CHECK(Normalize("file:///data/../blue.png") == "file:///blue.png");

	// CHECK(Normalize("file://data/blue.png") == "file://data/blue.png");
	// CHECK(Normalize("file://data/blue.png") == "file://data/blue.png");
	// CHECK(Normalize("file://data/../blue.png") == "file://blue.png");

	// CHECK(Normalize("C:/data/blue.png") == "C:/data/blue.png");
	// CHECK(Normalize("C:/data/./blue.png") == "C:/data/blue.png");
	// CHECK(Normalize("C:/data./blue.png") == "C:/data./blue.png");
	// CHECK(Normalize("C:/data/../blue.png") == "C:/blue.png");

	// CHECK(Normalize(R"(C:\data\blue.png)") == "C:/data/blue.png");
	// CHECK(Normalize(R"(C:\data\.\blue.png)") == "C:/data/blue.png");
	// CHECK(Normalize(R"(C:\data.\blue.png)") == "C:/data./blue.png");
	// CHECK(Normalize(R"(C:\data\..\blue.png)") == "C:/blue.png");

	Rml::SetSystemInterface(old_system_interface);
}

TEST_CASE("url.join")
{
	SystemInterface* old_system_interface = Rml::GetSystemInterface();
	Rml::SetSystemInterface(&basic_system_interface);

	CHECK(JoinPath("data/gui/d.rml", "blue.png") == "data/gui/blue.png");
	CHECK(JoinPath("data/gui/d.rml", "../../data/images/icons/blue.png") == "data/images/icons/blue.png");
	CHECK(JoinPath("/data/d.rml", "../images/icons/blue.png") == "/images/icons/blue.png");

	CHECK(JoinPath(R"(C:\data\d.rml)", R"(blue.png)") == R"(C:/data/blue.png)");
	CHECK(JoinPath(R"(C:\data\d.rml)", R"(../blue.png)") == R"(C:/blue.png)");
	CHECK(JoinPath(R"(C:\data\d.rml)", R"(..\blue.png)") == R"(C:/blue.png)");

	CHECK(JoinPath("file://C:/data/d.rml", "img/blue.png") == "file://C:/data/img/blue.png");
	CHECK(JoinPath("file://data/d.rml", "img/blue.png") == "file://data/img/blue.png");
	CHECK(JoinPath("file://C:/data/d.rml", "img/../blue.png") == "file://C:/data/blue.png");
	CHECK(JoinPath("file://data/d.rml", "img/../../blue.png") == "file://blue.png");

	CHECK(JoinPath("file://data/d.rml", "file://C:/data/blue.png") == "file://C:/data/blue.png");
	CHECK(JoinPath("file://data/d.rml", "file://data/blue.png") == "file://data/blue.png");

	CHECK(JoinPath("file:///data/d.rml", "../blue.png") == "file:///blue.png");
	CHECK(JoinPath("file:///data/d.rml", "img/../blue.png") == "file:///data/blue.png");

	/*** These ones are supported now, but we may want to revise them later ***/

	CHECK(JoinPath("/d.rml", "../data/images/icons/blue.png") == "/../data/images/icons/blue.png");
	CHECK(JoinPath("/data/d.rml", "../../images/icons/blue.png") == "/../images/icons/blue.png");

	/*** We may want to support these later ***/

	// CHECK(JoinPath("data/gui/d.rml", "/images/icons/blue.png") == "/images/icons/blue.png");
	// CHECK(JoinPath("/data/gui/d.rml", "/images/icons/blue.png") == "/images/icons/blue.png");

	// CHECK(JoinPath("data/gui/d.rml", "/images/../icons/blue.png") == "/icons/blue.png");
	// CHECK(JoinPath("data/gui/d.rml", "/images/./icons/../../blue.png") == "/blue.png");
	// CHECK(JoinPath("/data/gui/d.rml", "/../../images/icons/../blue.png") == "/../../images/blue.png");
	// CHECK(JoinPath("/data/gui/d.rml", "/images/./icons/blue.png") == "/images/icons/blue.png");

	// CHECK(JoinPath("data/gui/d.rml", "./../images/icons/blue.png") == "data/images/icons/blue.png");
	// CHECK(JoinPath("/d.rml", "./data/images/icons/blue.png") == "/data/images/icons/blue.png");
	// CHECK(JoinPath("/data/d.rml", "./images/icons/blue.png") == "/data/images/icons/blue.png");
	// CHECK(JoinPath("/data/d.rml", "./../images/icons/blue.png") == "/images/icons/blue.png");
	// CHECK(JoinPath("/data/d.rml", "./../../images/icons/blue.png") == "/../images/icons/blue.png");

	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/blue.png)") == R"(/blue.png)");

	// CHECK(JoinPath("file://C:/data/d.rml", "/img/blue.png") == "file://C:/img/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/blue.png") == "file://data/img/blue.png");
	// CHECK(JoinPath("file://C:/data/d.rml", "/img/../blue.png") == "file://C:/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/../../blue.png") == "file://data/blue.png");

	// CHECK(JoinPath("file://data/d.rml", "file://C:/data/../blue.png") == "file://C:/blue.png");

	// CHECK(JoinPath("file://data/d.rml", "file://data/./blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "file://data/../blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "file:///data/../blue.png") == "file:///blue.png");

	// CHECK(JoinPath("file://data/d.rml", "../blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "img/../blue.png") == "file://data/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/blue.png") == "file://data/img/blue.png");
	// CHECK(JoinPath("file://data/d.rml", "/img/../blue.png") == "file://data/blue.png");

	// CHECK(JoinPath("file:///data/d.rml", "/img/blue.png") == "file:///img/blue.png");
	// CHECK(JoinPath("file:///data/d.rml", "/img/../blue.png") == "file:///blue.png");

	// CHECK(JoinPath("file:data/d.rml", "../blue.png") == "file:blue.png");
	// CHECK(JoinPath("file:data/d.rml", "img/../blue.png") == "file:data/blue.png");
	// CHECK(JoinPath("file:data/d.rml", "/img/blue.png") == "file:img/blue.png");
	// CHECK(JoinPath("file:data/d.rml", "/img/../blue.png") == "file:blue.png");

	// CHECK(JoinPath("file:C:/data/d.rml", "../blue.png") == "file:C:/blue.png");
	// CHECK(JoinPath("file:C:/data/d.rml", "img/../blue.png") == "file:C:/data/blue.png");
	// CHECK(JoinPath("file:C:/data/d.rml", "/img/blue.png") == "file:img/blue.png");
	// CHECK(JoinPath("file:C:/data/d.rml", "/img/../blue.png") == "file:blue.png");

	/*** Not sure about these, maybe report as invalid? ***/

	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(../../blue.png)") == R"(C:/blue.png)");
	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/../blue.png)") == R"(/../blue.png)");
	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/..\blue.png)") == R"(/../blue.png)");
	// CHECK(JoinPath(R"(C:\data\d.rml)", R"(/../../blue.png)") == R"(/../../blue.png)");

	Rml::SetSystemInterface(old_system_interface);
}
