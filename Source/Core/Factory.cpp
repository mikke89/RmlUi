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

#include "precompiled.h"
#include "../../Include/RmlUi/Core.h"
#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "ContextInstancerDefault.h"
#include "DecoratorTiledBoxInstancer.h"
#include "DecoratorTiledHorizontalInstancer.h"
#include "DecoratorTiledImageInstancer.h"
#include "DecoratorTiledVerticalInstancer.h"
#include "ElementHandle.h"
#include "ElementImage.h"
#include "ElementTextDefault.h"
#include "EventInstancerDefault.h"
#include "FontEffectOutlineInstancer.h"
#include "FontEffectShadowInstancer.h"
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
typedef UnorderedMap< String, ElementInstancerPtr > ElementInstancerMap;
static ElementInstancerMap element_instancers;

// Decorator instancers.
typedef UnorderedMap< String, std::unique_ptr<DecoratorInstancer> > DecoratorInstancerMap;
static DecoratorInstancerMap decorator_instancers;

// Font effect instancers.
typedef UnorderedMap< String, std::unique_ptr<FontEffectInstancer> > FontEffectInstancerMap;
static FontEffectInstancerMap font_effect_instancers;

// The context instancer.
static SharedPtr<ContextInstancer> context_instancer;

// The event instancer
static EventInstancer* event_instancer = NULL;

// Event listener instancer.
static EventListenerInstancer* event_listener_instancer = NULL;

Factory::Factory()
{
}

Factory::~Factory()
{
}

bool Factory::Initialise()
{
	// Bind the default context instancer.
	if (!context_instancer)
		context_instancer = std::make_shared<ContextInstancerDefault>();

	// Bind default event instancer
	if (event_instancer == NULL)
		event_instancer = new EventInstancerDefault();

	// No default event listener instancer
	if (event_listener_instancer == NULL)
		event_listener_instancer = NULL;

	// Bind the default element instancers
	RegisterElementInstancer("*", ElementInstancerPtr(new ElementInstancerGeneric< Element >));
	RegisterElementInstancer("img", ElementInstancerPtr(new ElementInstancerGeneric< ElementImage >));
	RegisterElementInstancer("#text", ElementInstancerPtr(new ElementInstancerGeneric< ElementTextDefault >));
	RegisterElementInstancer("handle", ElementInstancerPtr(new ElementInstancerGeneric< ElementHandle >));
	RegisterElementInstancer("body", ElementInstancerPtr(new ElementInstancerGeneric< ElementDocument >));

	// Bind the default decorator instancers
	RegisterDecoratorInstancer("tiled-horizontal", std::make_unique<DecoratorTiledHorizontalInstancer>());
	RegisterDecoratorInstancer("tiled-vertical", std::make_unique<DecoratorTiledVerticalInstancer>());
	RegisterDecoratorInstancer("tiled-box", std::make_unique<DecoratorTiledBoxInstancer>());
	RegisterDecoratorInstancer("image", std::make_unique<DecoratorTiledImageInstancer>());

	RegisterFontEffectInstancer("shadow", std::make_unique<FontEffectShadowInstancer>());
	RegisterFontEffectInstancer("outline", std::make_unique<FontEffectOutlineInstancer>());

	// Register the core XML node handlers.
	XMLParser::RegisterNodeHandler("", new XMLNodeHandlerDefault())->RemoveReference();
	XMLParser::RegisterNodeHandler("body", new XMLNodeHandlerBody())->RemoveReference();
	XMLParser::RegisterNodeHandler("head", new XMLNodeHandlerHead())->RemoveReference();
	XMLParser::RegisterNodeHandler("template", new XMLNodeHandlerTemplate())->RemoveReference();

	return true;
}

void Factory::Shutdown()
{
	element_instancers.clear();

	decorator_instancers.clear();

	font_effect_instancers.clear();

	context_instancer.reset();

	if (event_listener_instancer)
		event_listener_instancer->RemoveReference();
	event_listener_instancer = NULL;

	if (event_instancer)
		event_instancer->RemoveReference();
	event_instancer = NULL;

	XMLParser::ReleaseHandlers();
}

// Registers the instancer to use when instancing contexts.
ContextInstancer* Factory::RegisterContextInstancer(SharedPtr<ContextInstancer> instancer)
{
	ContextInstancer* result = instancer.get();
	context_instancer = std::move(instancer);
	return result;
}

// Instances a new context.
UniquePtr<Context> Factory::InstanceContext(const String& name)
{
	UniquePtr<Context> new_context = context_instancer->InstanceContext(name);
	if (new_context)
		new_context->SetInstancer(context_instancer);
	return new_context;
}

ElementInstancer* Factory::RegisterElementInstancer(const String& name, ElementInstancerPtr instancer)
{
	ElementInstancer* result = instancer.get();
	String lower_case_name = ToLower(name);
	element_instancers[lower_case_name] = std::move(instancer);
	return result;
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

	return instancer_iterator->second.get();
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

			PluginRegistry::NotifyElementCreate(element.get());
		}

		return element;
	}

	return NULL;
}

