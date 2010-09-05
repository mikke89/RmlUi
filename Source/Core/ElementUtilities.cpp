/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include <Rocket/Core/ElementUtilities.h>
#include <queue>
#include "FontFaceHandle.h"
#include "LayoutEngine.h"
#include <Rocket/Core.h>

namespace Rocket {
namespace Core {

struct ClipRegion
{
	ClipRegion() : origin(-1, -1), dimensions(-1, -1)
	{
	}

	ClipRegion(const Vector2i& origin, const Vector2i& dimensions) : origin(origin), dimensions(dimensions)
	{
	}

	int AddClipElement(Rocket::Core::Element* element)
	{
		int num_ignored_clips = 0;
		const Property* clip_property = element->GetProperty(CLIP);
		if (clip_property->unit == Property::NUMBER)
			num_ignored_clips = clip_property->Get< int >();
		else if (clip_property->Get< int >() == CLIP_NONE)
			num_ignored_clips = -1;

		// Ignore nodes that don't clip.
		if ((element->GetProperty(OVERFLOW_X)->Get< int >() == OVERFLOW_VISIBLE &&
			 element->GetProperty(OVERFLOW_Y)->Get< int >() == OVERFLOW_VISIBLE) ||
			(element->GetClientWidth() > element->GetScrollWidth() &&
			 element->GetClientHeight() > element->GetScrollHeight()))
		{
			return num_ignored_clips;
		}

		Vector2f element_origin_f = element->GetAbsoluteOffset(/*clipping_element->GetClientArea()*/ Box::CONTENT);
		Vector2f element_dimensions_f = element->GetBox().GetSize(/*clipping_element->GetClientArea()*/ Box::CONTENT);

		Vector2i element_origin(Math::RealToInteger(element_origin_f.x), Math::RealToInteger(element_origin_f.y));
		Vector2i element_dimensions(Math::RealToInteger(element_dimensions_f.x), Math::RealToInteger(element_dimensions_f.y));

		if (origin == Vector2i(-1, -1) &&
			dimensions == Vector2i(-1, -1))
		{
			origin = element_origin;
			dimensions = element_dimensions;
		}
		else
		{
			Vector2i top_left(Math::Max(origin.x, element_origin.x),
										 Math::Max(origin.y, element_origin.y));

			Vector2i bottom_right(Math::Min(origin.x + dimensions.x, element_origin.x + element_dimensions.x),
											 Math::Min(origin.y + dimensions.y, element_origin.y + element_dimensions.y));

			origin = top_left;
			dimensions.x = Math::Max(0, bottom_right.x - top_left.x);
			dimensions.y = Math::Max(0, bottom_right.y - top_left.y);
		}

		return num_ignored_clips;
	}

	Vector2i origin;
	Vector2i dimensions;
};

//typedef std::vector< ClippingRegion > ClippingRegionStack;

struct ClipState
{
	ClipState() : clip_region(Vector2i(-1, -1), Vector2i(-1, -1))
	{
		clip_on = false;
	}

