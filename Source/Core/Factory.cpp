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

#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "../../Include/RmlUi/Core.h"

#include "ContextInstancerDefault.h"
#include "DecoratorTiledBoxInstancer.h"
#include "DecoratorTiledHorizontalInstancer.h"
#include "DecoratorTiledImageInstancer.h"
#include "DecoratorTiledVerticalInstancer.h"
#include "DecoratorNinePatch.h"
#include "DecoratorGradient.h"
#include "ElementHandle.h"
#include "ElementImage.h"
#include "ElementTextDefault.h"
#include "EventInstancerDefault.h"
#include "FontEffectBlur.h"
#include "FontEffectGlow.h"
#include "FontEffectOutline.h"
#include "FontEffectShadow.h"
#include "PluginRegistry.h"
#include "PropertyParserColour.h"
#include "StreamFile.h"
#include "StyleSheetFactory.h"
#include "TemplateCache.h"
#include "XMLNodeHandlerBody.h"
#include "XMLNodeHandlerDefault.h"
#include "XMLNodeHandlerHead.h"
#include "XMLNodeHandlerTemplate.h"
#include "XMLParseTools.h"

namespace Rml {
namespace Core {

// Element instancers.
typedef UnorderedMap< String, ElementInstancer* > ElementInstancerMap;
static ElementInstancerMap element_instancers;

// Decorator instancers.
typedef UnorderedMap< String, DecoratorInstancer* > DecoratorInstancerMap;
static DecoratorInstancerMap decorator_instancers;

// Font effect instancers.
typedef UnorderedMap< String, FontEffectInstancer* > FontEffectInstancerMap;
static FontEffectInstancerMap font_effect_instancers;

// The context instancer.
static ContextInstancer* context_instancer = nullptr;;

// The event instancer
static EventInstancer* event_instancer = nullptr;;

// Event listener instancer.
static EventListenerInstancer* event_listener_instancer = nullptr;


// Default instancers are constructed and destroyed on Initialise and Shutdown, respectively.
struct DefaultInstancers {
	template<typename T> using Ptr = UniquePtr<T>;

	Ptr<ContextInstancer> context_default;
	Ptr<EventInstancer> event_default;

	Ptr<ElementInstancer> element_default = std::make_unique<ElementInstancerElement>();
	Ptr<ElementInstancer> element_text_default = std::make_unique<ElementInstancerTextDefault>();
	Ptr<ElementInstancer> element_img = std::make_unique<ElementInstancerGeneric<ElementImage>>();
	Ptr<ElementInstancer> element_handle = std::make_unique<ElementInstancerGeneric<ElementHandle>>();
	Ptr<ElementInstancer> element_body = std::make_unique<ElementInstancerGeneric<ElementDocument>>();

	Ptr<DecoratorInstancer> decorator_tiled_horizontal = std::make_unique<DecoratorTiledHorizontalInstancer>();
	Ptr<DecoratorInstancer> decorator_tiled_vertical = std::make_unique<DecoratorTiledVerticalInstancer>();
	Ptr<DecoratorInstancer> decorator_tiled_box = std::make_unique<DecoratorTiledBoxInstancer>();
	Ptr<DecoratorInstancer> decorator_image = std::make_unique<DecoratorTiledImageInstancer>();
	Ptr<DecoratorInstancer> decorator_ninepatch = std::make_unique<DecoratorNinePatchInstancer>();
	Ptr<DecoratorInstancer> decorator_gradient = std::make_unique<DecoratorGradientInstancer>();

