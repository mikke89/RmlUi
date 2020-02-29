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

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "DocumentHeader.h"
#include "ElementStyle.h"
#include "EventDispatcher.h"
#include "LayoutEngine.h"
#include "StreamFile.h"
#include "StyleSheetFactory.h"
#include "Template.h"
#include "TemplateCache.h"
#include "XMLParseTools.h"

namespace Rml {
namespace Core {

ElementDocument::ElementDocument(const String& tag) : Element(tag)
{
	style_sheet = nullptr;
	context = nullptr;

	modal = false;
	layout_dirty = true;

	position_dirty = false;

	ForceLocalStackingContext();
	SetOwnerDocument(this);

	SetProperty(PropertyId::Position, Property(Style::Position::Absolute));
}

ElementDocument::~ElementDocument()
{
}

void ElementDocument::ProcessHeader(const DocumentHeader* document_header)
{
	RMLUI_ZoneScoped;

	// Store the source address that we came from
	source_url = document_header->source;

	// Construct a new header and copy the template details across
	DocumentHeader header;
	header.MergePaths(header.template_resources, document_header->template_resources, document_header->source);

	// Merge in any templates, note a merge may cause more templates to merge
	for (size_t i = 0; i < header.template_resources.size(); i++)
	{
		Template* merge_template = TemplateCache::LoadTemplate(URL(header.template_resources[i]).GetURL());	

		if (merge_template)
			header.MergeHeader(*merge_template->GetHeader());
		else
			Log::Message(Log::LT_WARNING, "Template %s not found", header.template_resources[i].c_str());
	}

	// Merge the document's header last, as it is the most overriding.
	header.MergeHeader(*document_header);

	// Set the title to the document title.
	title = document_header->title;

	// If a style-sheet (or sheets) has been specified for this element, then we load them and set the combined sheet
	// on the element; all of its children will inherit it by default.
	SharedPtr<StyleSheet> new_style_sheet;
	if (header.rcss_external.size() > 0)
		new_style_sheet = StyleSheetFactory::GetStyleSheet(header.rcss_external);

	// Combine any inline sheets.
	if (header.rcss_inline.size() > 0)
	{			
		for (size_t i = 0;i < header.rcss_inline.size(); i++)
		{			
			UniquePtr<StyleSheet> inline_sheet = std::make_unique<StyleSheet>();
			auto stream = std::make_unique<StreamMemory>((const byte*) header.rcss_inline[i].c_str(), header.rcss_inline[i].size());
			stream->SetSourceURL(document_header->source);

			if (inline_sheet->LoadStyleSheet(stream.get(), header.rcss_inline_line_numbers[i]))
			{
				if (new_style_sheet)
				{
					SharedPtr<StyleSheet> combined_sheet = new_style_sheet->CombineStyleSheet(*inline_sheet);
					new_style_sheet = combined_sheet;
				}
				else
					new_style_sheet = std::move(inline_sheet);
			}

			stream.reset();
		}		
	}

	// If a style sheet is available, set it on the document and release it.
	if (new_style_sheet)
	{
		SetStyleSheet(std::move(new_style_sheet));
	}

	// Load external scripts.
	for (size_t i = 0; i < header.scripts_external.size(); i++)
	{
		auto stream = std::make_unique<StreamFile>();
		if (stream->Open(header.scripts_external[i]))
			LoadScript(stream.get(), header.scripts_external[i]);
	}

	// Load internal scripts.
	for (size_t i = 0; i < header.scripts_inline.size(); i++)
	{
		auto stream = std::make_unique<StreamMemory>((const byte*) header.scripts_inline[i].c_str(), header.scripts_inline[i].size());
		LoadScript(stream.get(), "");
	}

	// Hide this document.
	SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));

	// Update properties so that e.g. visibility status can be queried properly immediately.
	UpdateProperties();
}

// Returns the document's context.
Context* ElementDocument::GetContext()
{
	return context;
}

// Sets the document's title.
void ElementDocument::SetTitle(const String& _title)
{
	title = _title;
}

const String& ElementDocument::GetTitle() const
{
	return title;
}

const String& ElementDocument::GetSourceURL() const
{
	return source_url;
}

// Sets the style sheet this document, and all of its children, uses.
void ElementDocument::SetStyleSheet(SharedPtr<StyleSheet> _style_sheet)
{
	RMLUI_ZoneScoped;

	if (style_sheet == _style_sheet)
		return;

	style_sheet = _style_sheet;
	
	if (style_sheet)
		style_sheet->BuildNodeIndexAndOptimizeProperties();

	GetStyle()->DirtyDefinition();
}

// Returns the document's style sheet.
const SharedPtr<StyleSheet>& ElementDocument::GetStyleSheet() const
{
	return style_sheet;
}

// Brings the document to the front of the document stack.
void ElementDocument::PullToFront()
{
	if (context != nullptr)
		context->PullDocumentToFront(this);
}

// Sends the document to the back of the document stack.
void ElementDocument::PushToBack()
{
	if (context != nullptr)
		context->PushDocumentToBack(this);
}

