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

#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/ContextInstancer.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/DataModelHandle.h"
#include "../../Include/RmlUi/Core/Debug.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "DataModel.h"
#include "EventDispatcher.h"
#include "PluginRegistry.h"
#include "ScrollController.h"
#include "StreamFile.h"
#include <algorithm>
#include <iterator>
#include <limits>

namespace Rml {

static constexpr float DOUBLE_CLICK_TIME = 0.5f;    // [s]
static constexpr float DOUBLE_CLICK_MAX_DIST = 3.f; // [dp]
static constexpr float UNIT_SCROLL_LENGTH = 80.f;   // [dp]

Context::Context(const String& name, RenderManager* render_manager, TextInputHandler* text_input_handler) :
	name(name), render_manager(render_manager), text_input_handler(text_input_handler)
{
	instancer = nullptr;

	root = Factory::InstanceElement(nullptr, "*", "#root", XMLAttributes());
	root->SetId(name);
	root->SetOffset(Vector2f(0, 0), nullptr);
	root->SetProperty(PropertyId::ZIndex, Property(0, Unit::NUMBER));

	cursor_proxy = Factory::InstanceElement(nullptr, documents_base_tag, documents_base_tag, XMLAttributes());
	ElementDocument* cursor_proxy_document = rmlui_dynamic_cast<ElementDocument*>(cursor_proxy.get());
	RMLUI_ASSERT(cursor_proxy_document);
	cursor_proxy_document->context = this;

	// The cursor proxy takes the style from its cloned element's document. The latter may define style rules for `<body>` which we don't want on the
	// proxy. Thus, we override some properties here that we in particular don't want to inherit from the client document, especially those that
	// result in decoration of the body element.
	cursor_proxy_document->SetProperty(PropertyId::BackgroundColor, Property(Colourb(255, 255, 255, 0), Unit::COLOUR));
	cursor_proxy_document->SetProperty(PropertyId::BorderTopWidth, Property(0, Unit::PX));
	cursor_proxy_document->SetProperty(PropertyId::BorderRightWidth, Property(0, Unit::PX));
	cursor_proxy_document->SetProperty(PropertyId::BorderBottomWidth, Property(0, Unit::PX));
	cursor_proxy_document->SetProperty(PropertyId::BorderLeftWidth, Property(0, Unit::PX));
	cursor_proxy_document->SetProperty(PropertyId::Decorator, Property());
	cursor_proxy_document->SetProperty(PropertyId::OverflowX, Property(Style::Overflow::Visible));
	cursor_proxy_document->SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Visible));

	document_focus_history.push_back(root.get());
	focus = root.get();
	hover = nullptr;
	active = nullptr;
	drag = nullptr;

	drag_started = false;
	drag_verbose = false;
	drag_clone = nullptr;
	drag_hover = nullptr;
	last_click_element = nullptr;
	last_click_time = 0;

	mouse_active = false;
	enable_cursor = true;

	scroll_controller = MakeUnique<ScrollController>();
}

Context::~Context()
{
	PluginRegistry::NotifyContextDestroy(this);

	UnloadAllDocuments();

	ReleaseUnloadedDocuments();

	root.reset();

	cursor_proxy.reset();

	instancer = nullptr;
}

const String& Context::GetName() const
{
	return name;
}

void Context::SetDimensions(const Vector2i _dimensions)
{
	if (dimensions != _dimensions)
	{
		dimensions = _dimensions;
		render_manager->SetViewport(dimensions);
		root->SetBox(Box(Vector2f(dimensions)));
		root->DirtyLayout();

		for (int i = 0; i < root->GetNumChildren(); ++i)
		{
			ElementDocument* document = root->GetChild(i)->GetOwnerDocument();
			if (document != nullptr)
			{
				document->DirtyMediaQueries();
				document->DirtyVwAndVhProperties();
				document->DirtyLayout();
				document->DirtyPosition();
				document->DispatchEvent(EventId::Resize, Dictionary());
			}
		}
	}
}

Vector2i Context::GetDimensions() const
{
	return dimensions;
}

void Context::SetDensityIndependentPixelRatio(float _density_independent_pixel_ratio)
{
	if (density_independent_pixel_ratio != _density_independent_pixel_ratio)
	{
		density_independent_pixel_ratio = _density_independent_pixel_ratio;

		for (int i = 0; i < root->GetNumChildren(true); ++i)
		{
			ElementDocument* document = root->GetChild(i)->GetOwnerDocument();
			if (document)
			{
				document->DirtyMediaQueries();
				document->OnDpRatioChangeRecursive();
			}
		}
	}
}

float Context::GetDensityIndependentPixelRatio() const
{
	return density_independent_pixel_ratio;
}

bool Context::Update()
{
	RMLUI_ZoneScoped;

	next_update_timeout = std::numeric_limits<double>::infinity();

	if (scroll_controller->Update(mouse_position, density_independent_pixel_ratio))
		RequestNextUpdate(0);

	// Update the hover chain to detect any new or moved elements under the mouse.
	if (mouse_active)
		UpdateHoverChain(mouse_position);

	// Update all the data models before updating properties and layout.
	for (auto& data_model : data_models)
		data_model.second->Update(true);

	// The style definition of each document should be independent of each other. By manually resetting these flags we avoid unnecessary definition
	// lookups in unrelated documents, such as when adding a new document. Adding an element dirties the parent definition, which in this case is the
	// root. By extension the definition of all the other documents are also dirtied, unnecessarily.
	root->dirty_definition = false;
	root->dirty_child_definitions = false;

	root->Update(density_independent_pixel_ratio, Vector2f(dimensions));

	for (int i = 0; i < root->GetNumChildren(); ++i)
	{
		if (auto doc = root->GetChild(i)->GetOwnerDocument())
		{
			doc->UpdateLayout();
			doc->UpdatePosition();
		}
	}

	// Release any documents that were unloaded during the update.
	ReleaseUnloadedDocuments();

	return true;
}