	Ptr<FontEffectInstancer> font_effect_blur = std::make_unique<FontEffectBlurInstancer>();
	Ptr<FontEffectInstancer> font_effect_glow = std::make_unique<FontEffectGlowInstancer>();
	Ptr<FontEffectInstancer> font_effect_outline = std::make_unique<FontEffectOutlineInstancer>();
	Ptr<FontEffectInstancer> font_effect_shadow = std::make_unique<FontEffectShadowInstancer>();
};

static UniquePtr<DefaultInstancers> default_instancers;


Factory::Factory()
{
}

Factory::~Factory()
{
}


bool Factory::Initialise()
{
	default_instancers = std::make_unique<DefaultInstancers>();

	// Bind the default context instancer.
	if (!context_instancer)
	{
		default_instancers->context_default = std::make_unique<ContextInstancerDefault>();
		context_instancer = default_instancers->context_default.get();
	}

	// Bind default event instancer
	if (!event_instancer)
	{
		default_instancers->event_default = std::make_unique<EventInstancerDefault>();
		event_instancer = default_instancers->event_default.get();
	}

	// No default event listener instancer
	if (!event_listener_instancer)
		event_listener_instancer = nullptr;

	// Bind the default element instancers
	RegisterElementInstancer("*", default_instancers->element_default.get());
	RegisterElementInstancer("img", default_instancers->element_img.get());
	RegisterElementInstancer("#text", default_instancers->element_text_default.get());
	RegisterElementInstancer("handle", default_instancers->element_handle.get());
	RegisterElementInstancer("body", default_instancers->element_body.get());

	// Bind the default decorator instancers
	RegisterDecoratorInstancer("tiled-horizontal", default_instancers->decorator_tiled_horizontal.get());
	RegisterDecoratorInstancer("tiled-vertical", default_instancers->decorator_tiled_vertical.get());
	RegisterDecoratorInstancer("tiled-box", default_instancers->decorator_tiled_box.get());
	RegisterDecoratorInstancer("image", default_instancers->decorator_image.get());
	RegisterDecoratorInstancer("ninepatch", default_instancers->decorator_ninepatch.get());
	RegisterDecoratorInstancer("gradient", default_instancers->decorator_gradient.get());

	RegisterFontEffectInstancer("blur", default_instancers->font_effect_blur.get());
	RegisterFontEffectInstancer("glow", default_instancers->font_effect_glow.get());
	RegisterFontEffectInstancer("outline", default_instancers->font_effect_outline.get());
	RegisterFontEffectInstancer("shadow", default_instancers->font_effect_shadow.get());

	// Register the core XML node handlers.
	XMLParser::RegisterNodeHandler("", std::make_shared<XMLNodeHandlerDefault>());
	XMLParser::RegisterNodeHandler("body", std::make_shared<XMLNodeHandlerBody>());
	XMLParser::RegisterNodeHandler("head", std::make_shared<XMLNodeHandlerHead>());
	XMLParser::RegisterNodeHandler("template", std::make_shared<XMLNodeHandlerTemplate>());

	return true;
}

void Factory::Shutdown()
{
	element_instancers.clear();

	decorator_instancers.clear();

	font_effect_instancers.clear();

	context_instancer = nullptr;

	event_listener_instancer = nullptr;

	event_instancer = nullptr;

	XMLParser::ReleaseHandlers();

	default_instancers.reset();
}

// Registers the instancer to use when instancing contexts.
void Factory::RegisterContextInstancer(ContextInstancer* instancer)
{
	context_instancer = instancer;
}

// Instances a new context.
ContextPtr Factory::InstanceContext(const String& name)
{
	ContextPtr new_context = context_instancer->InstanceContext(name);
	if (new_context)
		new_context->SetInstancer(context_instancer);
	return new_context;
}

void Factory::RegisterElementInstancer(const String& name, ElementInstancer* instancer)
{
	element_instancers[StringUtilities::ToLower(name)] = instancer;
}

// Looks up the instancer for the given element
ElementInstancer* Factory::GetElementInstancer(const String& tag)
{
	ElementInstancerMap::iterator instancer_iterator = element_instancers.find(tag);
	if (instancer_iterator == element_instancers.end())
	{
		instancer_iterator = element_instancers.find("*");
		if (instancer_iterator == element_instancers.end())
			return nullptr;
	}

	return instancer_iterator->second;
}

// Instances a single element.
ElementPtr Factory::InstanceElement(Element* parent, const String& instancer_name, const String& tag, const XMLAttributes& attributes)
{
	ElementInstancer* instancer = GetElementInstancer(instancer_name);

	if (instancer)
	{
		ElementPtr element = instancer->InstanceElement(parent, tag, attributes);		

		// Process the generic attributes and bind any events
		if (element)
		{
			element->SetInstancer(instancer);
			element->SetAttributes(attributes);
			ElementUtilities::BindEventAttributes(element.get());


			// TODO: Relies on parent, a bit hacky.
			if (parent && !parent->HasAttribute("data-for"))
			{
				// Look for the data-model attribute or otherwise copy it from the parent element
				auto it = attributes.find("data-model");
				if (it != attributes.end())
				{
					String error_msg;

					if (auto context = parent->GetContext())
					{
						String name = it->second.Get<String>();
						if (auto model = context->GetDataModel(name))
							element->data_model = model;
						else
							Log::Message(Log::LT_WARNING, "Could not locate data model '%s'.", name.c_str());
					}
					else
					{
						Log::Message(Log::LT_WARNING, "Could not add data model to element '%s' because the context is not available.", element->GetAddress().c_str());
					}
				}
				else if (parent && parent->data_model)
				{
					element->data_model = parent->data_model;
				}

				// If we have an active data model, check the attributes for any data bindings
				if (DataModel* data_model = element->data_model)
				{
					for (auto& attribute : attributes)
					{
						auto& name = attribute.first;

						if (name.size() > 5 && name[0] == 'd' && name[1] == 'a' && name[2] == 't' && name[3] == 'a' && name[4] == '-')
						{
							const size_t data_type_end = name.find('-', 5);
							const size_t count = (data_type_end == String::npos ? String::npos : data_type_end - 5);
							const String data_type = name.substr(5, count);
							const String value_bind_name = attribute.second.Get<String>();

							if (data_type == "attr")
							{
								const String attr_bind_name = name.substr(5 + data_type.size() + 1);

								auto view = std::make_unique<DataViewAttribute>(*data_model, element.get(), parent, value_bind_name, attr_bind_name);
								if (*view)
									data_model->AddView(std::move(view));
								else
									Log::Message(Log::LT_WARNING, "Could not add data-attr view to element '%s'.", parent->GetAddress().c_str());

								DataControllerAttribute data_controller(*data_model, parent, attr_bind_name, value_bind_name);
								if (data_controller)
									data_model->controllers.AddController(element.get(), std::move(data_controller));
								else
									Log::Message(Log::LT_WARNING, "Could not add data-attr controller to element '%s'.", parent->GetAddress().c_str());
							}
							else if (data_type == "style")
							{
								const String property_name = name.substr(5 + data_type.size() + 1);

								auto view = std::make_unique<DataViewStyle>(*data_model, element.get(), parent, value_bind_name, property_name);
								if (*view)
									data_model->AddView(std::move(view));
								else
									Log::Message(Log::LT_WARNING, "Could not add data-style view to element '%s'.", parent->GetAddress().c_str());
							}
							else if (data_type == "if")
							{
								auto view = std::make_unique<DataViewIf>(*data_model, element.get(), parent, value_bind_name);
								if (*view)
									data_model->AddView(std::move(view));
								else
									Log::Message(Log::LT_WARNING, "Could not add data-if view to element '%s'.", parent->GetAddress().c_str());
							}
						}
					}
				}
			}

			PluginRegistry::NotifyElementCreate(element.get());
		}

		return element;
	}

	return nullptr;
}

// Instances a single text element containing a string.
bool Factory::InstanceElementText(Element* parent, const String& text)
{
	SystemInterface* system_interface = GetSystemInterface();

	// Do any necessary translation. If any substitutions were made then new XML may have been introduced, so we'll
	// have to run the data through the XML parser again.
	String translated_data;
	if (system_interface != nullptr &&
		(system_interface->TranslateString(translated_data, text) > 0 ||
		 translated_data.find("<") != String::npos))
	{
		RMLUI_ZoneScopedNC("InstanceStream", 0xDC143C);
		auto stream = std::make_unique<StreamMemory>(translated_data.size() + 32);
		stream->Write("<body>", 6);
		stream->Write(translated_data);
		stream->Write("</body>", 7);
		stream->Seek(0, SEEK_SET);

		InstanceElementStream(parent, stream.get());
	}
	else
	{
		RMLUI_ZoneScopedNC("InstanceText", 0x8FBC8F);
		// Check if this text node contains only white-space; if so, we don't want to construct it.
		bool only_white_space = true;
		for (size_t i = 0; i < translated_data.size(); ++i)
		{
			if (!StringUtilities::IsWhitespace(translated_data[i]))
			{
				only_white_space = false;
				break;
			}
		}

		if (only_white_space)
			return true;

		// Attempt to instance the element.
		XMLAttributes attributes;
		ElementPtr element_ptr = Factory::InstanceElement(parent, "#text", "#text", attributes);
		if (!element_ptr)
		{
			Log::Message(Log::LT_ERROR, "Failed to instance text element '%s', instancer returned nullptr.", translated_data.c_str());
			return false;
		}

		// Assign the element its text value.
		ElementText* text_element = rmlui_dynamic_cast< ElementText* >(element_ptr.get());
		if (!text_element)
		{
			Log::Message(Log::LT_ERROR, "Failed to instance text element '%s'. Found type '%s', was expecting a derivative of ElementText.", translated_data.c_str(), rmlui_type_name(*element_ptr));
			return false;
		}

		// Add to active node.
		Element* element = parent->AppendChild(std::move(element_ptr));

		// See if this text element uses data bindings.
		bool data_view_added = false;
		if (DataModel* data_model = element->data_model)
		{
			const size_t i_brackets = translated_data.find("{{", 0);
			if (i_brackets != String::npos)
			{
				auto view = std::make_unique<DataViewText>(*data_model, text_element, translated_data, i_brackets);
				if (*view)
				{
					data_model->AddView(std::move(view));
					data_view_added = true;
				}
				else
				{
					Log::Message(Log::LT_WARNING, "Could not add data binding view to element '%s'.", parent->GetAddress().c_str());
				}
			}
		}

		if(!data_view_added)
			text_element->SetText(translated_data);
	}

	return true;
}

// Instances a element tree based on the stream
bool Factory::InstanceElementStream(Element* parent, Stream* stream)
{
	XMLParser parser(parent);
	parser.Parse(stream);
	return true;
}

// Instances a element tree based on the stream
ElementPtr Factory::InstanceDocumentStream(Rml::Core::Context* context, Stream* stream)
{
	RMLUI_ZoneScoped;

	ElementPtr element = Factory::InstanceElement(nullptr, "body", "body", XMLAttributes());
	if (!element)
	{
		Log::Message(Log::LT_ERROR, "Failed to instance document, instancer returned nullptr.");
		return nullptr;
	}

	ElementDocument* document = rmlui_dynamic_cast< ElementDocument* >(element.get());
	if (!document)
	{
		Log::Message(Log::LT_ERROR, "Failed to instance document element. Found type '%s', was expecting derivative of ElementDocument.", rmlui_type_name(*element));
		return nullptr;
	}

	document->context = context;

	XMLParser parser(element.get());
	parser.Parse(stream);

	return element;
}


// Registers an instancer that will be used to instance decorators.
void Factory::RegisterDecoratorInstancer(const String& name, DecoratorInstancer* instancer)
{
	RMLUI_ASSERT(instancer);
	decorator_instancers[StringUtilities::ToLower(name)] = instancer;
}

// Retrieves a decorator instancer registered with the factory.
DecoratorInstancer* Factory::GetDecoratorInstancer(const String& name)
{
	auto iterator = decorator_instancers.find(name);
	if (iterator == decorator_instancers.end())
		return nullptr;
	
	return iterator->second;
}

// Registers an instancer that will be used to instance font effects.
void Factory::RegisterFontEffectInstancer(const String& name, FontEffectInstancer* instancer)
{
	RMLUI_ASSERT(instancer);
	font_effect_instancers[StringUtilities::ToLower(name)] = instancer;
}

FontEffectInstancer* Factory::GetFontEffectInstancer(const String& name)
{
	auto iterator = font_effect_instancers.find(name);
	if (iterator == font_effect_instancers.end())
		return nullptr;

	return iterator->second;
}


// Creates a style sheet containing the passed in styles.
SharedPtr<StyleSheet> Factory::InstanceStyleSheetString(const String& string)
{
	auto memory_stream = std::make_unique<StreamMemory>((const byte*) string.c_str(), string.size());
	return InstanceStyleSheetStream(memory_stream.get());
}

// Creates a style sheet from a file.
SharedPtr<StyleSheet> Factory::InstanceStyleSheetFile(const String& file_name)
{
	auto file_stream = std::make_unique<StreamFile>();
	file_stream->Open(file_name);
	return InstanceStyleSheetStream(file_stream.get());
}

// Creates a style sheet from an Stream.
SharedPtr<StyleSheet> Factory::InstanceStyleSheetStream(Stream* stream)
{
	SharedPtr<StyleSheet> style_sheet = std::make_shared<StyleSheet>();
	if (style_sheet->LoadStyleSheet(stream))
	{
		return style_sheet;
	}
	return nullptr;
}

// Clears the style sheet cache. This will force style sheets to be reloaded.
void Factory::ClearStyleSheetCache()
{
	StyleSheetFactory::ClearStyleSheetCache();
}

/// Clears the template cache. This will force templates to be reloaded.
void Factory::ClearTemplateCache()
{
	TemplateCache::Clear();
}

// Registers an instancer for all RmlEvents
void Factory::RegisterEventInstancer(EventInstancer* instancer)
{
	event_instancer = instancer;
}

// Instance an event object.
EventPtr Factory::InstanceEvent(Element* target, EventId id, const String& type, const Dictionary& parameters, bool interruptible)
{
	EventPtr event = event_instancer->InstanceEvent(target, id, type, parameters, interruptible);
	if (event)
		event->instancer = event_instancer;
	return event;
}

// Register an instancer for all event listeners
void Factory::RegisterEventListenerInstancer(EventListenerInstancer* instancer)
{
	event_listener_instancer = instancer;
}

// Instance an event listener with the given string
EventListener* Factory::InstanceEventListener(const String& value, Element* element)
{
	// If we have an event listener instancer, use it
	if (event_listener_instancer)
		return event_listener_instancer->InstanceEventListener(value, element);

	return nullptr;
}

}
}
