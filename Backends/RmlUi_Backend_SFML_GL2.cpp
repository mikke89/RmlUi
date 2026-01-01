#include "RmlUi_Backend.h"
#include "RmlUi_Platform_SFML.h"
#include "RmlUi_Renderer_GL2.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Profiling.h>
#include <RmlUi/Debugger/Debugger.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cstdint>

#if SFML_VERSION_MAJOR >= 3
	#define SFML_COORDINATE(x, y) {x, y}
#else
	#define SFML_COORDINATE(x, y) x, y
#endif

/**
    Custom render interface example for the SFML/GL2 backend.

    Overloads the OpenGL2 render interface to load textures through SFML's built-in texture loading functionality.
 */
class RenderInterface_GL2_SFML : public RenderInterface_GL2 {
public:
	// -- Inherited from Rml::RenderInterface --

	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override
	{
		if (texture)
		{
			sf::Texture::bind((sf::Texture*)texture);
			texture = RenderInterface_GL2::TextureEnableWithoutBinding;
		}

		RenderInterface_GL2::RenderGeometry(handle, translation, texture);
	}

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override
	{
		Rml::FileInterface* file_interface = Rml::GetFileInterface();
		Rml::FileHandle file_handle = file_interface->Open(source);
		if (!file_handle)
			return false;

		file_interface->Seek(file_handle, 0, SEEK_END);
		size_t buffer_size = file_interface->Tell(file_handle);
		file_interface->Seek(file_handle, 0, SEEK_SET);

		using Rml::byte;
		Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
		file_interface->Read(buffer.get(), buffer_size, file_handle);
		file_interface->Close(file_handle);

		sf::Image image;
		if (!image.loadFromMemory(buffer.get(), buffer_size))
			return false;

		// Convert colors to premultiplied alpha, which is necessary for correct alpha compositing.
		for (unsigned int x = 0; x < image.getSize().x; x++)
		{
			for (unsigned int y = 0; y < image.getSize().y; y++)
			{
				sf::Color color = image.getPixel(SFML_COORDINATE(x, y));
				color.r = static_cast<std::uint8_t>((color.r * color.a) / 255);
				color.g = static_cast<std::uint8_t>((color.g * color.a) / 255);
				color.b = static_cast<std::uint8_t>((color.b * color.a) / 255);
				image.setPixel(SFML_COORDINATE(x, y), color);
			}
		}

		sf::Texture* texture = new sf::Texture();
		texture->setSmooth(true);

		if (!texture->loadFromImage(image))
		{
			delete texture;
			return false;
		}

		texture_dimensions = Rml::Vector2i(texture->getSize().x, texture->getSize().y);
		return (Rml::TextureHandle)texture;
	}

	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions_i) override
	{
		const auto source_dimensions = Rml::Vector2<unsigned int>(source_dimensions_i);

#if SFML_VERSION_MAJOR >= 3
		sf::Texture* texture = new sf::Texture(sf::Vector2u{source_dimensions.x, source_dimensions.y});
#else
		sf::Texture* texture = new sf::Texture();
		if (!texture->create(source_dimensions.x, source_dimensions.y))
		{
			delete texture;
			return false;
		}
#endif
		texture->setSmooth(true);
		texture->update(source.data(), SFML_COORDINATE(source_dimensions.x, source_dimensions.y), SFML_COORDINATE(0, 0));
		return (Rml::TextureHandle)texture;
	}

	void ReleaseTexture(Rml::TextureHandle texture_handle) override { delete (sf::Texture*)texture_handle; }
};

// Updates the viewport and context dimensions, should be called whenever the window size changes.
static void UpdateWindowDimensions(sf::RenderWindow& window, RenderInterface_GL2_SFML& render_interface, Rml::Context* context)
{
	const int width = (int)window.getSize().x;
	const int height = (int)window.getSize().y;

	if (context)
		context->SetDimensions(Rml::Vector2i(width, height));

	sf::View view(sf::FloatRect(SFML_COORDINATE(0.f, 0.f), SFML_COORDINATE((float)width, (float)height)));
	window.setView(view);

	render_interface.SetViewport(width, height);
}

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_SFML system_interface;
	RenderInterface_GL2_SFML render_interface;
	sf::RenderWindow window;
	bool running = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	data = Rml::MakeUnique<BackendData>();

	const std::uint32_t style = (allow_resize ? sf::Style::Default : (sf::Style::Titlebar | sf::Style::Close));
	constexpr unsigned int anti_aliasing_level = 2;

	// Create the window.
	sf::RenderWindow out_window;
	sf::ContextSettings context_settings;
	context_settings.stencilBits = 8;