bool Context::Render()
{
	RMLUI_ZoneScoped;

	render_manager->PrepareRender();

	root->Render();

	// Render the cursor proxy so that any attached drag clone will be rendered below the cursor.
	if (drag_clone)
	{
		static_cast<ElementDocument&>(*cursor_proxy).UpdateDocument();
		cursor_proxy->SetOffset(
			Vector2f((float)Math::Clamp(mouse_position.x, 0, dimensions.x), (float)Math::Clamp(mouse_position.y, 0, dimensions.y)), nullptr);
		cursor_proxy->Render();
	}

	render_manager->ResetState();

	return true;
}

ElementDocument* Context::CreateDocument(const String& instancer_name)
{
	ElementPtr element = Factory::InstanceElement(nullptr, instancer_name, documents_base_tag, XMLAttributes());
	if (!element)
	{
		Log::Message(Log::LT_ERROR, "Failed to instance document on instancer_name '%s', instancer returned nullptr.", instancer_name.c_str());
		return nullptr;
	}

	ElementDocument* document = rmlui_dynamic_cast<ElementDocument*>(element.get());
	if (!document)
	{
		Log::Message(Log::LT_ERROR,
			"Failed to instance document on instancer_name '%s', Found type '%s', was expecting derivative of ElementDocument.",
			instancer_name.c_str(), rmlui_type_name(*element));
		return nullptr;
	}

	document->context = this;
	root->AppendChild(std::move(element));

	PluginRegistry::NotifyDocumentLoad(document);

	return document;
}

ElementDocument* Context::LoadDocument(const String& document_path)
{
	auto stream = MakeUnique<StreamFile>();

	if (!stream->Open(document_path))
		return nullptr;

	ElementDocument* document = LoadDocument(stream.get());

	return document;
}

ElementDocument* Context::LoadDocument(Stream* stream)
{
	PluginRegistry::NotifyDocumentOpen(this, stream->GetSourceURL().GetURL());

	ElementPtr element = Factory::InstanceDocumentStream(this, stream, GetDocumentsBaseTag());
	if (!element)
		return nullptr;

	ElementDocument* document = rmlui_static_cast<ElementDocument*>(element.get());

	root->AppendChild(std::move(element));

	// The 'load' event is fired before updating the document, because the user might
	// need to initalize things before running an update. The drawback is that computed
	// values and layouting are not performed yet, resulting in default values when
	// querying such information in the event handler.
	PluginRegistry::NotifyDocumentLoad(document);
	document->DispatchEvent(EventId::Load, Dictionary());

	// Data models are updated after the 'load' event so that the user has a chance to change
	// any data variables first. We do not clear dirty variables here, since users may need to
	// retrieve whether or not eg. a data variable has changed in a controller.
	for (auto& data_model : data_models)
		data_model.second->Update(false);

	document->UpdateDocument();

	return document;
}

ElementDocument* Context::LoadDocumentFromMemory(const String& string, const String& source_url)
{
	// Open the stream based on the string contents.
	auto stream = MakeUnique<StreamMemory>(reinterpret_cast<const byte*>(string.c_str()), string.size());

	stream->SetSourceURL(source_url);

	// Load the document from the stream.
	ElementDocument* document = LoadDocument(stream.get());

	return document;
}

void Context::UnloadDocument(ElementDocument* _document)
{
	// Has this document already been unloaded?
	for (size_t i = 0; i < unloaded_documents.size(); ++i)
	{
		if (unloaded_documents[i].get() == _document)
			return;
	}

	ElementDocument* document = _document;

	if (document->GetParentNode() == root.get())
	{
		// Dispatch the unload notifications.
		document->DispatchEvent(EventId::Unload, Dictionary());
		PluginRegistry::NotifyDocumentUnload(document);

		// Move document to a temporary location to be released later.
		unloaded_documents.push_back(root->RemoveChild(document));
	}

	// Remove the item from the focus history.
	ElementList::iterator itr = std::find(document_focus_history.begin(), document_focus_history.end(), document);
	if (itr != document_focus_history.end())
		document_focus_history.erase(itr);

	// Focus to the previous document if the old document is the current focus.
	if (focus && focus->GetOwnerDocument() == document)
	{
		focus = nullptr;
		document_focus_history.back()->GetFocusLeafNode()->Focus();
	}

	// Clear the active element if the old document is the active element.
	if (active && active->GetOwnerDocument() == document)
	{
		active = nullptr;
	}

	// Clear other pointers to elements whose owner was deleted
	if (drag && drag->GetOwnerDocument() == document)
	{
		drag = nullptr;
		ReleaseDragClone();
	}

	if (drag_hover && drag_hover->GetOwnerDocument() == document)
	{
		drag_hover = nullptr;
	}

	// Rebuild the hover state.
	UpdateHoverChain(mouse_position);
}