	bool clip_on;
	ClipRegion clip_region;
};

// The current clipping state.
typedef std::map< RenderInterface*, ClipState > ClipStateMap;
ClipStateMap clip_states;

// Returns true if the element is visible within the current clipping region (if any), false if not.
//static bool IsElementVisible(const Element* element);

// Builds and sets the box for an element.
static void SetBox(Element* element);
// Positions an element relative to an offset parent.
static void SetElementOffset(Element* element, const Vector2f& offset);

Element* ElementUtilities::GetElementById(Element* root_element, const String& id)
{
	// Breadth first search on elements for the corresponding id
	typedef std::queue<Element*> SearchQueue;
	SearchQueue search_queue;
	search_queue.push(root_element);

	// Lowercase the id for searching
	String lower_id = id.ToLower();
	
	while (!search_queue.empty())
	{
		Element* element = search_queue.front();
		search_queue.pop();
		
		if (element->GetId() == lower_id)
		{
			return element;
		}
		
		// Add all children to search
		for (int i = 0; i < element->GetNumChildren(); i++)
			search_queue.push(element->GetChild(i));
	}

	return NULL;
}

void ElementUtilities::GetElementsByTagName(ElementList& elements, Element* root_element, const String& tag)
{
	// Breadth first search on elements for the corresponding id
	typedef std::queue< Element* > SearchQueue;
	SearchQueue search_queue;
	for (int i = 0; i < root_element->GetNumChildren(); ++i)
		search_queue.push(root_element->GetChild(i));

	while (!search_queue.empty())
	{
		Element* element = search_queue.front();
		search_queue.pop();

		if (element->GetTagName() == tag)
			elements.push_back(element);

		// Add all children to search.
		for (int i = 0; i < element->GetNumChildren(); i++)
			search_queue.push(element->GetChild(i));
	}
}

// Returns the element's font face.
FontFaceHandle* ElementUtilities::GetFontFaceHandle(Element* element)
{
	// Fetch the new font face.
	String font_family = element->GetProperty(FONT_FAMILY)->value.Get< String >();
	String font_charset = element->GetProperty(FONT_CHARSET)->value.Get< String >();
	Font::Style font_style = (Font::Style) element->GetProperty(FONT_STYLE)->value.Get< int >();
	Font::Weight font_weight = (Font::Weight) element->GetProperty(FONT_WEIGHT)->value.Get< int >();
	int font_size = Math::RealToInteger(element->ResolveProperty(FONT_SIZE, 0));

	FontFaceHandle* font = FontDatabase::GetFontFaceHandle(font_family, font_charset, font_style, font_weight, font_size);
	return font;
}

// Returns an element's font size, if it has a font defined.
int ElementUtilities::GetFontSize(Element* element)
{
	FontFaceHandle* font_face_handle = element->GetFontFaceHandle();
	if (font_face_handle == NULL)
		return 0;
	
	return font_face_handle->GetSize();
}

// Returns an element's line height, if it has a font defined.
int ElementUtilities::GetLineHeight(Element* element)
{
	FontFaceHandle* font_face_handle = element->GetFontFaceHandle();
	if (font_face_handle == NULL)
		return 0;

	int line_height = font_face_handle->GetLineHeight();
	const Property* line_height_property = element->GetProperty(LINE_HEIGHT);

	// If the property is a straight number or an em measurement, then it scales the line height.
	if (line_height_property->unit == Property::NUMBER ||
		line_height_property->unit == Property::EM)
		return Math::Round(line_height_property->value.Get< float >() * line_height);

	// If the property is a percentage, then it scales the line height.
	else if (line_height_property->unit == Property::PERCENT)
		return Math::Round(line_height_property->value.Get< float >() * line_height * 0.01f);

	// Otherwise, we're a px measurement.
	else if (line_height_property->unit == Property::PX)
		return Math::Round(line_height_property->value.Get< float >());

	return 0;
}

// Returns the width of a string rendered within the context of the given element.
int ElementUtilities::GetStringWidth(Element* element, const WString& string)
{
	FontFaceHandle* font_face_handle = element->GetFontFaceHandle();
	if (font_face_handle == NULL)
		return 0;

	return font_face_handle->GetStringWidth(string);
}

void ElementUtilities::BindEventAttributes(Element* element)
{
	int index = 0;
	String name;
	String value;

	// Check for and instance the on* events
	while(element->IterateAttributes(index, name, value))
	{
		if (name.Substring(0, 2) == "on")
		{
			EventListener* listener = Factory::InstanceEventListener(value);
			if (listener)
				element->AddEventListener(&name[2], listener, false);
		}
	}
}

// Generates the clipping region for an element.
bool ElementUtilities::GetClippingRegion(Vector2i& clip_origin, Vector2i& clip_dimensions, Element* element)
{
	ClipRegion clip_region;
//	ClippingRegionStack clipping_regions;

	// Check the clip property of the clipped element; if it is not clipped, then the clip root will be NULL.
	if (element != NULL)
	{
		const Property* clip_property = element->GetProperty(CLIP);

		// Does this element ignore all clipping?
		if (clip_property->unit == Property::KEYWORD &&
			clip_property->value.Get< int >() == CLIP_NONE)
		{
			return false;
		}

		int num_ignored_clips = 0;
		if (clip_property->unit == Property::NUMBER)
		{
			if (clip_property->unit == Property::NUMBER)
				num_ignored_clips = Math::RealToInteger(clip_property->value.Get< float >());
		}

		// Search through the element's ancestors, finding all elements that clip their overflow and have overflow to clip.
		// For each that we find, we combine their clipping region with the existing clipping region, and so build up a
		// complete clipping region for the element.
		Element* clipping_element = element->GetParentNode();

		while (clipping_element != NULL)
		{
			// Merge the existing clip region with the current clip region if we aren't ignoring clip regions.
			if (num_ignored_clips == 0)
				clip_region.AddClipElement(clipping_element);

			// If this region is meant to clip and we're skipping regions, update the counter.
			if (num_ignored_clips > 0)
			{
				if (clipping_element->GetProperty(OVERFLOW_X)->Get< int >() != OVERFLOW_VISIBLE ||
					clipping_element->GetProperty(OVERFLOW_Y)->Get< int >() != OVERFLOW_VISIBLE)
					num_ignored_clips--;
			}

			// Determine how many clip regions this ancestor ignores, and inherit the value. If this region ignores all
			// clipping regions, then we do too.
			int num_ignored_clips = 0;
			const Property* clip_property = clipping_element->GetProperty(CLIP);
			if (clip_property->unit == Property::NUMBER)
				num_ignored_clips = Math::Max(num_ignored_clips, clip_property->Get< int >());
			else if (clip_property->Get< int >() == CLIP_NONE)
				break;

			// Climb the tree to this region's parent.
			clipping_element = clipping_element->GetParentNode();
		}
	}

	if (clip_region.dimensions == Vector2i(-1, -1))
	{
		return false;
	}
	else
	{
		clip_origin = clip_region.origin;
		clip_dimensions = clip_region.dimensions;

		return true;
	}
}

// Sets the clipping region from an element and its ancestors.
bool ElementUtilities::SetClippingRegion(Element* element, Context* context)
{
	Vector2i clip_origin, clip_dimensions;
	bool clip = GetClippingRegion(clip_origin, clip_dimensions, element);

	Rocket::Core::RenderInterface* render_interface;
	if (element != NULL)
		render_interface = element->GetRenderInterface();
	else
	{
		if (context == NULL)
			render_interface = GetRenderInterface();
		else
			render_interface = context->GetRenderInterface();
	}

	if (render_interface == NULL)
		return false;

	ClipState* clip_state = NULL;
	ClipStateMap::iterator clip_iterator = clip_states.find(render_interface);
	if (clip_iterator == clip_states.end())
	{
		clip_state = &(clip_states.insert(ClipStateMap::value_type(render_interface, ClipState())).first->second);
		PushClipCache(render_interface);
	}
	else
		clip_state = &(clip_iterator->second);

	if (clip)
	{
		if (clip_dimensions.x <= 0 ||
			clip_dimensions.y <= 0)
			return false;

		if (!clip_state->clip_on)
		{
			render_interface->EnableScissorRegion(true);
			clip_state->clip_on = true;
		}

		if (clip_origin != clip_state->clip_region.origin ||
			clip_dimensions != clip_state->clip_region.dimensions)
		{
			render_interface->SetScissorRegion(clip_origin.x, clip_origin.y, clip_dimensions.x, clip_dimensions.y);
			clip_state->clip_region.origin = clip_origin;
			clip_state->clip_region.dimensions = clip_dimensions;
		}
	}
	else
	{
		if (clip_state->clip_on)
		{
			render_interface->EnableScissorRegion(false);
			clip_state->clip_on = false;
		}
	}

	return true;
}

void ElementUtilities::PushClipCache(RenderInterface* render_interface)
{
	if (render_interface == NULL)
		return;

	ClipStateMap::iterator clip_iterator = clip_states.find(render_interface);
	if (clip_iterator == clip_states.end())
		return;

	ClipState* clip_state = &(clip_iterator->second);
	render_interface->EnableScissorRegion(clip_state->clip_on);
	if (clip_state->clip_region.origin != Vector2i(-1, -1) &&
		clip_state->clip_region.dimensions != Vector2i(-1, -1))
		render_interface->SetScissorRegion(clip_state->clip_region.origin.x, clip_state->clip_region.origin.y, clip_state->clip_region.dimensions.x, clip_state->clip_region.dimensions.y);
}

// Formats the contents of an element.
bool ElementUtilities::FormatElement(Element* element, const Vector2f& containing_block)
{
	LayoutEngine layout_engine;
	return layout_engine.FormatElement(element, containing_block);
}

// Generates the box for an element.
void ElementUtilities::BuildBox(Box& box, const Vector2f& containing_block, Element* element, bool inline_element)
{
	LayoutEngine::BuildBox(box, containing_block, element, inline_element);
}

// Sizes and positions an element within its parent.
bool ElementUtilities::PositionElement(Element* element, const Vector2f& offset)
{
	Element* parent = element->GetParentNode();
	if (parent == NULL)
		return false;

	SetBox(element);
	SetElementOffset(element, offset);

	return true;
}

// Sizes an element, and positions it within its parent offset from the borders of its content area.
bool ElementUtilities::PositionElement(Element* element, const Vector2f& offset, PositionAnchor anchor)
{
	Element* parent = element->GetParentNode();
	if (parent == NULL)
		return false;

	SetBox(element);

	Vector2f containing_block = element->GetParentNode()->GetBox().GetSize(Box::CONTENT);
	Vector2f element_block = element->GetBox().GetSize(Box::MARGIN);

	Vector2f resolved_offset = offset;

	if (anchor & RIGHT)
		resolved_offset.x = containing_block.x - (element_block.x + offset.x);

	if (anchor & BOTTOM)
		resolved_offset.y = containing_block.y - (element_block.y + offset.y);

	SetElementOffset(element, resolved_offset);

	return true;
}
/*
// Returns true if the element is visible within the current clipping region (if any), false if not.
static bool IsElementVisible(const Element* ROCKET_UNUSED(element))
{
	// Fix this when text elements have their sizes correctly set!
	return true;

	if (clip_root == NULL)
		return true;

	Vector2f element_position = element->GetAbsoluteOffset(Box::BORDER);
	for (int i = 0; i < element->GetNumBoxes(); ++i)
	{
		Vector2f box_position = element_position + element->GetBox(i).GetPosition(Box::MARGIN);
		Vector2f box_size = element->GetBox(i).GetSize(Box::MARGIN);

		// If both the left and right edges of this box are to the left of the clipping region,
		// then this box can't intersect the clipping region.
		if (box_position.x < clipping_region.top_left.x &&
			box_position.x + box_size.x < clipping_region.top_left.x)
			continue;

		// If both the left and right edges of this box are to the right of the clipping region,
		// then this box can't intersect the clipping region.
		if (box_position.x > clipping_region.bottom_right.x &&
			box_position.x + box_size.x > clipping_region.bottom_right.x)
			continue;

		// If both the top and bottom edges of this box are to the top of the clipping region,
		// then this box can't intersect the clipping region.
		if (box_position.y < clipping_region.top_left.y &&
			box_position.y + box_size.y < clipping_region.top_left.y)
			continue;

		// If both the top and bottom edges of this box are to the bottom of the clipping region,
		// then this box can't intersect the clipping region.
		if (box_position.y > clipping_region.bottom_right.y &&
			box_position.y + box_size.y > clipping_region.bottom_right.y)
			continue;

		// We intersect!
		return true;
	}

	return false;
}
*/
// Builds and sets the box for an element.
static void SetBox(Element* element)
{
	Element* parent = element->GetParentNode();
	ROCKET_ASSERT(parent != NULL);

	Vector2f containing_block = parent->GetBox().GetSize();
	containing_block.x -= parent->GetElementScroll()->GetScrollbarSize(ElementScroll::VERTICAL);
	containing_block.y -= parent->GetElementScroll()->GetScrollbarSize(ElementScroll::HORIZONTAL);

	Box box;
	LayoutEngine::BuildBox(box, containing_block, element);
	if (element->GetLocalProperty(HEIGHT) == NULL)
		box.SetContent(Vector2f(box.GetSize().x, containing_block.y));

	element->SetBox(box);
}

// Positions an element relative to an offset parent.
static void SetElementOffset(Element* element, const Vector2f& offset)
{
	Vector2f relative_offset = element->GetParentNode()->GetBox().GetPosition(Box::CONTENT);
	relative_offset += offset;
	relative_offset.x += element->GetBox().GetEdge(Box::MARGIN, Box::LEFT);
	relative_offset.y += element->GetBox().GetEdge(Box::MARGIN, Box::TOP);

	element->SetOffset(relative_offset, element->GetParentNode());
}

}
}