#if SFML_VERSION_MAJOR >= 3
	context_settings.antiAliasingLevel = anti_aliasing_level;
	data->window.create(sf::VideoMode({(unsigned int)width, (unsigned int)height}), window_name, style, sf::State::Windowed, context_settings);
#else
	context_settings.antialiasingLevel = anti_aliasing_level;
	data->window.create(sf::VideoMode(width, height), window_name, style, context_settings);
#endif

	data->window.setVerticalSyncEnabled(true);
	if (!data->window.isOpen())
	{
		data.reset();
		return false;
	}

	// Optionally apply the SFML window to the system interface so that it can change its mouse cursor.
	data->system_interface.SetWindow(&data->window);

	UpdateWindowDimensions(data->window, data->render_interface, nullptr);

	return true;
}

void Backend::Shutdown()
{
	data.reset();
}

Rml::SystemInterface* Backend::GetSystemInterface()
{
	RMLUI_ASSERT(data);
	return &data->system_interface;
}

Rml::RenderInterface* Backend::GetRenderInterface()
{
	RMLUI_ASSERT(data);
	return &data->render_interface;
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

	// SFML does not seem to provide a way to wait for events with a timeout.
	(void)power_save;

	// The contents of this function is intended to be copied directly into your main loop.
	bool result = data->running;
	data->running = true;

	auto handle_key_pressed = [&](const sf::Event& ev, sf::Keyboard::Key key_pressed_code) {
		const Rml::Input::KeyIdentifier key = RmlSFML::ConvertKey(key_pressed_code);
		const int key_modifier = RmlSFML::GetKeyModifierState();
		const float native_dp_ratio = 1.f;

		// See if we have any global shortcuts that take priority over the context.
		if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, true))
			return;
		// Otherwise, hand the event over to the context by calling the input handler as normal.
		if (!RmlSFML::InputHandler(context, ev))
			return;
		// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
		if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, false))
			return;
	};

#if SFML_VERSION_MAJOR >= 3
	while (const std::optional ev = data->window.pollEvent())
	{
		if (ev->is<sf::Event::Resized>())
		{
			UpdateWindowDimensions(data->window, data->render_interface, context);
		}
		else if (auto key_pressed = ev->getIf<sf::Event::KeyPressed>())
		{
			handle_key_pressed(*ev, key_pressed->code);
		}
		else if (ev->is<sf::Event::Closed>())
		{
			result = false;
		}
		else
		{
			RmlSFML::InputHandler(context, *ev);
		}
	}

#else

	sf::Event ev;
	while (data->window.pollEvent(ev))
	{
		switch (ev.type)
		{
		case sf::Event::Resized: UpdateWindowDimensions(data->window, data->render_interface, context); break;
		case sf::Event::KeyPressed: handle_key_pressed(ev, ev.key.code); break;
		case sf::Event::Closed: result = false; break;
		default: RmlSFML::InputHandler(context, ev); break;
		}
	}
#endif
	return result;
}

void Backend::RequestExit()
{
	RMLUI_ASSERT(data);
	data->running = false;
}

void Backend::BeginFrame()
{
	RMLUI_ASSERT(data);
	sf::RenderWindow& window = data->window;

	window.resetGLStates();
	window.clear();

	data->render_interface.BeginFrame();

#if 0
	// Draw a simple shape with SFML for demonstration purposes. Make sure to push and pop GL states as appropriate.
	sf::Vector2f circle_position(100.f, 100.f);

	window.pushGLStates();

	sf::CircleShape circle(50.f);
	circle.setPosition(circle_position);
	circle.setFillColor(sf::Color::Blue);
	circle.setOutlineColor(sf::Color::Red);
	circle.setOutlineThickness(10.f);
	window.draw(circle);

	window.popGLStates();
#endif
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);

	data->render_interface.EndFrame();
	data->window.display();

	// Optional, used to mark frames during performance profiling.
	RMLUI_FrameMark;
}