void Context::UnloadAllDocuments()
{
	// Unload all children.
	while (root->GetNumChildren(true) > 0)
		UnloadDocument(root->GetChild(0)->GetOwnerDocument());

	// The element lists may point to elements that are getting removed.
	active_chain.clear();
	hover_chain.clear();
	drag_hover_chain.clear();
}

void Context::EnableMouseCursor(bool enable)
{
	// The cursor is set to an invalid name so that it is forced to update in the next update loop.
	cursor_name = ":reset:";
	enable_cursor = enable;
}

void Context::ActivateTheme(const String& theme_name, bool activate)
{
	bool theme_changed = false;

	if (activate)
		theme_changed = active_themes.insert(theme_name).second;
	else
		theme_changed = (active_themes.erase(theme_name) > 0);

	if (theme_changed)
	{
		for (int i = 0; i < root->GetNumChildren(true); ++i)
		{
			if (ElementDocument* document = root->GetChild(i)->GetOwnerDocument())
				document->DirtyMediaQueries();
		}
	}
}

bool Context::IsThemeActive(const String& theme_name) const
{
	return active_themes.count(theme_name);
}

ElementDocument* Context::GetDocument(const String& id)
{
	for (int i = 0; i < root->GetNumChildren(); i++)
	{
		ElementDocument* document = root->GetChild(i)->GetOwnerDocument();
		if (document == nullptr)
			continue;

		if (document->GetId() == id)
			return document;
	}

	return nullptr;
}

ElementDocument* Context::GetDocument(int index)
{
	Element* element = root->GetChild(index);
	if (element == nullptr)
		return nullptr;

	return element->GetOwnerDocument();
}

int Context::GetNumDocuments() const
{
	return root->GetNumChildren();
}

Element* Context::GetHoverElement()
{
	return hover;
}

Element* Context::GetFocusElement()
{
	return focus;
}

Element* Context::GetRootElement()
{
	return root.get();
}

void Context::PullDocumentToFront(ElementDocument* document)
{
	if (document != root->GetLastChild())
	{
		// Calling RemoveChild() / AppendChild() would be cleaner, but that dirties the document's layout
		// unnecessarily, so we'll go under the hood here.
		for (int i = 0; i < root->GetNumChildren(); ++i)
		{
			if (root->GetChild(i) == document)
			{
				ElementPtr element = std::move(root->children[i]);
				root->children.erase(root->children.begin() + i);
				root->children.insert(root->children.begin() + root->GetNumChildren(), std::move(element));

				root->DirtyStackingContext();
			}
		}
	}
}

void Context::PushDocumentToBack(ElementDocument* document)
{
	if (document != root->GetFirstChild())
	{
		// See PullDocumentToFront().
		for (int i = 0; i < root->GetNumChildren(); ++i)
		{
			if (root->GetChild(i) == document)
			{
				ElementPtr element = std::move(root->children[i]);
				root->children.erase(root->children.begin() + i);
				root->children.insert(root->children.begin(), std::move(element));

				root->DirtyStackingContext();
			}
		}
	}
}

void Context::UnfocusDocument(ElementDocument* document)
{
	auto it = std::find(document_focus_history.begin(), document_focus_history.end(), document);
	if (it != document_focus_history.end())
		document_focus_history.erase(it);

	if (!document_focus_history.empty())
		document_focus_history.back()->GetFocusLeafNode()->Focus();
}

void Context::AddEventListener(const String& event, EventListener* listener, bool in_capture_phase)
{
	root->AddEventListener(event, listener, in_capture_phase);
}

void Context::RemoveEventListener(const String& event, EventListener* listener, bool in_capture_phase)
{
	root->RemoveEventListener(event, listener, in_capture_phase);
}

bool Context::ProcessKeyDown(Input::KeyIdentifier key_identifier, int key_modifier_state)
{
	// Generate the parameters for the key event.
	Dictionary parameters;
	GenerateKeyEventParameters(parameters, key_identifier);
	GenerateKeyModifierEventParameters(parameters, key_modifier_state);

	if (focus)
		return focus->DispatchEvent(EventId::Keydown, parameters);
	else
		return root->DispatchEvent(EventId::Keydown, parameters);
}

bool Context::ProcessKeyUp(Input::KeyIdentifier key_identifier, int key_modifier_state)
{
	// Generate the parameters for the key event.
	Dictionary parameters;
	GenerateKeyEventParameters(parameters, key_identifier);
	GenerateKeyModifierEventParameters(parameters, key_modifier_state);

	if (focus)
		return focus->DispatchEvent(EventId::Keyup, parameters);
	else
		return root->DispatchEvent(EventId::Keyup, parameters);
}

bool Context::ProcessTextInput(char character)
{
	// Only the standard ASCII character set is a valid subset of UTF-8.
	if (static_cast<unsigned char>(character) > 127)
		return false;
	return ProcessTextInput(static_cast<Character>(character));
}

bool Context::ProcessTextInput(Character character)
{
	// Generate the parameters for the key event.
	String text = StringUtilities::ToUTF8(character);
	return ProcessTextInput(text);
}

bool Context::ProcessTextInput(const String& string)
{
	Element* target = (focus ? focus : root.get());

	Dictionary parameters;
	parameters["text"] = string;

	bool consumed = target->DispatchEvent(EventId::Textinput, parameters);

	return consumed;
}