void ElementDocument::Show(ModalFlag modal_flag, FocusFlag focus_flag)
{
	switch (modal_flag)
	{
	case Rml::Core::ModalFlag::None:     modal = false; break;
	case Rml::Core::ModalFlag::Modal:    modal = true;  break;
	case Rml::Core::ModalFlag::Keep: break;
	}

	bool focus = false;
	bool autofocus = false;
	bool focus_previous = false;

	switch (focus_flag)
	{
	case FocusFlag::None:
		break;
	case FocusFlag::Document:
		focus = true;
		break;
	case FocusFlag::Keep:
		focus = true;
		focus_previous = true;
		break;
	case FocusFlag::Auto:
		focus = true;
		autofocus = true;
		break;
	}

	// Set to visible and switch focus if necessary
	SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));
	
	// We should update the document now, otherwise the focusing methods below do not think we are visible
	// If this turns out to be slow, the more performant approach is just to compute the new visibility property
	UpdateDocument();

	if (focus)
	{
		Element* focus_element = this;

		if (autofocus)
		{
			Element* first_element = nullptr;
			Element* element = FindNextTabElement(this, true);

			while (element && element != first_element)
			{
				if (!first_element)
					first_element = element;

				if (element->HasAttribute("autofocus"))
				{
					focus_element = element;
					break;
				}

				element = FindNextTabElement(element, true);
			}
		}
		else if (focus_previous)
		{
			focus_element = GetFocusLeafNode();
		}

		// Focus the window or element
		bool focused = focus_element->Focus();
		if (focused && focus_element != this)
			focus_element->ScrollIntoView(false);
	}

	DispatchEvent(EventId::Show, Dictionary());
}

void ElementDocument::Hide()
{
	SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));

	// We should update the document now, so that the (un)focusing will get the correct visibility
	UpdateDocument();

	DispatchEvent(EventId::Hide, Dictionary());
	
	if (context)
	{
		context->UnfocusDocument(this);
	}
}

// Close this document
void ElementDocument::Close()
{
	if (context != nullptr)
		context->UnloadDocument(this);
}

ElementPtr ElementDocument::CreateElement(const String& name)
{
	return Factory::InstanceElement(nullptr, name, name, XMLAttributes());
}

// Create a text element.
ElementPtr ElementDocument::CreateTextNode(const String& text)
{
	// Create the element.
	ElementPtr element = CreateElement("#text");
	if (!element)
	{
		Log::Message(Log::LT_ERROR, "Failed to create text element, instancer returned nullptr.");
		return nullptr;
	}

	// Cast up
	ElementText* element_text = rmlui_dynamic_cast< ElementText* >(element.get());
	if (!element_text)
	{
		Log::Message(Log::LT_ERROR, "Failed to create text element, instancer didn't return a derivative of ElementText.");
		return nullptr;
	}
	
	// Set the text
	element_text->SetText(text);

	return element;
}

// Is the current document modal
bool ElementDocument::IsModal() const
{
	return modal && IsVisible();
}

// Default load script implementation
void ElementDocument::LoadScript(Stream* RMLUI_UNUSED_PARAMETER(stream), const String& RMLUI_UNUSED_PARAMETER(source_name))
{
	RMLUI_UNUSED(stream);
	RMLUI_UNUSED(source_name);
}

// Updates the document, including its layout
void ElementDocument::UpdateDocument()
{
	const float dp_ratio = (context ? context->GetDensityIndependentPixelRatio() : 1.0f);
	Update(dp_ratio);
	UpdateLayout();
	UpdatePosition();
}

// Updates the layout if necessary.
void ElementDocument::UpdateLayout()
{
	// Note: Carefully consider when to call this function for performance reasons.
	// Ideally, only called once per update loop.
	if(layout_dirty)
	{
		RMLUI_ZoneScoped;
		RMLUI_ZoneText(source_url.c_str(), source_url.size());

		layout_dirty = false;

		Vector2f containing_block(0, 0);
		if (GetParentNode() != nullptr)
			containing_block = GetParentNode()->GetBox().GetSize();

		LayoutEngine layout_engine;
		layout_engine.FormatElement(this, containing_block);
	}
}

// Updates the position of the document based on the style properties.
void ElementDocument::UpdatePosition()
{
	if(position_dirty)
	{
		RMLUI_ZoneScoped;

		position_dirty = false;

		Element* root = GetParentNode();

		// We only position ourselves if we are a child of our context's root element. That is, we don't want to proceed if we are unparented or an iframe document.
		if (!root || !context || (root != context->GetRootElement()))
			return;

		Vector2f position;
		// Work out our containing block; relative offsets are calculated against it.
		Vector2f containing_block = root->GetBox().GetSize(Box::CONTENT);

		auto& computed = GetComputedValues();

		if (computed.left.type != Style::Left::Auto)
			position.x = ResolveValue(computed.left, containing_block.x);
		else if (computed.right.type != Style::Right::Auto)
			position.x = (containing_block.x - GetBox().GetSize(Box::MARGIN).x) - ResolveValue(computed.right, containing_block.x);
		else
			position.x = GetBox().GetEdge(Box::MARGIN, Box::LEFT);

		if (computed.top.type != Style::Top::Auto)
			position.y = ResolveValue(computed.top, containing_block.y);
		else if (computed.bottom.type != Style::Bottom::Auto)
			position.y = (containing_block.y - GetBox().GetSize(Box::MARGIN).y) - ResolveValue(computed.bottom, containing_block.y);
		else
			position.y = GetBox().GetEdge(Box::MARGIN, Box::TOP);

		SetOffset(position, nullptr);
	}
}