// Instances a single text element containing a string.
bool Factory::InstanceElementText(Element* parent, const String& text)
{
	SystemInterface* system_interface = GetSystemInterface();

	// Do any necessary translation. If any substitutions were made then new XML may have been introduced, so we'll
	// have to run the data through the XML parser again.
	String translated_data;
	if (system_interface != NULL &&
		(system_interface->TranslateString(translated_data, text) > 0 ||
		 translated_data.find("<") != String::npos))
	{
		StreamMemory* stream = new StreamMemory(translated_data.size() + 32);
		stream->Write("<body>", 6);
		stream->Write(translated_data);
		stream->Write("</body>", 7);
		stream->Seek(0, SEEK_SET);

		InstanceElementStream(parent, stream);
		stream->RemoveReference();
	}
	else
	{
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
		ElementPtr element = Factory::InstanceElement(parent, "#text", "#text", attributes);
		if (!element)
		{
			Log::Message(Log::LT_ERROR, "Failed to instance text element '%s', instancer returned NULL.", translated_data.c_str());
			return false;
		}

		// Assign the element its text value.
		ElementText* text_element = dynamic_cast< ElementText* >(element.get());
		if (!text_element)
		{
			Log::Message(Log::LT_ERROR, "Failed to instance text element '%s'. Found type '%s', was expecting a derivative of ElementText.", translated_data.c_str(), typeid(element).name());
			return false;
		}

		text_element->SetText(ToWideString(translated_data));

		// Add to active node.
		parent->AppendChild(std::move(element));
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
	ElementPtr element = Factory::InstanceElement(nullptr, "body", "body", XMLAttributes());
	if (!element)
	{
		Log::Message(Log::LT_ERROR, "Failed to instance document, instancer returned NULL.");
		return nullptr;
	}

	ElementDocument* document = dynamic_cast< ElementDocument* >(element.get());
	if (!document)
	{
		Log::Message(Log::LT_ERROR, "Failed to instance document element. Found type '%s', was expecting derivative of ElementDocument.", typeid(element).name());
		return nullptr;
	}

	document->context = context;

	XMLParser parser(element.get());
	parser.Parse(stream);

	return element;
}


// Registers an instancer that will be used to instance decorators.
void Factory::RegisterDecoratorInstancer(const String& name, std::unique_ptr<DecoratorInstancer> instancer)
{
	if (!instancer)
		return;

	String lower_case_name = ToLower(name);
	decorator_instancers[lower_case_name] = std::move(instancer);
}

// Retrieves a decorator instancer registered with the factory.
DecoratorInstancer* Factory::GetDecoratorInstancer(const String& name)
{
	auto iterator = decorator_instancers.find(name);
	if (iterator == decorator_instancers.end())
		return nullptr;
	
	return iterator->second.get();
}

// Registers an instancer that will be used to instance font effects.
void Factory::RegisterFontEffectInstancer(const String& name, std::unique_ptr<FontEffectInstancer> instancer)
{
	if (!instancer)
		return;

	String lower_case_name = ToLower(name);
	font_effect_instancers[lower_case_name] = std::move(instancer);
}

FontEffectInstancer* Factory::GetFontEffectInstancer(const String& name)
{
	auto iterator = font_effect_instancers.find(name);
	if (iterator == font_effect_instancers.end())
		return nullptr;

	return iterator->second.get();
}


// Creates a style sheet containing the passed in styles.
StyleSheet* Factory::InstanceStyleSheetString(const String& string)
{
	StreamMemory* memory_stream = new StreamMemory((const byte*) string.c_str(), string.size());
	StyleSheet* style_sheet = InstanceStyleSheetStream(memory_stream);
	memory_stream->RemoveReference();
	return style_sheet;
}

// Creates a style sheet from a file.
StyleSheet* Factory::InstanceStyleSheetFile(const String& file_name)
{
	StreamFile* file_stream = new StreamFile();
	file_stream->Open(file_name);
	StyleSheet* style_sheet = InstanceStyleSheetStream(file_stream);
	file_stream->RemoveReference();
	return style_sheet;
}

// Creates a style sheet from an Stream.
StyleSheet* Factory::InstanceStyleSheetStream(Stream* stream)
{
	StyleSheet* style_sheet = new StyleSheet();
	if (style_sheet->LoadStyleSheet(stream))
	{
		return style_sheet;
	}

	style_sheet->RemoveReference();
	return NULL;
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
EventInstancer* Factory::RegisterEventInstancer(EventInstancer* instancer)
{
	instancer->AddReference();

	if (event_instancer)
		event_instancer->RemoveReference();

	event_instancer = instancer;
	return instancer;
}

// Instance an event object.
Event* Factory::InstanceEvent(Element* target, EventId id, const String& type, const Dictionary& parameters, bool interruptible)
{
	Event* event = event_instancer->InstanceEvent(target, id, type, parameters, interruptible);
	if (event != NULL)
		event->instancer = event_instancer;

	return event;
}

// Register an instancer for all event listeners
EventListenerInstancer* Factory::RegisterEventListenerInstancer(EventListenerInstancer* instancer)
{
	instancer->AddReference();

	if (event_listener_instancer)
		event_listener_instancer->RemoveReference();

	event_listener_instancer = instancer;
	return instancer;
}

// Instance an event listener with the given string
EventListener* Factory::InstanceEventListener(const String& value, Element* element)
{
	// If we have an event listener instancer, use it
	if (event_listener_instancer)
		return event_listener_instancer->InstanceEventListener(value, element);

	return NULL;
}

}
}