bool Context::ProcessMouseMove(int x, int y, int key_modifier_state)
{
	// Check whether the mouse moved since the last event came through.
	Vector2i old_mouse_position = mouse_position;
	mouse_position = {x, y};
	const bool mouse_moved = (mouse_position != old_mouse_position || !mouse_active);
	mouse_active = true;

	// Update the current hover chain. This will send all necessary 'onmouseout', 'onmouseover', 'ondragout' and 'ondragover' messages.
	Dictionary parameters, drag_parameters;
	UpdateHoverChain(old_mouse_position, key_modifier_state, &parameters, &drag_parameters);

	// Dispatch any 'onmousemove' events.
	if (mouse_moved)
	{
		if (hover)
		{
			hover->DispatchEvent(EventId::Mousemove, parameters);

			if (drag_hover && drag_verbose)
				drag_hover->DispatchEvent(EventId::Dragmove, drag_parameters);
		}
	}

	return !IsMouseInteracting();
}

static Element* FindFocusElement(Element* element)
{
	ElementDocument* owner_document = element->GetOwnerDocument();
	if (!owner_document || owner_document->GetComputedValues().focus() == Style::Focus::None)
		return nullptr;

	while (element && element->GetComputedValues().focus() == Style::Focus::None)
	{
		element = element->GetParentNode();
	}

	return element;
}

bool Context::ProcessMouseButtonDown(int button_index, int key_modifier_state)
{
	Dictionary parameters;
	GenerateMouseEventParameters(parameters, button_index);
	GenerateKeyModifierEventParameters(parameters, key_modifier_state);

	bool propagate = true;

	if (button_index == 0)
	{
		Element* new_focus = hover;

		// Set the currently hovered element to focus if it isn't already the focus.
		if (hover)
		{
			new_focus = FindFocusElement(hover);
			if (new_focus && new_focus != focus)
			{
				if (!new_focus->Focus())
					return !IsMouseInteracting();
			}
		}

		// Save the just-pressed-on element as the pressed element.
		active = new_focus;

		// Call 'onmousedown' on every item in the hover chain, and copy the hover chain to the active chain.
		if (hover)
			propagate = hover->DispatchEvent(EventId::Mousedown, parameters);

		if (propagate)
		{
			// Check for a double-click on an element; if one has occured, we send the 'dblclick' event to the hover
			// element. If not, we'll start a timer to catch the next one.
			float mouse_distance_squared = float((mouse_position - last_click_mouse_position).SquaredMagnitude());
			float max_mouse_distance = DOUBLE_CLICK_MAX_DIST * density_independent_pixel_ratio;

			double click_time = GetSystemInterface()->GetElapsedTime();

			if (active == last_click_element && float(click_time - last_click_time) < DOUBLE_CLICK_TIME &&
				mouse_distance_squared < max_mouse_distance * max_mouse_distance)
			{
				if (hover)
					propagate = hover->DispatchEvent(EventId::Dblclick, parameters);

				last_click_element = nullptr;
				last_click_time = 0;
			}
			else
			{
				last_click_element = active;
				last_click_time = click_time;
			}
		}

		last_click_mouse_position = mouse_position;

		active_chain.insert(active_chain.end(), hover_chain.begin(), hover_chain.end());

		if (propagate)
		{
			// Traverse down the hierarchy of the newly focused element (if any), and see if we can begin dragging it.
			drag_started = false;
			drag = hover;
			while (drag)
			{
				Style::Drag drag_style = drag->GetComputedValues().drag();
				switch (drag_style)
				{
				case Style::Drag::None: drag = drag->GetParentNode(); continue;
				case Style::Drag::Block: drag = nullptr; continue;
				default: drag_verbose = (drag_style == Style::Drag::DragDrop || drag_style == Style::Drag::Clone);
				}

				break;
			}
		}
	}
	else
	{
		// Not the primary mouse button, so we're not doing any special processing.
		if (hover)
			propagate = hover->DispatchEvent(EventId::Mousedown, parameters);
	}

	if (scroll_controller->GetMode() == ScrollController::Mode::Autoscroll)
	{
		scroll_controller->Reset();
	}
	else if (button_index == 2 && hover && propagate)
	{
		Dictionary scroll_parameters;
		GenerateMouseEventParameters(scroll_parameters);
		GenerateKeyModifierEventParameters(scroll_parameters, key_modifier_state);
		scroll_parameters["autoscroll"] = true;

		// Dispatch a mouse scroll event, this gives elements an opportunity to block autoscroll from being initialized.
		if (hover->DispatchEvent(EventId::Mousescroll, scroll_parameters))
			scroll_controller->ActivateAutoscroll(hover->GetClosestScrollableContainer(), mouse_position);
	}

	return !IsMouseInteracting();
}

