#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

Rml::ElementDocument* document = nullptr;

void DeactivateAllThemes(Rml::Context* context)
{
	context->ActivateTheme("dark", false);
	context->ActivateTheme("high-contrast", false);
}

void ApplyTheme(Rml::Context* context, const char* theme_name)
{
	DeactivateAllThemes(context);
	if (theme_name)
		context->ActivateTheme(theme_name, true);
	Rml::Log::Message(Rml::Log::LT_INFO, "Active theme: %s", theme_name ? theme_name : "default");
}

// HSL → RGB. h in [0,360), s and l in [0,1]. Output channels in [0,255].
void HslToRgb(float h, float s, float l, int& r, int& g, int& b)
{
	const auto channel = [&](float n) {
		const float k = std::fmod(n + h / 30.0f, 12.0f);
		const float a = s * std::min(l, 1.0f - l);
		return l - a * std::max(-1.0f, std::min({k - 3.0f, 9.0f - k, 1.0f}));
	};
	r = static_cast<int>(std::round(channel(0) * 255.0f));
	g = static_cast<int>(std::round(channel(8) * 255.0f));
	b = static_cast<int>(std::round(channel(4) * 255.0f));
}

void RandomizeAccent(Rml::Context* context)
{
	// HSL with random hue, high saturation, and theme-shifted lightness so the random accent reads as a proper accent
	// against the active theme's background.
	const float hue = static_cast<float>(std::rand() % 360);
	const float saturation = 0.78f + (std::rand() % 23) * 0.01f;

	float lightness;
	if (context->IsThemeActive("high-contrast"))
		lightness = 0.50f;
	else if (context->IsThemeActive("dark"))
		lightness = 0.58f + (std::rand() % 14) * 0.01f;
	else
		lightness = 0.36f + (std::rand() % 14) * 0.01f;

	int r, g, b;
	HslToRgb(hue, saturation, lightness, r, g, b);
	char buf[16];
	std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
	document->SetProperty("--accent", buf);
	Rml::Log::Message(Rml::Log::LT_INFO, "Accent override set to %s (HSL %.0f/%.2f/%.2f, use 'C' to clear)", buf, hue, saturation, lightness);
}

void ClearAccentOverride()
{
	document->RemoveProperty("--accent");
	Rml::Log::Message(Rml::Log::LT_INFO, "Accent override cleared, falling back to RCSS-declared value");
}

bool ProcessKey(Rml::Context* context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority)
{
	switch (key)
	{
	case Rml::Input::KI_1: ApplyTheme(context, "dark"); return false;
	case Rml::Input::KI_2: ApplyTheme(context, nullptr); return false;
	case Rml::Input::KI_3: ApplyTheme(context, "high-contrast"); return false;
	case Rml::Input::KI_R: RandomizeAccent(context); return false;
	case Rml::Input::KI_C: ClearAccentOverride(); return false;
	case Rml::Input::KI_ESCAPE: Backend::RequestExit(); return false;
	default: return Shell::ProcessKeyDownShortcuts(context, key, key_modifier, native_dp_ratio, priority);
	}
}

} // namespace

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int window_width = 1024;
	const int window_height = 768;

	if (!Shell::Initialize())
		return -1;

	if (!Backend::Initialize("RCSS Variables Sample", window_width, window_height, false))
	{
		Shell::Shutdown();
		return -1;
	}

	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	Rml::Initialise();

	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);

	Shell::LoadFonts();

	document = context->LoadDocument("basic/variables/data/variables.rml");
	if (document)
		document->Show();

	context->ActivateTheme("dark", true);

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &ProcessKey, true);
		context->Update();
		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	Rml::Shutdown();
	Backend::Shutdown();
	Shell::Shutdown();
	return 0;
}