void ElementDocument::DirtyPosition()
{
	position_dirty = true;
}

void ElementDocument::DirtyLayout()
{
	layout_dirty = true;
}

bool ElementDocument::IsLayoutDirty()
{
	return layout_dirty;
}

void ElementDocument::DirtyDpProperties()
{
	GetStyle()->DirtyPropertiesWithUnitRecursive(Property::DP);
}

// Repositions the document if necessary.
void ElementDocument::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	// If the document's font-size has been changed, we need to dirty all rem properties.
	if (changed_properties.Contains(PropertyId::FontSize))
		GetStyle()->DirtyPropertiesWithUnitRecursive(Property::REM);

	if (changed_properties.Contains(PropertyId::Top) ||
		changed_properties.Contains(PropertyId::Right) ||
		changed_properties.Contains(PropertyId::Bottom) ||
		changed_properties.Contains(PropertyId::Left))
		DirtyPosition();
}

// Processes the 'onpropertychange' event, checking for a change in position or size.
void ElementDocument::ProcessDefaultAction(Event& event)
{
	Element::ProcessDefaultAction(event);

	// Process generic keyboard events for this window in bubble phase
	if (event == EventId::Keydown)
	{
		int key_identifier = event.GetParameter<int>("key_identifier", Input::KI_UNKNOWN);

		// Process TAB
		if (key_identifier == Input::KI_TAB)
		{
			if (Element* element = FindNextTabElement(event.GetTargetElement(), !event.GetParameter<bool>("shift_key", false)))
			{
				if(element->Focus())
					element->ScrollIntoView(false);
			}
		}
		// Process ENTER being pressed on a focusable object (emulate click)
		else if (key_identifier == Input::KI_RETURN ||
				 key_identifier == Input::KI_NUMPADENTER)
		{
			Element* focus_node = GetFocusLeafNode();

			if (focus_node && focus_node->GetComputedValues().tab_index == Style::TabIndex::Auto)
			{
				focus_node->Click();
			}
		}
	}
}

void ElementDocument::OnResize()
{
	DirtyPosition();
}


// Find the next element to focus, starting at the current element
//
// This algorithm is quite sneaky, I originally thought a depth first search would
// work, but it appears not. What is required is to cut the tree in half along the nodes
// from current_element up the root and then either traverse the tree in a clockwise or
// anticlock wise direction depending if you're searching forward or backward respectively
Element* ElementDocument::FindNextTabElement(Element* current_element, bool forward)
{
	// If we're searching forward, check the immediate children of this node first off
	if (forward)
	{
		for (int i = 0; i < current_element->GetNumChildren(); i++)
			if (Element* result = SearchFocusSubtree(current_element->GetChild(i), forward))
				return result;
	}

	// Now walk up the tree, testing either the bottom or top
	// of the tree, depending on whether we're going forwards
	// or backwards respectively
	//
	// If we make it all the way up to the document, then
	// we search the entire tree (to loop back round)
	bool search_enabled = false;
	Element* document = current_element->GetOwnerDocument();
	Element* child = current_element;
	Element* parent = current_element->GetParentNode();
	while (child != document)
	{
		for (int i = 0; i < parent->GetNumChildren(); i++)
		{
			// Calculate index into children
			int child_index = i;
			if (!forward)
				child_index = parent->GetNumChildren() - i - 1;
			Element* search_child = parent->GetChild(child_index);

			// Do a search if its enabled
			if (search_enabled)
				if(Element* result = SearchFocusSubtree(search_child, forward))
					return result;

			// If we find the child, enable searching
			if (search_child == child)
				search_enabled = true;
		}

		// Advance up the tree
		child = parent;
		parent = parent->GetParentNode();

		// If we hit the top, enable searching the entire tree
		if (parent == document)
			search_enabled = true;
		else // otherwise enable searching if we're going backward and disable if we're going forward
			search_enabled = false;
	}

	return nullptr;
}

Element* ElementDocument::SearchFocusSubtree(Element* element, bool forward)
{
	// Skip disabled elements
	if (element->IsPseudoClassSet("disabled"))
	{
		return nullptr;
	}
	if (!element->IsVisible())
	{
		return nullptr;
	}

	// Check if this is the node we're looking for
	if (element->GetComputedValues().tab_index == Style::TabIndex::Auto)
	{
		return element;
	}

	// Check all children
	for (int i = 0; i < element->GetNumChildren(); i++)
	{
		int child_index = i;
		if (!forward)
			child_index = element->GetNumChildren() - i - 1;
		if (Element * result = SearchFocusSubtree(element->GetChild(child_index), forward))
			return result;
	}

	return nullptr;
}

}
}