bool Context::ProcessMouseButtonUp(int button_index, int key_modifier_state)
{
	Dictionary parameters;
	GenerateMouseEventParameters(parameters, button_index);
	GenerateKeyModifierEventParameters(parameters, key_modifier_state);

	// We want to return the interaction state before handling the mouse up events, so that any active element that is released is considered to
	// capture the event.
	const bool result = !IsMouseInteracting();

	// Process primary click.
	if (button_index == 0)
	{
		// The elements in the new hover chain have the 'onmouseup' event called on them.
		if (hover)
			hover->DispatchEvent(EventId::Mouseup, parameters);

		// If the active element (the one that was being hovered over when the mouse button was pressed) is still being
		// hovered over, we click it.
		if (hover && active && active == FindFocusElement(hover))
		{
			active->DispatchEvent(EventId::Click, parameters);
		}

		// Unset the 'active' pseudo-class on all the elements in the active chain; because they may not necessarily
		// have had 'onmouseup' called on them, we can't guarantee this has happened already.
		for (Element* element : active_chain)
			element->SetPseudoClass("active", false);
		active_chain.clear();
		active = nullptr;

		if (drag)
		{
			if (drag_started)
			{
				Dictionary drag_parameters;
				GenerateMouseEventParameters(drag_parameters);
				GenerateDragEventParameters(drag_parameters);
				GenerateKeyModifierEventParameters(drag_parameters, key_modifier_state);

				if (drag_hover)
				{
					if (drag_verbose)
					{
						drag_hover->DispatchEvent(EventId::Dragdrop, drag_parameters);
						// User may have removed the element, do an extra check.
						if (drag_hover)
							drag_hover->DispatchEvent(EventId::Dragout, drag_parameters);
					}
				}

				if (drag)
					drag->DispatchEvent(EventId::Dragend, drag_parameters);

				ReleaseDragClone();
			}

			drag = nullptr;
			drag_hover = nullptr;
			drag_hover_chain.clear();

			// We may have changes under our mouse, this ensures that the hover chain is properly updated
			ProcessMouseMove(mouse_position.x, mouse_position.y, key_modifier_state);
		}
	}
	else
	{
		// Not the left mouse button, so we're not doing any special processing.
		if (hover)
			hover->DispatchEvent(EventId::Mouseup, parameters);
	}

	// If we have autoscrolled while holding the middle mouse button, release the autoscroll mode now.
	if (scroll_controller->HasAutoscrollMoved())
		scroll_controller->Reset();

	return result;
}

bool Context::ProcessMouseWheel(float wheel_delta, int key_modifier_state)
{
	return ProcessMouseWheel(Vector2f{0.f, wheel_delta}, key_modifier_state);
}

bool Context::ProcessMouseWheel(Vector2f wheel_delta, int key_modifier_state)
{
	if (scroll_controller->GetMode() == ScrollController::Mode::Autoscroll)
	{
		scroll_controller->Reset();
		return false;
	}
	else if (!hover)
	{
		scroll_controller->Reset();
		return true;
	}

	Dictionary scroll_parameters;
	GenerateMouseEventParameters(scroll_parameters);
	GenerateKeyModifierEventParameters(scroll_parameters, key_modifier_state);
	scroll_parameters["wheel_delta_x"] = wheel_delta.x;
	scroll_parameters["wheel_delta_y"] = wheel_delta.y;

	// Dispatch a mouse scroll event, this gives elements an opportunity to block scrolling from being performed.
	if (!hover->DispatchEvent(EventId::Mousescroll, scroll_parameters))
		return false;

	const float unit_scroll_length = UNIT_SCROLL_LENGTH * density_independent_pixel_ratio;
	const Vector2f scroll_length = wheel_delta * unit_scroll_length;
	Element* target = hover->GetClosestScrollableContainer();

	if (scroll_controller->GetMode() == ScrollController::Mode::Smoothscroll && scroll_controller->GetTarget() == target)
		scroll_controller->IncrementSmoothscrollTarget(scroll_length);
	else
		scroll_controller->ActivateSmoothscroll(target, scroll_length, ScrollBehavior::Auto);

	return target == nullptr;
}

bool Context::ProcessMouseLeave()
{
	mouse_active = false;

	// Update the hover chain. Now that 'mouse_active' is disabled this will remove the hover state from all elements.
	UpdateHoverChain(mouse_position);

	return !IsMouseInteracting();
}

bool Context::IsMouseInteracting() const
{
	return (hover && hover != root.get()) || (active && active != root.get()) || scroll_controller->GetMode() == ScrollController::Mode::Autoscroll;
}

void Context::SetDefaultScrollBehavior(ScrollBehavior scroll_behavior, float speed_factor)
{
	scroll_controller->SetDefaultScrollBehavior(scroll_behavior, speed_factor);
}

RenderManager& Context::GetRenderManager()
{
	return *render_manager;
}

TextInputHandler* Context::GetTextInputHandler() const
{
	return text_input_handler;
}

void Context::SetInstancer(ContextInstancer* _instancer)
{
	RMLUI_ASSERT(instancer == nullptr);
	instancer = _instancer;
}

DataModelConstructor Context::CreateDataModel(const String& name, DataTypeRegister* data_type_register)
{
	if (!data_type_register)
	{
		if (!default_data_type_register)
			default_data_type_register = MakeUnique<DataTypeRegister>();
		data_type_register = default_data_type_register.get();
	}

	auto result = data_models.emplace(name, MakeUnique<DataModel>(data_type_register));
	bool inserted = result.second;
	if (inserted)
		return DataModelConstructor(result.first->second.get());

	Log::Message(Log::LT_ERROR, "Data model name '%s' already exists.", name.c_str());
	return DataModelConstructor();
}

