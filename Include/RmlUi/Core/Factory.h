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

#ifndef RMLUICOREFACTORY_H
#define RMLUICOREFACTORY_H

#include "XMLParser.h"
#include "Header.h"

namespace Rml {
namespace Core {

class Context;
class ContextInstancer;
class Decorator;
class DecoratorInstancer;
class Element;
class ElementDocument;
class ElementInstancer;
class Event;
class EventInstancer;
class EventListener;
class EventListenerInstancer;
class FontEffect;
class FontEffectInstancer;
class StyleSheet;
class PropertyDictionary;
class PropertySpecification;
class DecoratorInstancerInterface;
enum class EventId : uint16_t;

/**
	The Factory contains a registry of instancers for different types.

	All instantiation of these rmlui types should go through the factory
	so that scripting API's can bind in new types.

	@author Lloyd Weehuizen
 */

class RMLUICORE_API Factory
{
public:
	/// Initialise the element factory
	static bool Initialise();
	/// Cleanup and shutdown the factory
	static void Shutdown();

	/// Registers a non-owning pointer to the instancer used to instance contexts.
	/// @param[in] instancer The new context instancer.
	/// @lifetime The instancer must be kept alive until after the call to Core::Shutdown.
	static void RegisterContextInstancer(ContextInstancer* instancer);
	/// Instances a new context.
	/// @param[in] name The name of the new context.
	/// @return The new context, or nullptr if no context could be created.
	static ContextPtr InstanceContext(const String& name);

	/// Registers a non-owning pointer to the element instancer that will be used to instance an element when the specified tag is encountered.
	/// @param[in] name Name of the instancer; elements with this as their tag will use this instancer.
	/// @param[in] instancer The instancer to call when the tag is encountered.
	/// @lifetime The instancer must be kept alive until after the call to Core::Shutdown.
	static void RegisterElementInstancer(const String& name, ElementInstancer* instancer);
	/// Returns the element instancer for the specified tag.
	/// @param[in] tag Name of the tag to get the instancer for.
	/// @return The requested element instancer, or nullptr if no such instancer is registered.
	static ElementInstancer* GetElementInstancer(const String& tag);
	/// Instances a single element.
	/// @param[in] parent The parent of the new element, or nullptr for a root tag.
	/// @param[in] instancer The name of the instancer to create the element with.
	/// @param[in] tag The tag of the element to be instanced.
	/// @param[in] attributes The attributes to instance the element with.
	/// @return The instanced element, or nullptr if the instancing failed.
	static ElementPtr InstanceElement(Element* parent, const String& instancer, const String& tag, const XMLAttributes& attributes);

	/// Instances a single text element containing a string. The string is assumed to contain no RML markup, but will
	/// be translated and therefore may have some introduced. In this case more than one element may be instanced.
	/// @param[in] parent The element any instanced elements will be parented to.
	/// @param[in] text The text to instance the element (or elements) from.
	/// @return True if the string was parsed without error, false otherwise.
	static bool InstanceElementText(Element* parent, const String& text);
	/// Instances an element tree based on the stream.
	/// @param[in] parent The element the stream elements will be added to.
	/// @param[in] stream The stream to read the element RML from.
	/// @return True if the stream was parsed without error, false otherwise.
	static bool InstanceElementStream(Element* parent, Stream* stream);
	/// Instances a document from a stream.
	/// @param[in] context The context that is creating the document.
	/// @param[in] stream The stream to instance from.
	/// @return The instanced document, or nullptr if an error occurred.
	static ElementPtr InstanceDocumentStream(Rml::Core::Context* context, Stream* stream);

	/// Registers a non-owning pointer to an instancer that will be used to instance decorators.
	/// @param[in] name The name of the decorator the instancer will be called for.
	/// @param[in] instancer The instancer to call when the decorator name is encountered.
	/// @lifetime The instancer must be kept alive until after the call to Core::Shutdown.
	/// @return The added instancer if the registration was successful, nullptr otherwise.
	static void RegisterDecoratorInstancer(const String& name, DecoratorInstancer* instancer);
	/// Retrieves a decorator instancer registered with the factory.
	/// @param[in] name The name of the desired decorator type.
	/// @return The decorator instancer it it exists, nullptr otherwise.
	static DecoratorInstancer* GetDecoratorInstancer(const String& name);

	/// Registers a non-owning pointer to an instancer that will be used to instance font effects.
	/// @param[in] name The name of the font effect the instancer will be called for.
	/// @param[in] instancer The instancer to call when the font effect name is encountered.
	/// @lifetime The instancer must be kept alive until after the call to Core::Shutdown.
	/// @return The added instancer if the registration was successful, nullptr otherwise.
	static void RegisterFontEffectInstancer(const String& name, FontEffectInstancer* instancer);
	/// Retrieves a font-effect instancer registered with the factory.
	/// @param[in] name The name of the desired font-effect type.
	/// @return The font-effect instancer it it exists, nullptr otherwise.
	static FontEffectInstancer* GetFontEffectInstancer(const String& name);

	/// Creates a style sheet from a user-generated string.
	/// @param[in] string The contents of the style sheet.
	/// @return A pointer to the newly created style sheet.
	static SharedPtr<StyleSheet> InstanceStyleSheetString(const String& string);
	/// Creates a style sheet from a file.
	/// @param[in] file_name The location of the style sheet file.
	/// @return A pointer to the newly created style sheet.
	static SharedPtr<StyleSheet> InstanceStyleSheetFile(const String& file_name);
	/// Creates a style sheet from an Stream.
	/// @param[in] stream A pointer to the stream containing the style sheet's contents.
	/// @return A pointer to the newly created style sheet.
	static SharedPtr<StyleSheet> InstanceStyleSheetStream(Stream* stream);
	/// Clears the style sheet cache. This will force style sheets to be reloaded.
	static void ClearStyleSheetCache();
	/// Clears the template cache. This will force template to be reloaded.
	static void ClearTemplateCache();

	/// Registers an instancer for all events.
	/// @param[in] instancer The instancer to be called.
	/// @lifetime The instancer must be kept alive until after the call to Core::Shutdown.
	static void RegisterEventInstancer(EventInstancer* instancer);
	/// Instance an event object
	/// @param[in] target Target element of this event.
	/// @param[in] name Name of this event.
	/// @param[in] parameters Additional parameters for this event.
	/// @param[in] interruptible If the event propagation can be stopped.
	/// @return The instanced event.
	static EventPtr InstanceEvent(Element* target, EventId id, const String& type, const Dictionary& parameters, bool interruptible);

	/// Register the instancer to be used for all event listeners.
	/// @lifetime The instancer must be kept alive until after the call to Core::Shutdown, or until a new instancer is set.
	static void RegisterEventListenerInstancer(EventListenerInstancer* instancer);
	/// Instance an event listener with the given string. This is used for instancing listeners for the on* events from RML.
	/// @param[in] value The parameters to the event listener.
	/// @return The instanced event listener.
	static EventListener* InstanceEventListener(const String& value, Element* element);

private:
	Factory();
	~Factory();
};

}
}

#endif
