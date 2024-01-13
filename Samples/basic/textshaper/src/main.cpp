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

#include "FontEngineInterfaceTextShaper.h"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

/*
	This demo shows how to create a custom text-shaping font engine implementation using HarfBuzz.
*/

// Toggle this variable to enable/disable text shaping.
constexpr bool EnableTextShaping = true;

class TextShaperEventListener : public Rml::EventListener {
public:
	TextShaperEventListener(const Rml::String& value, Rml::Element* element) : value(value), lorem_ipsum_element(element) {}

	void ProcessEvent(Rml::Event& /*event*/) override
	{
		if (value == "set-english")
		{
			lorem_ipsum_element->SetAttribute("lang", "en");
			lorem_ipsum_element->SetAttribute("dir", "ltr");
			lorem_ipsum_element->SetInnerRML(
				"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Urna "
				"neque viverra justo nec ultrices dui sapien eget mi. Risus quis varius quam quisque id. Amet est placerat in egestas erat "
				"imperdiet. Velit egestas dui id ornare arcu odio ut sem. Aliquet porttitor lacus luctus accumsan tortor posuere. Et malesuada fames "
				"ac turpis egestas integer eget. Enim nunc faucibus a pellentesque sit amet porttitor eget. Nunc pulvinar sapien et ligula. Sit amet "
				"mattis vulputate enim nulla aliquet porttitor lacus luctus. Dolor sit amet consectetur adipiscing. Congue eu consequat ac felis "
				"donec et odio pellentesque. Nunc non blandit massa enim nec dui nunc mattis.");
		}
		else if (value == "set-arabic")
		{
			lorem_ipsum_element->SetAttribute("lang", "ar");
			lorem_ipsum_element->SetAttribute("dir", "rtl");
			lorem_ipsum_element->SetInnerRML(
				u8"هناك حقيقة مثبتة منذ زمن طويل وهي أن المحتوى المقروء لصفحة ما سيلهي القارئ عن التركيز على الشكل الخارجي للنص أو شكل توضع الفقرات "
				u8"في الصفحة التي يقرأها. ولذلك يتم استخدام طريقة لوريم إيبسوم لأنها تعطي توزيعاَ طبيعياَ -إلى حد ما- للأحرف عوضاً عن استخدام هنا يوجد "
				u8"محتوى نصي، هنا يوجد محتوى نصي فتجعلها تبدو (أي الأحرف) وكأنها نص مقروء. العديد من برامح النشر المكتبي وبرامح تحرير صفحات الويب "
				u8"تستخدم لوريم إيبسوم بشكل إفتراضي كنموذج عن النص، وإذا قمت بإدخال \"muspi merol\" في أي محرك بحث ستظهر العديد من المواقع الحديثة "
				u8"العهد في نتائج البحث. على مدى السنين ظهرت نسخ جديدة ومختلفة من نص لوريم إيبسوم، أحياناً عن طريق الصدفة، وأحياناً عن عمد كإدخال بعض "
				u8"العبارات الفكاهية إليها.");
		}
		else if (value == "set-hindi")
		{
			lorem_ipsum_element->SetAttribute("lang", "hi");
			lorem_ipsum_element->SetAttribute("dir", "ltr");
			lorem_ipsum_element->SetInnerRML(
				u8"यह एक लंबा स्थापित तथ्य है कि जब एक पाठक एक पृष्ठ के खाखे को देखेगा तो पठनीय सामग्री से विचलित हो जाएगा. Lorem Ipsum का उपयोग करने का मुद्दा यह "
			    u8"है कि इसमें एक और अधिक या कम अक्षरों का सामान्य वितरण किया गया है, 'Content here, content here' प्रयोग करने की जगह इसे पठनीय English के रूप में "
			    u8"प्रयोग किया जाये. अब कई डेस्कटॉप प्रकाशन संकुल और वेब पेज संपादक उनके डिफ़ॉल्ट मॉडल पाठ के रूप में Lorem Ipsum उपयोग करते हैं, और अब \"Lorem Ipsum\" "
			    u8"के लिए खोज अपने शैशव में कई वेब साइटों को उजागर करती है. इसके विभिन्न संस्करणों का वर्षों में विकास हुआ है, कभी दुर्घटना से, तो कभी प्रयोजन पर (हास्य और "
			    u8"लगाव डालने के लिए).");
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
	Rml::String value;
	Rml::Element* lorem_ipsum_element;
};

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	int window_width = 1024;
	int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Text Shaper Sample", window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	// Construct and load the font interface.
	Rml::UniquePtr<FontEngineInterfaceTextShaper> font_interface = nullptr;
	if (EnableTextShaping)
	{
		font_interface = Rml::MakeUnique<FontEngineInterfaceTextShaper>();
		Rml::SetFontEngineInterface(font_interface.get());

		// Add language data to the font engine's internal language lookup table.
		font_interface->RegisterLanguage("en", "Latn", TextFlowDirection::LeftToRight);
		font_interface->RegisterLanguage("ar", "Arab", TextFlowDirection::RightToLeft);
		font_interface->RegisterLanguage("hi", "Deva", TextFlowDirection::LeftToRight);
	}

	// RmlUi initialisation.
	Rml::Initialise();

	// Create the main RmlUi context.
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);

	// Load required fonts.
	Rml::String font_paths[3] = {
		"assets/LatoLatin-Regular.ttf",
		"basic/textshaper/data/Cairo-Regular.ttf",
		"basic/textshaper/data/Poppins-Regular.ttf"
	};
	for (const Rml::String& font_path : font_paths)
		if (!Rml::LoadFontFace(font_path))
		{
			Rml::Shutdown();
			Backend::Shutdown();
			Shell::Shutdown();
			return -1;
		}

	// Load and show the demo document.
	if (Rml::ElementDocument* document = context->LoadDocument("basic/textshaper/data/textshaper.rml"))
	{
		if (auto el = document->GetElementById("title"))
			el->SetInnerRML("Text Shaping");

		document->Show();

		// Create event handlers.
		Rml::Element* lorem_ipsum_element = document->GetElementById("lorem-ipsum");
		for (const Rml::String& button_id : {"set-english", "set-arabic", "set-hindi"})
			document->GetElementById(button_id)->AddEventListener(Rml::EventId::Click, new TextShaperEventListener(button_id, lorem_ipsum_element));
	}

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	// Shut down debugger before font interface.
	Rml::Debugger::Shutdown();
	if (EnableTextShaping)
		font_interface.reset();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