DataModelConstructor Context::GetDataModel(const String& name)
{
	if (DataModel* model = GetDataModelPtr(name))
		return DataModelConstructor(model);

	Log::Message(Log::LT_ERROR, "Data model name '%s' could not be found.", name.c_str());
	return DataModelConstructor();
}

bool Context::RemoveDataModel(const String& name)
{
	auto it = data_models.find(name);
	if (it == data_models.end())
		return false;

	DataModel* model = it->second.get();
	ElementList elements = model->GetAttachedModelRootElements();

	for (Element* element : elements)
		element->SetDataModel(nullptr);

	data_models.erase(it);

	return true;
}

void Context::OnElementDetach(Element* element)
{
	auto it_hover = hover_chain.find(element);
	if (it_hover != hover_chain.end())
	{
		Dictionary parameters;
		GenerateMouseEventParameters(parameters, -1);
		element->DispatchEvent(EventId::Mouseout, parameters);

		hover_chain.erase(it_hover);

		if (hover == element)
			hover = nullptr;
	}

	auto it_active = std::find(active_chain.begin(), active_chain.end(), element);
	if (it_active != active_chain.end())
	{
		active_chain.erase(it_active);

		if (active == element)
			active = nullptr;
	}

	if (drag)
	{
		auto it = drag_hover_chain.find(element);
		if (it != drag_hover_chain.end())
		{
			drag_hover_chain.erase(it);

			if (drag_hover == element)
				drag_hover = nullptr;
		}

		if (drag == element)
		{
			// The dragged element is being removed, silently cancel the drag operation
			if (drag_started)
				ReleaseDragClone();

			drag = nullptr;
			drag_hover = nullptr;
			drag_hover_chain.clear();
		}
	}

	// Focus normally cleared and set by parent during Element::RemoveChild.
	// However, there are some exceptions, such as when an there are multiple
	// ElementDocuments in the hierarchy above the current element.
	if (element == focus)
		focus = nullptr;

	// If the element is a document lower down in the hierarchy, we may need to remove it from the focus history.
	if (element->GetOwnerDocument() == element)
	{
		auto it = std::find(document_focus_history.begin(), document_focus_history.end(), element);
		if (it != document_focus_history.end())
			document_focus_history.erase(it);
	}

	if (scroll_controller->GetTarget() == element)
		scroll_controller->Reset();
}

bool Context::OnFocusChange(Element* new_focus, bool focus_visible)
{
	RMLUI_ASSERT(new_focus);

	ElementSet old_chain;
	ElementSet new_chain;

	Element* old_focus = focus;
	ElementDocument* old_document = old_focus ? old_focus->GetOwnerDocument() : nullptr;
	ElementDocument* new_document = new_focus->GetOwnerDocument();

	// If the current focus is modal and the new focus is cannot receive focus from modal, deny the request.
	if (old_document && old_document->IsModal() && (!new_document || !(new_document->IsModal() || new_document->IsFocusableFromModal())))
		return false;

	// Build the old chains
	Element* element = old_focus;
	while (element)
	{
		old_chain.insert(element);
		element = element->GetParentNode();
	}

	// Build the new chain
	element = new_focus;
	while (element)
	{
		new_chain.insert(element);
		element = element->GetParentNode();
	}

	// Send out blur/focus events.
	Dictionary parameters;
	SendEvents(old_chain, new_chain, EventId::Blur, parameters);

	if (focus_visible)
		parameters["focus_visible"] = true;

	SendEvents(new_chain, old_chain, EventId::Focus, parameters);

	focus = new_focus;

	// Raise the element's document to the front, if desired.
	ElementDocument* document = focus->GetOwnerDocument();
	if (document != nullptr)
	{
		Style::ZIndex z_index_property = document->GetComputedValues().z_index();
		if (z_index_property.type == Style::ZIndex::Auto)
			document->PullToFront();
	}

	// Update the focus history
	if (old_document != new_document)
	{
		// If documents have changed, add the new document to the end of the history
		ElementList::iterator itr = std::find(document_focus_history.begin(), document_focus_history.end(), new_document);
		if (itr != document_focus_history.end())
			document_focus_history.erase(itr);

		if (new_document != nullptr)
			document_focus_history.push_back(new_document);
	}

	return true;
}

void Context::GenerateClickEvent(Element* element)
{
	Dictionary parameters;
	GenerateMouseEventParameters(parameters, 0);

	element->DispatchEvent(EventId::Click, parameters);
}

void Context::UpdateHoverChain(Vector2i old_mouse_position, int key_modifier_state, Dictionary* out_parameters, Dictionary* out_drag_parameters)
{
	const Vector2f position(mouse_position);

	Dictionary local_parameters, local_drag_parameters;
	Dictionary& parameters = out_parameters ? *out_parameters : local_parameters;
	Dictionary& drag_parameters = out_drag_parameters ? *out_drag_parameters : local_drag_parameters;

	// Generate the parameters for the mouse events (there could be a few!).
	GenerateMouseEventParameters(parameters);
	GenerateKeyModifierEventParameters(parameters, key_modifier_state);

	GenerateMouseEventParameters(drag_parameters);
	GenerateDragEventParameters(drag_parameters);
	GenerateKeyModifierEventParameters(drag_parameters, key_modifier_state);

	// Send out drag events.
	if (drag)
	{
		if (mouse_position != old_mouse_position)
		{
			if (!drag_started)
			{
				Dictionary drag_start_parameters = drag_parameters;
				drag_start_parameters["mouse_x"] = old_mouse_position.x;
				drag_start_parameters["mouse_y"] = old_mouse_position.y;
				drag->DispatchEvent(EventId::Dragstart, drag_start_parameters);
				drag_started = true;

				if (drag->GetComputedValues().drag() == Style::Drag::Clone)
				{
					// Clone the element and attach it to the mouse cursor.
					CreateDragClone(drag);
				}
			}

			drag->DispatchEvent(EventId::Drag, drag_parameters);
		}
	}

	hover = mouse_active ? GetElementAtPoint(position) : nullptr;

	if (enable_cursor)
	{
		String new_cursor_name;

		if (scroll_controller->GetMode() == ScrollController::Mode::Autoscroll)
			new_cursor_name = scroll_controller->GetAutoscrollCursor(mouse_position, density_independent_pixel_ratio);
		else if (drag)
			new_cursor_name = drag->GetComputedValues().cursor();
		else if (hover)
			new_cursor_name = hover->GetComputedValues().cursor();

		if (new_cursor_name != cursor_name)
		{
			GetSystemInterface()->SetMouseCursor(new_cursor_name);
			cursor_name = new_cursor_name;
		}
	}

	// Build the new hover chain.
	ElementSet new_hover_chain;
	Element* element = hover;
	while (element != nullptr)
	{
		new_hover_chain.insert(element);
		element = element->GetParentNode();
	}

	// Send mouseout / mouseover events.
	SendEvents(hover_chain, new_hover_chain, EventId::Mouseout, parameters);
	SendEvents(new_hover_chain, hover_chain, EventId::Mouseover, parameters);

	// Send out drag events.
	if (drag && mouse_active)
	{
		drag_hover = GetElementAtPoint(position, drag);

		ElementSet new_drag_hover_chain;
		element = drag_hover;
		while (element != nullptr)
		{
			new_drag_hover_chain.insert(element);
			element = element->GetParentNode();
		}

		if (drag_started && drag_verbose)
		{
			// Send out ondragover and ondragout events as appropriate.
			SendEvents(drag_hover_chain, new_drag_hover_chain, EventId::Dragout, drag_parameters);
			SendEvents(new_drag_hover_chain, drag_hover_chain, EventId::Dragover, drag_parameters);
		}

		drag_hover_chain.swap(new_drag_hover_chain);
	}

	// Swap the new chain in.
	hover_chain.swap(new_hover_chain);
}

Element* Context::GetElementAtPoint(Vector2f point, const Element* ignore_element, Element* element) const
{
	if (!element)
	{
		if (ignore_element == root.get())
			return nullptr;

		element = root.get();
	}

	bool is_modal = false;
	ElementDocument* focus_document = nullptr;

	// If we have modal focus, only check down documents that can receive focus from modals.
	if (element == root.get() && focus)
	{
		focus_document = focus->GetOwnerDocument();
		if (focus_document && focus_document->IsModal())
			is_modal = true;
	}

	// Check any elements within our stacking context. We want to return the lowest-down element
	// that is under the cursor.
	if (element->local_stacking_context)
	{
		if (element->stacking_context_dirty)
			element->BuildLocalStackingContext();

		for (int i = (int)element->stacking_context.size() - 1; i >= 0; --i)
		{
			Element* stacking_child = element->stacking_context[i];
			if (ignore_element)
			{
				// Check if the element is a descendant of the element we're ignoring.
				Element* element_hierarchy = stacking_child;
				while (element_hierarchy)
				{
					if (element_hierarchy == ignore_element)
						break;

					element_hierarchy = element_hierarchy->GetParentNode();
				}

				if (element_hierarchy)
					continue;
			}

			if (is_modal)
			{
				ElementDocument* child_document = stacking_child->GetOwnerDocument();
				if (!child_document || !(child_document == focus_document || child_document->IsFocusableFromModal()))
					continue;
			}

			Element* child_element = GetElementAtPoint(point, ignore_element, stacking_child);
			if (child_element)
				return child_element;
		}
	}

	// Ignore elements whose pointer events are disabled.
	if (element->GetComputedValues().pointer_events() == Style::PointerEvents::None)
		return nullptr;

	// Projection may fail if we have a singular transformation matrix.
	bool projection_result = element->Project(point);

	// Check if the point is actually within this element.
	bool within_element = (projection_result && element->IsPointWithinElement(point));
	if (within_element)
	{
		// The element may have been clipped out of view if it overflows an ancestor, so check its clipping region.
		Rectanglei clip_region;
		if (ElementUtilities::GetClippingRegion(element, clip_region))
			within_element = clip_region.Contains(Vector2i(point));
	}

	if (within_element)
		return element;

	return nullptr;
}

void Context::CreateDragClone(Element* element)
{
	RMLUI_ASSERTMSG(cursor_proxy, "Unable to create drag clone, no cursor proxy document.");

	ReleaseDragClone();

	// Instance the drag clone.
	ElementPtr element_drag_clone = element->Clone();
	if (!element_drag_clone)
	{
		Log::Message(Log::LT_ERROR, "Unable to duplicate drag clone.");
		return;
	}

	// Set the style sheet on the cursor proxy.
	if (ElementDocument* document = element->GetOwnerDocument())
	{
		// Borrow the target document's style sheet. Sharing style sheet containers should be used with care, and
		// only within the same context.
		static_cast<ElementDocument&>(*cursor_proxy).SetStyleSheetContainer(document->style_sheet_container);
	}

	drag_clone = element_drag_clone.get();

	// Append the clone to the cursor proxy element.
	cursor_proxy->AppendChild(std::move(element_drag_clone));

	// Position the clone. Use projected mouse coordinates to handle any ancestor transforms.
	const Vector2f absolute_pos = element->GetAbsoluteOffset(BoxArea::Border);
	Vector2f projected_mouse_position = Vector2f(mouse_position);
	if (Element* parent = element->GetParentNode())
		parent->Project(projected_mouse_position);

	drag_clone->SetProperty(PropertyId::Position, Property(Style::Position::Absolute));
	drag_clone->SetProperty(PropertyId::Left, Property(absolute_pos.x - projected_mouse_position.x, Unit::PX));
	drag_clone->SetProperty(PropertyId::Top, Property(absolute_pos.y - projected_mouse_position.y, Unit::PX));
	// We remove margins so that percentage- and auto-margins are evaluated correctly.
	drag_clone->SetProperty(PropertyId::MarginLeft, Property(0.f, Unit::PX));
	drag_clone->SetProperty(PropertyId::MarginTop, Property(0.f, Unit::PX));
	drag_clone->SetPseudoClass("drag", true);
}

void Context::ReleaseDragClone()
{
	if (drag_clone)
	{
		cursor_proxy->RemoveChild(drag_clone);
		drag_clone = nullptr;
		static_cast<ElementDocument&>(*cursor_proxy).SetStyleSheetContainer(nullptr);
	}
}

void Context::PerformSmoothscrollOnTarget(Element* target, Vector2f delta_offset, ScrollBehavior scroll_behavior)
{
	scroll_controller->ActivateSmoothscroll(target, delta_offset, scroll_behavior);
}

DataModel* Context::GetDataModelPtr(const String& name) const
{
	auto it = data_models.find(name);
	if (it != data_models.end())
		return it->second.get();
	return nullptr;
}

void Context::GenerateKeyEventParameters(Dictionary& parameters, Input::KeyIdentifier key_identifier)
{
	parameters["key_identifier"] = (int)key_identifier;
}

void Context::GenerateMouseEventParameters(Dictionary& parameters, int button_index)
{
	parameters.reserve(3);
	parameters["mouse_x"] = mouse_position.x;
	parameters["mouse_y"] = mouse_position.y;
	if (button_index >= 0)
		parameters["button"] = button_index;
}

void Context::GenerateKeyModifierEventParameters(Dictionary& parameters, int key_modifier_state)
{
	static const String property_names[] = {"ctrl_key", "shift_key", "alt_key", "meta_key", "caps_lock_key", "num_lock_key", "scroll_lock_key"};

	for (int i = 0; i < 7; i++)
		parameters[property_names[i]] = (int)((key_modifier_state & (1 << i)) > 0);
}

void Context::GenerateDragEventParameters(Dictionary& parameters)
{
	parameters["drag_element"] = (void*)drag;
}

void Context::ReleaseUnloadedDocuments()
{
	if (!unloaded_documents.empty())
	{
		OwnedElementList documents = std::move(unloaded_documents);
		unloaded_documents.clear();

		// Clear the deleted list.
		for (size_t i = 0; i < documents.size(); ++i)
			documents[i]->GetEventDispatcher()->DetachAllEvents();
		documents.clear();
	}
}

using ElementObserverList = Vector<ObserverPtr<Element>>;

class ElementObserverListBackInserter {
public:
	using iterator_category = std::output_iterator_tag;
	using value_type = void;
	using difference_type = void;
	using pointer = void;
	using reference = void;
	using container_type = ElementObserverList;

	ElementObserverListBackInserter(ElementObserverList& elements) : elements(&elements) {}
	ElementObserverListBackInserter& operator=(Element* element)
	{
		elements->push_back(element->GetObserverPtr());
		return *this;
	}
	ElementObserverListBackInserter& operator*() { return *this; }
	ElementObserverListBackInserter& operator++() { return *this; }
	ElementObserverListBackInserter& operator++(int) { return *this; }

private:
	ElementObserverList* elements;
};

void Context::SendEvents(const ElementSet& old_items, const ElementSet& new_items, EventId id, const Dictionary& parameters)
{
	// We put our elements in observer pointers in case some of them are deleted during dispatch.
	ElementObserverList elements;
	std::set_difference(old_items.begin(), old_items.end(), new_items.begin(), new_items.end(), ElementObserverListBackInserter(elements));
	for (auto& element : elements)
	{
		if (element)
			element->DispatchEvent(id, parameters);
	}
}

void Context::Release()
{
	if (instancer)
	{
		instancer->ReleaseContext(this);
	}
}

void Context::SetDocumentsBaseTag(const String& tag)
{
	documents_base_tag = tag;
}

const String& Context::GetDocumentsBaseTag()
{
	return documents_base_tag;
}

void Context::RequestNextUpdate(double delay)
{
	RMLUI_ASSERT(delay >= 0.0);
	next_update_timeout = Math::Min(next_update_timeout, delay);
}

double Context::GetNextUpdateDelay() const
{
	return next_update_timeout;
}

} // namespace Rml
