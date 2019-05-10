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
#include "../../Include/Rocket/Core/Element.h"
#include "../../Include/Rocket/Core/Dictionary.h"
#include "../../Include/Rocket/Core/TransformPrimitive.h"
#include <algorithm>
#include <limits>
#include "Clock.h"
#include "ElementAnimation.h"
#include "ElementBackground.h"
#include "ElementBorder.h"
#include "ElementDefinition.h"
#include "ElementStyle.h"
#include "EventDispatcher.h"
#include "ElementDecoration.h"
#include "FontFaceHandle.h"
#include "LayoutEngine.h"
#include "PluginRegistry.h"
#include "Pool.h"
#include "StyleSheetParser.h"
#include "XMLParseTools.h"
#include "../../Include/Rocket/Core/Core.h"

namespace Rocket {
namespace Core {

/**
	STL function object for sorting elements by z-type (ie, float-types before general types, etc).
	@author Peter Curry
 */
class ElementSortZOrder
{
public:
	bool operator()(const std::pair< Element*, float >& lhs, const std::pair< Element*, float >& rhs) const
	{
		return lhs.second < rhs.second;
	}
};

/**
	STL function object for sorting elements by z-index property.
	@author Peter Curry
 */
class ElementSortZIndex
{
public:
	bool operator()(const Element* lhs, const Element* rhs) const
	{
		// Check the z-index.
		return lhs->GetZIndex() < rhs->GetZIndex();
	}
};


struct ElementMetaChunk;
static Pool< ElementMetaChunk > element_meta_chunk_pool(200, true);


// Meta objects for element collected in a single struct to reduce memory allocations
struct ElementMeta 
{
	ElementMeta(Element* el) : event_dispatcher(el), style(el), background(el), border(el), decoration(el), scroll(el)  {}
	EventDispatcher event_dispatcher;
	ElementStyle style;
	ElementBackground background;
	ElementBorder border;
	ElementDecoration decoration;
	ElementScroll scroll;
	Style::ComputedValues computed_values;

	void* ElementMeta::operator new(size_t size)
	{
		void* memory = element_meta_chunk_pool.AllocateObject();
		return memory;
	}
	void ElementMeta::operator delete(void* chunk)
	{
		element_meta_chunk_pool.DeallocateObject((ElementMetaChunk*)chunk);
	}
}; 

struct alignas(ElementMeta) ElementMetaChunk
{
	ElementMetaChunk() { memset(buffer, 0, size); }
	static constexpr size_t size = sizeof(ElementMeta);
	char buffer[size];
};


/// Constructs a new libRocket element.
Element::Element(const String& tag) : tag(tag), relative_offset_base(0, 0), relative_offset_position(0, 0), absolute_offset(0, 0), scroll_offset(0, 0), content_offset(0, 0), content_box(0, 0), 
transform_state(), transform_state_perspective_dirty(true), transform_state_transform_dirty(true), transform_state_parent_transform_dirty(true), dirty_animation(false)
{
	ROCKET_ASSERT(tag == ToLower(tag));
	parent = NULL;
	focus = NULL;
	instancer = NULL;
	owner_document = NULL;

	offset_fixed = false;
	offset_parent = NULL;
	offset_dirty = true;

	client_area = Box::PADDING;

	num_non_dom_children = 0;

	visible = true;

	z_index = 0;

	local_stacking_context = false;
	local_stacking_context_forced = false;
	stacking_context_dirty = false;

	structure_dirty = false;
	parent_structure_dirty = false;

	all_properties_dirty = true;
	computed_values_are_default = true;
	box_dirty = false;

	font_face_handle = NULL;
	
	clipping_ignore_depth = 0;
	clipping_enabled = false;
	clipping_state_dirty = true;

	element_meta = new ElementMeta(this);

	event_dispatcher = &element_meta->event_dispatcher;
	style = &element_meta->style;
	background = &element_meta->background;
	border = &element_meta->border;
	decoration = &element_meta->decoration;
	scroll = &element_meta->scroll;
}

Element::~Element()
{
	ROCKET_ASSERT(parent == NULL);	

	PluginRegistry::NotifyElementDestroy(this);

	// Remove scrollbar elements before we delete the children!
	scroll->ClearScrollbars();

	while (!children.empty())
	{
		// A simplified version of RemoveChild() for destruction.
		Element* child = children.front();
		child->OnChildRemove(child);

		if (num_non_dom_children > 0)
			num_non_dom_children--;

		deleted_children.push_back(child);
		children.erase(children.begin());
	}

	// Release all deleted children.
	ReleaseElements(deleted_children);

	delete element_meta;

	if (font_face_handle != NULL)
		font_face_handle->RemoveReference();

	if (instancer)
		instancer->RemoveReference();
}

void Element::Update()
{
	ReleaseElements(deleted_children);
	active_children = children;

	OnUpdate();

	UpdateStructure();

	style->UpdateDefinition();
	scroll->Update();

	UpdateAnimation();
	AdvanceAnimations();

	if(all_properties_dirty || dirty_properties.size() > 0)
	{
		using namespace Style;
		const ComputedValues* parent_values = (parent ? &parent->GetComputedValues() : nullptr);
		float ppi = GetRenderInterface()->GetPixelsPerInch();
		float dp_ratio = 1.0f;
		const ComputedValues* document_values = nullptr;
		if (auto doc = GetOwnerDocument())
		{
			document_values = &doc->GetComputedValues();
			if (auto context = doc->GetContext())
				dp_ratio = context->GetDensityIndependentPixelRatio();
		}
		style->ComputeValues(element_meta->computed_values, parent_values, document_values, computed_values_are_default, dp_ratio, ppi);

		computed_values_are_default = false;
	}

	// Right now we are assuming computed values are calculated before OnPropertyChange
	// TODO: Any dirtied properties during the next function call will not have their values computed.
	UpdateDirtyProperties();

	if (box_dirty)
	{
		box_dirty = false;
		OnResize();
	}

	UpdateTransformState();

	for (size_t i = 0; i < active_children.size(); i++)
		active_children[i]->Update();
}

void Element::Render()
{
	// Rebuild our stacking context if necessary.
	if (stacking_context_dirty)
		BuildLocalStackingContext();

	// Apply our transform
	ElementUtilities::ApplyTransform(*this);

	// Render all elements in our local stacking context that have a z-index beneath our local index of 0.
	size_t i = 0;
	for (; i < stacking_context.size() && stacking_context[i]->z_index < 0; ++i)
		stacking_context[i]->Render();

	// Set up the clipping region for this element.
	if (ElementUtilities::SetClippingRegion(this))
	{
		background->RenderBackground();
		border->RenderBorder();
		decoration->RenderDecorators();

		OnRender();
	}

	// Render the rest of the elements in the stacking context.
	for (; i < stacking_context.size(); ++i)
		stacking_context[i]->Render();

	// Unapply our transform
	ElementUtilities::UnapplyTransform(*this);
}

// Clones this element, returning a new, unparented element.
Element* Element::Clone() const
{
	Element* clone = NULL;

	if (instancer != NULL)
	{
		clone = instancer->InstanceElement(NULL, GetTagName(), attributes);
		if (clone != NULL)
			clone->SetInstancer(instancer);
	}
	else
		clone = Factory::InstanceElement(NULL, GetTagName(), GetTagName(), attributes);

	if (clone != NULL)
	{
		String inner_rml;
		GetInnerRML(inner_rml);

		clone->SetInnerRML(inner_rml);
	}

	return clone;
}

// Sets or removes a class on the element.
void Element::SetClass(const String& class_name, bool activate)
{
	style->SetClass(class_name, activate);
}

// Checks if a class is set on the element.
bool Element::IsClassSet(const String& class_name) const
{
	return style->IsClassSet(class_name);
}

// Specifies the entire list of classes for this element. This will replace any others specified.
void Element::SetClassNames(const String& class_names)
{
	SetAttribute("class", class_names);
}

/// Return the active class list
String Element::GetClassNames() const
{
	return style->GetClassNames();
}

// Returns the active style sheet for this element. This may be NULL.
StyleSheet* Element::GetStyleSheet() const
{
	return style->GetStyleSheet();
}

// Returns the element's definition, updating if necessary.
const ElementDefinition* Element::GetDefinition()
{
	return style->GetDefinition();
}

// Fills an String with the full address of this element.
String Element::GetAddress(bool include_pseudo_classes) const
{
	// Add the tag name onto the address.
	String address(tag);

	// Add the ID if we have one.
	if (!id.empty())
	{
		address += "#";
		address += id;
	}

	String classes = style->GetClassNames();
	if (!classes.empty())
	{
		classes = Replace(classes, ".", " ");
		address += ".";
		address += classes;
	}

	if (include_pseudo_classes)
	{
		const PseudoClassList& pseudo_classes = style->GetActivePseudoClasses();		
		for (PseudoClassList::const_iterator i = pseudo_classes.begin(); i != pseudo_classes.end(); ++i)
		{
			address += ":";
			address += (*i);
		}
	}

	if (parent)
	{
		address += " < ";
		return address + parent->GetAddress(true);
	}
	else
		return address;
}

// Sets the position of this element, as a two-dimensional offset from another element.
void Element::SetOffset(const Vector2f& offset, Element* _offset_parent, bool _offset_fixed)
{
	_offset_fixed |= GetPosition() == Style::Position::Fixed;

	// If our offset has definitely changed, or any of our parenting has, then these are set and
	// updated based on our left / right / top / bottom properties.
	if (relative_offset_base != offset ||
		offset_parent != _offset_parent ||
		offset_fixed != _offset_fixed)
	{
		relative_offset_base = offset;
		offset_fixed = _offset_fixed;
		offset_parent = _offset_parent;
		UpdateOffset();
		DirtyOffset();
	}

	// Otherwise, our offset is updated in case left / right / top / bottom will have an impact on
	// our final position, and our children are dirtied if they do.
	else
	{
		Vector2f& old_base = relative_offset_base;
		Vector2f& old_position = relative_offset_position;

		UpdateOffset();

		if (old_base != relative_offset_base ||
			old_position != relative_offset_position)
			DirtyOffset();
	}
}

// Returns the position of the top-left corner of one of the areas of this element's primary box.
Vector2f Element::GetRelativeOffset(Box::Area area)
{
	return relative_offset_base + relative_offset_position + GetBox().GetPosition(area);
}

// Returns the position of the top-left corner of one of the areas of this element's primary box.
Vector2f Element::GetAbsoluteOffset(Box::Area area)
{
	if (offset_dirty)
	{
		offset_dirty = false;

		if (offset_parent != NULL)
			absolute_offset = offset_parent->GetAbsoluteOffset(Box::BORDER) + relative_offset_base + relative_offset_position;
		else
			absolute_offset = relative_offset_base + relative_offset_position;

		// Add any parent scrolling onto our position as well. Could cache this if required.
		if (!offset_fixed)
		{
			Element* scroll_parent = parent;
			while (scroll_parent != NULL)
			{
				absolute_offset -= (scroll_parent->scroll_offset + scroll_parent->content_offset);
				if (scroll_parent == offset_parent)
					break;
				else
					scroll_parent = scroll_parent->parent;
			}
		}
	}

	return absolute_offset + GetBox().GetPosition(area);
}

// Sets an alternate area to use as the client area.
void Element::SetClientArea(Box::Area _client_area)
{
	client_area = _client_area;
}

// Returns the area the element uses as its client area.
Box::Area Element::GetClientArea() const
{
	return client_area;
}

// Sets the dimensions of the element's internal content.
void Element::SetContentBox(const Vector2f& _content_offset, const Vector2f& _content_box)
{
	if (content_offset != _content_offset ||
		content_box != _content_box)
	{
		// Seems to be jittering a wee bit; might need to be looked at.
		scroll_offset.x += (content_offset.x - _content_offset.x);
		scroll_offset.y += (content_offset.y - _content_offset.y);

		content_offset = _content_offset;
		content_box = _content_box;

		scroll_offset.x = Math::Min(scroll_offset.x, GetScrollWidth() - GetClientWidth());
		scroll_offset.y = Math::Min(scroll_offset.y, GetScrollHeight() - GetClientHeight());
		DirtyOffset();
	}
}

// Sets the box describing the size of the element.
void Element::SetBox(const Box& box)
{
	if (box != main_box || additional_boxes.size() > 0)
	{
		main_box = box;
		additional_boxes.clear();

		box_dirty = true;
		background->DirtyBackground();
		border->DirtyBorder();
		decoration->DirtyDecorators(true);
	}
}

// Adds a box to the end of the list describing this element's geometry.
void Element::AddBox(const Box& box)
{
	additional_boxes.push_back(box);

	box_dirty = true;
	background->DirtyBackground();
	border->DirtyBorder();
	decoration->DirtyDecorators(true);
}

// Returns one of the boxes describing the size of the element.
const Box& Element::GetBox()
{
	return main_box;
}

// Returns one of the boxes describing the size of the element.
const Box& Element::GetBox(int index)
{
	if (index < 1)
		return main_box;
	
	int additional_box_index = index - 1;
	if (additional_box_index >= (int)additional_boxes.size())
		return main_box;

	return additional_boxes[additional_box_index];
}

// Returns the number of boxes making up this element's geometry.
int Element::GetNumBoxes()
{
	return 1 + (int)additional_boxes.size();
}

// Returns the baseline of the element, in pixels offset from the bottom of the element's content area.
float Element::GetBaseline() const
{
	return 0;
}

// Gets the intrinsic dimensions of this element, if it is of a type that has an inherent size.
bool Element::GetIntrinsicDimensions(Vector2f& ROCKET_UNUSED_PARAMETER(dimensions))
{
	ROCKET_UNUSED(dimensions);

	return false;
}

// Checks if a given point in screen coordinates lies within the bordered area of this element.
bool Element::IsPointWithinElement(const Vector2f& point)
{
	Vector2f position = GetAbsoluteOffset(Box::BORDER);

	for (int i = 0; i < GetNumBoxes(); ++i)
	{
		const Box& box = GetBox(i);

		Vector2f box_position = position + box.GetOffset();
		Vector2f box_dimensions = box.GetSize(Box::BORDER);
		if (point.x >= box_position.x &&
			point.x <= (box_position.x + box_dimensions.x) &&
			point.y >= box_position.y &&
			point.y <= (box_position.y + box_dimensions.y))
		{
			return true;
		}
	}

	return false;
}

// Returns the visibility of the element.
bool Element::IsVisible() const
{
	return visible;
}

// Returns the z-index of the element.
float Element::GetZIndex() const
{
	return z_index;
}

// Returns the element's font face handle.
FontFaceHandle* Element::GetFontFaceHandle() const
{
	return font_face_handle;
}

// Sets a local property override on the element.
bool Element::SetProperty(const String& name, const String& value)
{
	return style->SetProperty(name, value);
}

// Removes a local property override on the element.
void Element::RemoveProperty(const String& name)
{
	style->RemoveProperty(name);
}

// Sets a local property override on the element to a pre-parsed value.
bool Element::SetProperty(const String& name, const Property& property)
{
	return style->SetProperty(name, property);
}

// Returns one of this element's properties.
const Property* Element::GetProperty(const String& name)
{
	return style->GetProperty(name);	
}

// Returns one of this element's properties.
const Property* Element::GetLocalProperty(const String& name)
{
	return style->GetLocalProperty(name);
}

const PropertyMap * Element::GetLocalProperties()
{
	return style->GetLocalProperties();
}

// Resolves one of this element's style.
float Element::ResolveProperty(const String& name, float base_value)
{
	return style->ResolveProperty(name, base_value);
}

// Resolves one of this element's style.
float Element::ResolveProperty(const Property *property, float base_value)
{
	return style->ResolveProperty(property, base_value);
}

Vector2f Element::GetContainingBlock()
{
	Vector2f containing_block(0, 0);

	if (offset_parent != NULL)
	{
		using namespace Style;
		Position position_property = GetPosition();
		const Box& parent_box = offset_parent->GetBox();

		if (position_property == Position::Static || position_property == Position::Relative)
		{
			containing_block = parent_box.GetSize();
		}
		else if(position_property == Position::Absolute || position_property == Position::Fixed)
		{
			containing_block = parent_box.GetSize(Box::PADDING);
		}
	}

	return containing_block;
}

Style::Position Element::GetPosition()
{
	return element_meta->computed_values.position;
}

Style::Float Element::GetFloat()
{
	return element_meta->computed_values.float_;
}

Style::Display Element::GetDisplay()
{
	return element_meta->computed_values.display;
}

float Element::GetLineHeight()
{
	return element_meta->computed_values.line_height.value;
}

// Returns this element's TransformState
const TransformState *Element::GetTransformState() const noexcept
{
	return transform_state.get();
}

// Returns the TransformStates that are effective for this element.
void Element::GetEffectiveTransformState(
	const TransformState **local_perspective,
	const TransformState **perspective,
	const TransformState **transform
) noexcept
{
	UpdateTransformState();

	if (local_perspective)
	{
		*local_perspective = 0;
	}
	if (perspective)
	{
		*perspective = 0;
	}
	if (transform)
	{
		*transform = 0;
	}

	Element *perspective_node = 0, *transform_node = 0;

	// Find the TransformState to use for unprojecting.
	if (transform_state.get() && transform_state->GetLocalPerspective(0))
	{
		if (local_perspective)
		{
			*local_perspective = transform_state.get();
		}
	}
	else
	{
		Element *node = 0;
		for (node = parent; node; node = node->parent)
		{
			if (node->transform_state.get() && node->transform_state->GetPerspective(0))
			{
				if (perspective)
				{
					*perspective = node->transform_state.get();
				}
				perspective_node = node;
				break;
			}
		}
	}

	// Find the TransformState to use for transforming.
	Element *node = 0;
	for (node = this; node; node = node->parent)
	{
		if (node->transform_state.get() && node->transform_state->GetRecursiveTransform(0))
		{
			if (transform)
			{
				*transform = node->transform_state.get();
			}
			transform_node = node;
			break;
		}
	}
}

// Project a 2D point in pixel coordinates onto the element's plane.
const Vector2f Element::Project(const Vector2f& point) noexcept
{
	UpdateTransformState();

	Context *context = GetContext();
	if (!context)
	{
		return point;
	}

	const TransformState *local_perspective, *perspective, *transform;
	GetEffectiveTransformState(&local_perspective, &perspective, &transform);

	Vector2i view_pos(0, 0);
	Vector2i view_size = context->GetDimensions();

	// Compute the line segment for ray picking, one point on the near and one on the far plane.
	// These need to be in clip space coordinates ([-1; 1]³) so that we an unproject them.
	Vector3f line_segment[2] =
	{
		// When unprojected, the intersection point on the near plane
		Vector3f(
			(point.x - view_pos.x) / (0.5f * view_size.x) - 1.0f,
			(view_size.y - point.y - view_pos.y) / (0.5f * view_size.y) - 1.0f,
			-1
		),
		// When unprojected, the intersection point on the far plane
		Vector3f(
			(point.x - view_pos.x) / (0.5f * view_size.x) - 1.0f,
			(view_size.y - point.y - view_pos.y) / (0.5f * view_size.y) - 1.0f,
			1
		)
	};

	// Find the TransformState to use for unprojecting.
	if (local_perspective)
	{
		TransformState::LocalPerspective the_local_perspective;
		local_perspective->GetLocalPerspective(&the_local_perspective);
		line_segment[0] = the_local_perspective.Unproject(line_segment[0]);
		line_segment[1] = the_local_perspective.Unproject(line_segment[1]);
	}
	else if (perspective)
	{
		TransformState::Perspective the_perspective;
		perspective->GetPerspective(&the_perspective);
		line_segment[0] = the_perspective.Unproject(line_segment[0]);
		line_segment[1] = the_perspective.Unproject(line_segment[1]);
	}
	else
	{
		line_segment[0] = context->GetViewState().Unproject(line_segment[0]);
		line_segment[1] = context->GetViewState().Unproject(line_segment[1]);
	}

	// Compute three points on the context's corners to define the element's plane.
	// It may seem elegant to base this computation on the element's size, but
	// there are elements with zero length or height.
	Vector3f element_rect[3] =
	{
		// Top-left corner
		Vector3f(0, 0, 0),
		// Top-right corner
		Vector3f((float)view_size.x, 0, 0),
		// Bottom-left corner
		Vector3f(0, (float)view_size.y, 0)
	};
	// Transform by the correct matrix
	if (transform)
	{
		element_rect[0] = transform->Transform(element_rect[0]);
		element_rect[1] = transform->Transform(element_rect[1]);
		element_rect[2] = transform->Transform(element_rect[2]);
	}

	Vector3f u = line_segment[0] - line_segment[1];
	Vector3f v = element_rect[1] - element_rect[0];
	Vector3f w = element_rect[2] - element_rect[0];

	// Now compute the intersection point of the line segment and the element's rectangle.
	// This is based on the algorithm discussed at Wikipedia
	// (http://en.wikipedia.org/wiki/Line-plane_intersection).
	Matrix4f A = Matrix4f::FromColumns(
		Vector4f(u, 0),
		Vector4f(v, 0),
		Vector4f(w, 0),
		Vector4f(0, 0, 0, 1)
	);
	if (A.Invert())
	{
		Vector3f factors = A * (line_segment[0] - element_rect[0]);
		Vector3f intersection3d = element_rect[0] + v * factors[1] + w * factors[2];
		Vector3f projected;
		if (transform)
		{
			projected = transform->Untransform(intersection3d);
			//ROCKET_ASSERT(fabs(projected.z) < 0.0001);
		}
		else
		{
			// FIXME: Is this correct?
			projected = intersection3d;
		}
		return Vector2f(projected.x, projected.y);
	}
	else
	{
		// The line segment is parallel to the element's plane.
		// Although, mathematically, it could also lie within the plane
		// (yielding infinitely many intersection points), we still
		// return a value that's pretty sure to not match anything,
		// since this case has nothing to do with the user `picking'
		// anything.
		float inf = std::numeric_limits< float >::infinity();
		return Vector2f(-inf, -inf);
	}
}


// Iterates over the properties defined on this element.
bool Element::IterateProperties(int& index, String& name, const Property*& property, const PseudoClassList** pseudo_classes) const
{
	return style->IterateProperties(index, name, property, pseudo_classes);
}

// Sets or removes a pseudo-class on the element.
void Element::SetPseudoClass(const String& pseudo_class, bool activate)
{
	style->SetPseudoClass(pseudo_class, activate);
}

// Checks if a specific pseudo-class has been set on the element.
bool Element::IsPseudoClassSet(const String& pseudo_class) const
{
	return style->IsPseudoClassSet(pseudo_class);
}

// Checks if a complete set of pseudo-classes are set on the element.
bool Element::ArePseudoClassesSet(const PseudoClassList& pseudo_classes) const
{
	for (PseudoClassList::const_iterator i = pseudo_classes.begin(); i != pseudo_classes.end(); ++i)
	{
		if (!IsPseudoClassSet(*i))
			return false;
	}

	return true;
}

// Gets a list of the current active pseudo classes
const PseudoClassList& Element::GetActivePseudoClasses() const
{
	return style->GetActivePseudoClasses();
}

/// Get the named attribute
Variant* Element::GetAttribute(const String& name)
{
	return GetIf(attributes, name);
}

// Checks if the element has a certain attribute.
bool Element::HasAttribute(const String& name) const
{
	return attributes.find(name) != attributes.end();
}

// Removes an attribute from the element
void Element::RemoveAttribute(const String& name)
{
	if (auto it = attributes.find(name); it != attributes.end())
	{
		attributes.erase(it);

		AttributeNameList changed_attributes;
		changed_attributes.insert(name);

		OnAttributeChange(changed_attributes);
	}
}

// Gets the outer most focus element down the tree from this node
Element* Element::GetFocusLeafNode()
{
	// If there isn't a focus, then we are the leaf.
	if (!focus)
	{
		return this;
	}

	// Recurse down the tree until we found the leaf focus element
	Element* focus_element = focus;
	while (focus_element->focus)
		focus_element = focus_element->focus;

	return focus_element;
}

// Returns the element's context.
Context* Element::GetContext()
{
	ElementDocument* document = GetOwnerDocument();
	if (document != NULL)
		return document->GetContext();

	return NULL;
}

// Set a group of attributes
void Element::SetAttributes(const ElementAttributes* _attributes)
{
	attributes.reserve(attributes.size() + _attributes->size());
	for (auto& [key, value] : *_attributes)
		attributes[key] = value;

	AttributeNameList changed_attributes;
	changed_attributes.reserve(_attributes->size());
	for (auto& [key, value] : *_attributes)
		changed_attributes.insert(key);
	OnAttributeChange(changed_attributes);
}

// Returns the number of attributes on the element.
int Element::GetNumAttributes() const
{
	return (int)attributes.size();
}

// Iterates over all decorators attached to the element.
bool Element::IterateDecorators(int& index, PseudoClassList& pseudo_classes, String& name, Decorator*& decorator, DecoratorDataHandle& decorator_data)
{
	return decoration->IterateDecorators(index, pseudo_classes, name, decorator, decorator_data);
}

// Gets the name of the element.
const String& Element::GetTagName() const
{
	return tag;
}

// Gets the ID of the element.
const String& Element::GetId() const
{
	return id;
}

// Sets the ID of the element.
void Element::SetId(const String& _id)
{
	SetAttribute("id", _id);
}

// Gets the horizontal offset from the context's left edge to element's left border edge.
float Element::GetAbsoluteLeft()
{
	return GetAbsoluteOffset(Box::BORDER).x;
}

// Gets the vertical offset from the context's top edge to element's top border edge.
float Element::GetAbsoluteTop()
{
	return GetAbsoluteOffset(Box::BORDER).y;
}

// Gets the width of the left border of an element.
float Element::GetClientLeft()
{
	return GetBox().GetPosition(client_area).x;
}

// Gets the height of the top border of an element.
float Element::GetClientTop()
{
	return GetBox().GetPosition(client_area).y;
}

// Gets the inner width of the element.
float Element::GetClientWidth()
{
	return GetBox().GetSize(client_area).x - scroll->GetScrollbarSize(ElementScroll::VERTICAL);
}

// Gets the inner height of the element.
float Element::GetClientHeight()
{
	return GetBox().GetSize(client_area).y - scroll->GetScrollbarSize(ElementScroll::HORIZONTAL);
}

// Returns the element from which all offset calculations are currently computed.
Element* Element::GetOffsetParent()
{
	return offset_parent;
}

// Gets the distance from this element's left border to its offset parent's left border.
float Element::GetOffsetLeft()
{
	return relative_offset_base.x + relative_offset_position.x;
}

// Gets the distance from this element's top border to its offset parent's top border.
float Element::GetOffsetTop()
{
	return relative_offset_base.y + relative_offset_position.y;
}

// Gets the width of the element, including the client area, padding, borders and scrollbars, but not margins.
float Element::GetOffsetWidth()
{
	return GetBox().GetSize(Box::BORDER).x;
}

// Gets the height of the element, including the client area, padding, borders and scrollbars, but not margins.
float Element::GetOffsetHeight()
{
	return GetBox().GetSize(Box::BORDER).y;
}

// Gets the left scroll offset of the element.
float Element::GetScrollLeft()
{
	return scroll_offset.x;
}

// Sets the left scroll offset of the element.
void Element::SetScrollLeft(float scroll_left)
{
	scroll_offset.x = Math::Clamp(scroll_left, 0.0f, GetScrollWidth() - GetClientWidth());
	scroll->UpdateScrollbar(ElementScroll::HORIZONTAL);
	DirtyOffset();

	DispatchEvent("scroll", Dictionary());
}

// Gets the top scroll offset of the element.
float Element::GetScrollTop()
{
	return scroll_offset.y;
}

// Sets the top scroll offset of the element.
void Element::SetScrollTop(float scroll_top)
{
	scroll_offset.y = Math::Clamp(scroll_top, 0.0f, GetScrollHeight() - GetClientHeight());
	scroll->UpdateScrollbar(ElementScroll::VERTICAL);
	DirtyOffset();

	DispatchEvent("scroll", Dictionary());
}

// Gets the width of the scrollable content of the element; it includes the element padding but not its margin.
float Element::GetScrollWidth()
{
	return Math::Max(content_box.x, GetClientWidth());
}

// Gets the height of the scrollable content of the element; it includes the element padding but not its margin.
float Element::GetScrollHeight()
{
	return Math::Max(content_box.y, GetClientHeight());
}

// Gets the object representing the declarations of an element's style attributes.
ElementStyle* Element::GetStyle()
{
	return style;
}

// Gets the document this element belongs to.
ElementDocument* Element::GetOwnerDocument()
{
	if (parent == NULL)
		return NULL;
	
	if (!owner_document)
	{
		owner_document = parent->GetOwnerDocument();
	}

	return owner_document;
}

// Gets this element's parent node.
Element* Element::GetParentNode() const
{
	return parent;
}

// Gets the element immediately following this one in the tree.
Element* Element::GetNextSibling() const
{
	if (parent == NULL)
		return NULL;

	for (size_t i = 0; i < parent->children.size() - (parent->num_non_dom_children + 1); i++)
	{
		if (parent->children[i] == this)
			return parent->children[i + 1];
	}

	return NULL;
}

// Gets the element immediately preceding this one in the tree.
Element* Element::GetPreviousSibling() const
{
	if (parent == NULL)
		return NULL;

	for (size_t i = 1; i < parent->children.size() - parent->num_non_dom_children; i++)
	{
		if (parent->children[i] == this)
			return parent->children[i - 1];
	}

	return NULL;
}

// Returns the first child of this element.
Element* Element::GetFirstChild() const
{
	if (GetNumChildren() > 0)
		return children[0];

	return NULL;
}

// Gets the last child of this element.
Element* Element::GetLastChild() const
{
	if (GetNumChildren() > 0)
		return *(children.end() - (num_non_dom_children + 1));

	return NULL;
}

Element* Element::GetChild(int index) const
{
	if (index < 0 || index >= (int) children.size())
		return NULL;

	return children[index];
}

int Element::GetNumChildren(bool include_non_dom_elements) const
{
	return (int) children.size() - (include_non_dom_elements ? 0 : num_non_dom_children);
}

// Gets the markup and content of the element.
void Element::GetInnerRML(String& content) const
{
	for (int i = 0; i < GetNumChildren(); i++)
	{
		children[i]->GetRML(content);
	}
}

// Gets the markup and content of the element.
String Element::GetInnerRML() const {
	String result;
	GetInnerRML(result);
	return result;
}

// Sets the markup and content of the element. All existing children will be replaced.
void Element::SetInnerRML(const String& rml)
{
	// Remove all DOM children.
	while ((int) children.size() > num_non_dom_children)
		RemoveChild(children.front());

	Factory::InstanceElementText(this, rml);
}

// Sets the current element as the focus object.
bool Element::Focus()
{
	// Are we allowed focus?
	Style::Focus focus_property = element_meta->computed_values.focus;
	if (focus_property == Style::Focus::None)
		return false;

	// Ask our context if we can switch focus.
	Context* context = GetContext();
	if (context == NULL)
		return false;

	if (!context->OnFocusChange(this))
		return false;

	// Set this as the end of the focus chain.
	focus = NULL;

	// Update the focus chain up the hierarchy.
	Element* element = this;
	while (element->GetParentNode())
	{
		element->GetParentNode()->focus = element;
		element = element->GetParentNode();
	}

	return true;
}

// Removes focus from from this element.
void Element::Blur()
{
	if (parent)
	{
		Context* context = GetContext();
		if (context == NULL)
			return;

		if (context->GetFocusElement() == this)
		{
			parent->Focus();
		}
		else if (parent->focus == this)
		{
			parent->focus = NULL;
		}
	}
}

// Fakes a mouse click on this element.
void Element::Click()
{
	Context* context = GetContext();
	if (context == NULL)
		return;

	context->GenerateClickEvent(this);
}

// Adds an event listener
void Element::AddEventListener(const String& event, EventListener* listener, bool in_capture_phase)
{
	event_dispatcher->AttachEvent(event, listener, in_capture_phase);
}

// Removes an event listener from this element.
void Element::RemoveEventListener(const String& event, EventListener* listener, bool in_capture_phase)
{
	event_dispatcher->DetachEvent(event, listener, in_capture_phase);
}

// Dispatches the specified event
bool Element::DispatchEvent(const String& event, const Dictionary& parameters, bool interruptible)
{
	return event_dispatcher->DispatchEvent(this, event, parameters, interruptible);
}

// Scrolls the parent element's contents so that this element is visible.
void Element::ScrollIntoView(bool align_with_top)
{
	Vector2f size(0, 0);
	if (!align_with_top)
	{
		size.y = main_box.GetOffset().y +
			main_box.GetSize(Box::BORDER).y;
	}

	Element* scroll_parent = parent;
	while (scroll_parent != NULL)
	{
		Style::Overflow overflow_x_property = scroll_parent->GetComputedValues().overflow_x;
		Style::Overflow overflow_y_property = scroll_parent->GetComputedValues().overflow_y;

		if ((overflow_x_property != Style::Overflow::Visible &&
			 scroll_parent->GetScrollWidth() > scroll_parent->GetClientWidth()) ||
			(overflow_y_property != Style::Overflow::Visible &&
			 scroll_parent->GetScrollHeight() > scroll_parent->GetClientHeight()))
		{
			Vector2f offset = scroll_parent->GetAbsoluteOffset(Box::BORDER) - GetAbsoluteOffset(Box::BORDER);
			Vector2f scroll_offset(scroll_parent->GetScrollLeft(), scroll_parent->GetScrollTop());
			scroll_offset -= offset;
			scroll_offset.x += scroll_parent->GetClientLeft();
			scroll_offset.y += scroll_parent->GetClientTop();

			if (!align_with_top)
				scroll_offset.y -= (scroll_parent->GetClientHeight() - size.y);

			if (overflow_x_property != Style::Overflow::Visible)
				scroll_parent->SetScrollLeft(scroll_offset.x);
			if (overflow_y_property != Style::Overflow::Visible)
				scroll_parent->SetScrollTop(scroll_offset.y);
		}

		scroll_parent = scroll_parent->GetParentNode();
	}
}

// Appends a child to this element
void Element::AppendChild(Element* child, bool dom_element)
{
	LockLayout(true);

	child->AddReference();
	child->SetParent(this);
	if (dom_element)
		children.insert(children.end() - num_non_dom_children, child);
	else
	{
		children.push_back(child);
		num_non_dom_children++;
	}

	child->GetStyle()->DirtyDefinition();
	all_properties_dirty = true;

	child->OnChildAdd(child);
	DirtyStackingContext();
	DirtyStructure();

	if (dom_element)
		DirtyLayout();

	LockLayout(false);
}

// Adds a child to this element, directly after the adjacent element. Inherits
// the dom/non-dom status from the adjacent element.
void Element::InsertBefore(Element* child, Element* adjacent_element)
{
	// Find the position in the list of children of the adjacent element. If
	// it's NULL or we can't find it, then we insert it at the end of the dom
	// children, as a dom element.
	size_t child_index = 0;
	bool found_child = false;
	if (adjacent_element)
	{
		for (child_index = 0; child_index < children.size(); child_index++)
		{
			if (children[child_index] == adjacent_element)
			{
				found_child = true;
				break;
			}
		}
	}

	if (found_child)
	{
		LockLayout(true);

		child->AddReference();
		child->SetParent(this);

		if ((int) child_index >= GetNumChildren())
			num_non_dom_children++;
		else
			DirtyLayout();

		children.insert(children.begin() + child_index, child);

		child->GetStyle()->DirtyDefinition();
		all_properties_dirty = true;

		child->OnChildAdd(child);
		DirtyStackingContext();
		DirtyStructure();

		LockLayout(false);
	}
	else
	{
		AppendChild(child);
	}	
}

// Replaces the second node with the first node.
bool Element::ReplaceChild(Element* inserted_element, Element* replaced_element)
{
	inserted_element->AddReference();
	inserted_element->SetParent(this);

	ElementList::iterator insertion_point = children.begin();
	while (insertion_point != children.end() && *insertion_point != replaced_element)
	{
		++insertion_point;
	}

	if (insertion_point == children.end())
	{
		AppendChild(inserted_element);
		return false;
	}

	LockLayout(true);

	children.insert(insertion_point, inserted_element);
	RemoveChild(replaced_element);

	inserted_element->GetStyle()->DirtyDefinition();
	all_properties_dirty = true;
	inserted_element->OnChildAdd(inserted_element);

	LockLayout(false);

	return true;
}

// Removes the specified child
bool Element::RemoveChild(Element* child)
{
	size_t child_index = 0;

	for (ElementList::iterator itr = children.begin(); itr != children.end(); ++itr)
	{
		// Add the element to the delete list
		if ((*itr) == child)
		{
			LockLayout(true);

			// Inform the context of the element's pending removal (if we have a valid context).
			Context* context = GetContext();
			if (context)
				context->OnElementRemove(child);

			child->OnChildRemove(child);

			if (child_index >= children.size() - num_non_dom_children)
				num_non_dom_children--;

			deleted_children.push_back(child);
			children.erase(itr);

			// Remove the child element as the focussed child of this element.
			if (child == focus)
			{
				focus = NULL;

				// If this child (or a descendant of this child) is the context's currently
				// focussed element, set the focus to us instead.
				Context* context = GetContext();
				if (context != NULL)
				{
					Element* focus_element = context->GetFocusElement();
					while (focus_element != NULL)
					{
						if (focus_element == child)
						{
							Focus();
							break;
						}

						focus_element = focus_element->GetParentNode();
					}
				}
			}

			DirtyLayout();
			DirtyStackingContext();
			DirtyStructure();

			LockLayout(false);

			return true;
		}

		child_index++;
	}

	return false;
}

bool Element::HasChildNodes() const
{
	return (int) children.size() > num_non_dom_children;
}

Element* Element::GetElementById(const String& id)
{
	// Check for special-case tokens.
	if (id == "#self")
		return this;
	else if (id == "#document")
		return GetOwnerDocument();
	else if (id == "#parent")
		return this->parent;
	else
	{
		Element* search_root = GetOwnerDocument();
		if (search_root == NULL)
			search_root = this;
		return ElementUtilities::GetElementById(search_root, id);
	}
}

// Get all elements with the given tag.
void Element::GetElementsByTagName(ElementList& elements, const String& tag)
{
	return ElementUtilities::GetElementsByTagName(elements, this, tag);
}

// Get all elements with the given class set on them.
void Element::GetElementsByClassName(ElementList& elements, const String& class_name)
{
	return ElementUtilities::GetElementsByClassName(elements, this, class_name);
}

// Access the event dispatcher
EventDispatcher* Element::GetEventDispatcher() const
{
	return event_dispatcher;
}

String Element::GetEventDispatcherSummary() const
{
	return event_dispatcher->ToString();
}

// Access the element background.
ElementBackground* Element::GetElementBackground() const
{
	return background;
}

// Access the element border.
ElementBorder* Element::GetElementBorder() const
{
	return border;
}

// Access the element decorators
ElementDecoration* Element::GetElementDecoration() const
{
	return decoration;
}

// Returns the element's scrollbar functionality.
ElementScroll* Element::GetElementScroll() const
{
	return scroll;
}
	
int Element::GetClippingIgnoreDepth()
{
	if (clipping_state_dirty)
	{
		IsClippingEnabled();
	}
	
	return clipping_ignore_depth;
}
	
bool Element::IsClippingEnabled()
{
	if (clipping_state_dirty)
	{
		const auto& computed = GetComputedValues();

		// Is clipping enabled for this element, yes unless both overlow properties are set to visible
		clipping_enabled = computed.overflow_x != Style::Overflow::Visible
							|| computed.overflow_y != Style::Overflow::Visible;
		
		// Get the clipping ignore depth from the clip property
		clipping_ignore_depth = computed.clip.number;

		clipping_state_dirty = false;
	}
	
	return clipping_enabled;
}

// Gets the render interface owned by this element's context.
RenderInterface* Element::GetRenderInterface()
{
	Context* context = GetContext();
	if (context != NULL)
		return context->GetRenderInterface();

	return Rocket::Core::GetRenderInterface();
}

void Element::SetInstancer(ElementInstancer* _instancer)
{
	// Only record the first instancer being set as some instancers call other instancers to do their dirty work, in
	// which case we don't want to update the lowest level instancer.
	if (instancer == NULL)
	{
		instancer = _instancer;
		instancer->AddReference();
	}
}

// Forces the element to generate a local stacking context, regardless of the value of its z-index property.
void Element::ForceLocalStackingContext()
{
	local_stacking_context_forced = true;
	local_stacking_context = true;

	DirtyStackingContext();
}

// Called during the update loop after children are rendered.
void Element::OnUpdate()
{
}

// Called during render after backgrounds, borders, decorators, but before children, are rendered.
void Element::OnRender()
{
}

void Element::OnResize()
{
}

// Called during a layout operation, when the element is being positioned and sized.
void Element::OnLayout()
{
}

// Called when attributes on the element are changed.
void Element::OnAttributeChange(const AttributeNameList& changed_attributes)
{
	if (changed_attributes.find("id") != changed_attributes.end())
	{
		id = GetAttribute< String >("id", "");
		style->DirtyDefinition();
	}

	if (changed_attributes.find("class") != changed_attributes.end())
	{
		style->SetClassNames(GetAttribute< String >("class", ""));
	}

	// Add any inline style declarations.
	if (changed_attributes.find("style") != changed_attributes.end())
	{
		PropertyDictionary properties;
		StyleSheetParser parser;
		parser.ParseProperties(properties, GetAttribute< String >("style", ""));

		Rocket::Core::PropertyMap property_map = properties.GetProperties();
		for (Rocket::Core::PropertyMap::iterator i = property_map.begin(); i != property_map.end(); ++i)
		{
			SetProperty((*i).first, (*i).second);
		}
	}
}

// Called when properties on the element are changed.
void Element::OnPropertyChange(const PropertyNameList& changed_properties)
{
	bool all_dirty = false;
	{
		auto& registered_properties = StyleSheetSpecification::GetRegisteredProperties();
		if (&registered_properties == &changed_properties || registered_properties == changed_properties)
			all_dirty = true;
	}

	if (!IsLayoutDirty())
	{
		if (all_dirty)
		{
			DirtyLayout();
		}
		else
		{
			// Force a relayout if any of the changed properties require it.
			for (PropertyNameList::const_iterator i = changed_properties.begin(); i != changed_properties.end(); ++i)
			{
				const PropertyDefinition* property_definition = StyleSheetSpecification::GetProperty(*i);
				if (property_definition)
				{
					if (property_definition->IsLayoutForced())
					{
						DirtyLayout();
						break;
					}
				}
			}
		}
	}

	// Update the visibility.
	if (all_dirty || changed_properties.find(VISIBILITY) != changed_properties.end() ||
		changed_properties.find(DISPLAY) != changed_properties.end())
	{
		bool new_visibility = (element_meta->computed_values.display != Style::Display::None && element_meta->computed_values.visibility == Style::Visibility::Visible);
			
		if (visible != new_visibility)
		{
			visible = new_visibility;

			if (parent != NULL)
				parent->DirtyStackingContext();
		}

		if (all_dirty || 
			changed_properties.find(DISPLAY) != changed_properties.end())
		{
			if (parent != NULL)
				parent->DirtyStructure();
		}
	}

	// Fetch a new font face if it has been changed.
	if (all_dirty ||
		changed_properties.find(FONT_FAMILY) != changed_properties.end() ||
		changed_properties.find(FONT_CHARSET) != changed_properties.end() ||
		changed_properties.find(FONT_WEIGHT) != changed_properties.end() ||
		changed_properties.find(FONT_STYLE) != changed_properties.end() ||
		changed_properties.find(FONT_SIZE) != changed_properties.end())
	{
		// Store the old em; if it changes, then we need to dirty all em-relative properties.
		int old_em = -1;
		if (font_face_handle != NULL)
			old_em = font_face_handle->GetLineHeight();

		// Fetch the new font face.
		FontFaceHandle * new_font_face_handle = ElementUtilities::GetFontFaceHandle(element_meta->computed_values);

		// If this is different from our current font face, then we've got to nuke
		// all our characters and tell our parent that we have to be re-laid out.
		if (new_font_face_handle != font_face_handle)
		{
			if (font_face_handle)
				font_face_handle->RemoveReference();

			font_face_handle = new_font_face_handle;

			// Our font face has changed; odds are, so has our em. All of our em-relative values
			// have therefore probably changed as well, so we'll need to dirty them.
			int new_em = -1;
			if (font_face_handle != NULL)
				new_em = font_face_handle->GetLineHeight();

			// However, if all properties are dirty, we don't need to perform this expensive
			// step as all properties are dirtied below anyway.
			if (old_em != new_em && !all_dirty)
			{
				style->DirtyEmProperties();
			}
		}
		else if (new_font_face_handle != NULL)
			new_font_face_handle->RemoveReference();
	}


	// Update the position.
	if (all_dirty ||
		changed_properties.find(LEFT) != changed_properties.end() ||
		changed_properties.find(RIGHT) != changed_properties.end() ||
		changed_properties.find(TOP) != changed_properties.end() ||
		changed_properties.find(BOTTOM) != changed_properties.end())
	{
		UpdateOffset();
		DirtyOffset();
	}

	// Update the z-index.
	if (all_dirty || 
		changed_properties.find(Z_INDEX) != changed_properties.end())
	{
		Style::ZIndex z_index_property = element_meta->computed_values.z_index;

		if (z_index_property.type == Style::ZIndex::Auto)
		{
			if (local_stacking_context &&
				!local_stacking_context_forced)
			{
				// We're no longer acting as a stacking context.
				local_stacking_context = false;

				stacking_context_dirty = false;
				stacking_context.clear();
			}

			// If our old z-index was not zero, then we must dirty our stacking context so we'll be re-indexed.
			if (z_index != 0)
			{
				z_index = 0;
				DirtyStackingContext();
			}
		}
		else
		{
			float new_z_index = z_index_property.value;

			if (new_z_index != z_index)
			{
				z_index = new_z_index;

				if (parent != NULL)
					parent->DirtyStackingContext();
			}

			if (!local_stacking_context)
			{
				local_stacking_context = true;
				stacking_context_dirty = true;
			}
		}
	}

	// Dirty the background if it's changed.
    if (all_dirty ||
        changed_properties.find(BACKGROUND_COLOR) != changed_properties.end() ||
		changed_properties.find(OPACITY) != changed_properties.end() ||
		changed_properties.find(IMAGE_COLOR) != changed_properties.end()) {
        background->DirtyBackground();
        decoration->DirtyDecorators(true);
    }

	// Dirty the border if it's changed.
	if (all_dirty || 
		changed_properties.find(BORDER_TOP_WIDTH) != changed_properties.end() ||
		changed_properties.find(BORDER_RIGHT_WIDTH) != changed_properties.end() ||
		changed_properties.find(BORDER_BOTTOM_WIDTH) != changed_properties.end() ||
		changed_properties.find(BORDER_LEFT_WIDTH) != changed_properties.end() ||
		changed_properties.find(BORDER_TOP_COLOR) != changed_properties.end() ||
		changed_properties.find(BORDER_RIGHT_COLOR) != changed_properties.end() ||
		changed_properties.find(BORDER_BOTTOM_COLOR) != changed_properties.end() ||
		changed_properties.find(BORDER_LEFT_COLOR) != changed_properties.end() ||
		changed_properties.find(OPACITY) != changed_properties.end())
		border->DirtyBorder();

	
	// Check for clipping state changes
	if (all_dirty ||
		changed_properties.find(CLIP) != changed_properties.end() ||
		changed_properties.find(OVERFLOW_X) != changed_properties.end() ||
		changed_properties.find(OVERFLOW_Y) != changed_properties.end())
	{
		clipping_state_dirty = true;
	}

	// Check for `perspective' and `perspective-origin' changes
	if (all_dirty ||
		changed_properties.find(PERSPECTIVE) != changed_properties.end() ||
		changed_properties.find(PERSPECTIVE_ORIGIN_X) != changed_properties.end() ||
		changed_properties.find(PERSPECTIVE_ORIGIN_Y) != changed_properties.end())
	{
		DirtyTransformState(true, false, false);
	}

	// Check for `transform' and `transform-origin' changes
	if (all_dirty ||
		changed_properties.find(TRANSFORM) != changed_properties.end() ||
		changed_properties.find(TRANSFORM_ORIGIN_X) != changed_properties.end() ||
		changed_properties.find(TRANSFORM_ORIGIN_Y) != changed_properties.end() ||
		changed_properties.find(TRANSFORM_ORIGIN_Z) != changed_properties.end())
	{
		DirtyTransformState(false, true, false);
	}

	// Check for `animation' changes
	if (all_dirty || changed_properties.find(ANIMATION) != changed_properties.end())
	{
		DirtyAnimation();
	}
}

void Element::DirtyProperties(const PropertyNameList& changed_properties) 
{ 
	if (all_properties_dirty)
		return;

	dirty_properties.insert(changed_properties.begin(), changed_properties.end());
}

void Element::UpdateDirtyProperties()
{
	if (!all_properties_dirty && dirty_properties.empty())
		return;

	auto& update_properties = (all_properties_dirty ? StyleSheetSpecification::GetRegisteredProperties() : dirty_properties);
	
	OnPropertyChange(update_properties);

	all_properties_dirty = false;
	dirty_properties.clear();
}

// Called when a child node has been added somewhere in the hierarchy
void Element::OnChildAdd(Element* child)
{
	if (parent)
		parent->OnChildAdd(child);
}

// Called when a child node has been removed somewhere in the hierarchy
void Element::OnChildRemove(Element* child)
{
	if (parent)
		parent->OnChildRemove(child);
}

// Forces a re-layout of this element, and any other children required.
void Element::DirtyLayout()
{
	Element* document = GetOwnerDocument();
	if (document != NULL)
		document->DirtyLayout();
}

/// Increment/Decrement the layout lock
void Element::LockLayout(bool lock)
{
	ElementDocument* document = GetOwnerDocument();
	if (document != NULL)
		document->LockLayout(lock);
}

// Forces a re-layout of this element, and any other children required.
bool Element::IsLayoutDirty()
{
	Element* document = GetOwnerDocument();
	if (document != NULL)
		return document->IsLayoutDirty();
	return false;
}

// Forces a reevaluation of applicable font effects.
void Element::DirtyFont()
{
	for (size_t i = 0; i < children.size(); ++i)
		children[i]->DirtyFont();
}

void Element::OnReferenceDeactivate()
{
	if (instancer)
	{
		instancer->ReleaseElement(this);
	}
	else
	{
		// Hopefully we can just delete ourselves.
		//delete this;
		Log::Message(Log::LT_WARNING, "Leak detected: element %s not instanced via Rocket Factory. Unable to release.", GetAddress().c_str());
	}
}

void Element::ProcessEvent(Event& event)
{
	if (event == MOUSEDOWN && IsPointWithinElement(Vector2f(event.GetParameter< float >("mouse_x", 0), event.GetParameter< float >("mouse_y", 0))) &&
		event.GetParameter< int >("button", 0) == 0)
		SetPseudoClass("active", true);

	if (event == MOUSESCROLL)
	{
		if (GetScrollHeight() > GetClientHeight())
		{
			Style::Overflow overflow_property = element_meta->computed_values.overflow_y;
			if (overflow_property == Style::Overflow::Auto ||
				overflow_property == Style::Overflow::Scroll)
			{
				// Stop the propagation if the current element has scrollbars.
				// This prevents scrolling in parent elements, which is often unintended. If instead desired behavior is
				// to scroll in parent elements when reaching top/bottom, move StopPropagation inside the next if statement.
				event.StopPropagation();

				int wheel_delta = event.GetParameter< int >("wheel_delta", 0);
				if ((wheel_delta < 0 && GetScrollTop() > 0) ||
					(wheel_delta > 0 && GetScrollHeight() > GetScrollTop() + GetClientHeight()))
				{
					SetScrollTop(GetScrollTop() + wheel_delta * GetLineHeight());
				}
			}
		}

		return;
	}

	if (event.GetTargetElement() == this)
	{
		if (event == MOUSEOVER)
			SetPseudoClass("hover", true);
		else if (event == MOUSEOUT)
			SetPseudoClass("hover", false);
		else if (event == FOCUS)
			SetPseudoClass(FOCUS, true);
		else if (event == BLUR)
			SetPseudoClass(FOCUS, false);
	}
}

const Style::ComputedValues& Element::GetComputedValues() const
{
	return element_meta->computed_values;
}

void Element::GetRML(String& content)
{
	// First we start the open tag, add the attributes then close the open tag.
	// Then comes the children in order, then we add our close tag.
	content += "<";
	content += tag;

	for( auto& [name, variant] : attributes)
	{
		String value;
		if (variant.GetInto(value))
			content += " " + name + "=\"" + value + "\"";
	}

	if (HasChildNodes())
	{
		content += ">";

		GetInnerRML(content);

		content += "</";
		content += tag;
		content += ">";
	}
	else
	{
		content += " />";
	}
}

void Element::SetParent(Element* _parent)
{	
	// If there's an old parent, detach from it first.
	if (parent &&
		parent != _parent)
		parent->RemoveChild(this);

	// Save our parent
	parent = _parent;
}

void Element::ReleaseDeletedElements()
{
	for (size_t i = 0; i < active_children.size(); i++)
	{
		active_children[i]->ReleaseDeletedElements();
	}

	ReleaseElements(deleted_children);
	active_children = children;
}

void Element::ReleaseElements(ElementList& released_elements)
{
	// Remove deleted children from this element.
	while (!released_elements.empty())
	{
		Element* element = released_elements.back();
		released_elements.pop_back();

		// If this element has been added back into our list, then we remove our previous oustanding reference on it
		// and continue.
		if (std::find(children.begin(), children.end(), element) != children.end())
		{
			element->RemoveReference();
			continue;
		}

		// Set the parent to NULL unless it's been reparented already.
		if (element->GetParentNode() == this)
			element->parent = NULL;

		element->RemoveReference();
	}
}

void Element::DirtyOffset()
{
	offset_dirty = true;

	if(transform_state)
		DirtyTransformState(true, true, false);

	// Not strictly true ... ?
	for (size_t i = 0; i < children.size(); i++)
		children[i]->DirtyOffset();
}

void Element::UpdateOffset()
{
	using namespace Style;
	const auto& computed = element_meta->computed_values;
	Position position_property = computed.position;

	if (position_property == Position::Absolute ||
		position_property == Position::Fixed)
	{
		if (offset_parent != NULL)
		{
			const Box& parent_box = offset_parent->GetBox();
			Vector2f containing_block = parent_box.GetSize(Box::PADDING);

			// If the element is anchored left, then the position is offset by that resolved value.
			if (computed.left.type != Left::Auto)
				relative_offset_base.x = parent_box.GetEdge(Box::BORDER, Box::LEFT) + (::Rocket::Core::ResolveProperty(computed.left, containing_block.x) + GetBox().GetEdge(Box::MARGIN, Box::LEFT));

			// If the element is anchored right, then the position is set first so the element's right-most edge
			// (including margins) will render up against the containing box's right-most content edge, and then
			// offset by the resolved value.
			else if (computed.right.type != Right::Auto)
				relative_offset_base.x = containing_block.x + parent_box.GetEdge(Box::BORDER, Box::LEFT) - (::Rocket::Core::ResolveProperty(computed.right, containing_block.x) + GetBox().GetSize(Box::BORDER).x + GetBox().GetEdge(Box::MARGIN, Box::RIGHT));

			// If the element is anchored top, then the position is offset by that resolved value.
			if (computed.top.type != Top::Auto)
				relative_offset_base.y = parent_box.GetEdge(Box::BORDER, Box::TOP) + (::Rocket::Core::ResolveProperty(computed.top, containing_block.y) + GetBox().GetEdge(Box::MARGIN, Box::TOP));

			// If the element is anchored bottom, then the position is set first so the element's right-most edge
			// (including margins) will render up against the containing box's right-most content edge, and then
			// offset by the resolved value.
			else if (computed.bottom.type != Bottom::Auto)
				relative_offset_base.y = containing_block.y + parent_box.GetEdge(Box::BORDER, Box::TOP) - (::Rocket::Core::ResolveProperty(computed.bottom, containing_block.y) + GetBox().GetSize(Box::BORDER).y + GetBox().GetEdge(Box::MARGIN, Box::BOTTOM));
		}
	}
	else if (position_property == Position::Relative)
	{
		if (offset_parent != NULL)
		{
			const Box& parent_box = offset_parent->GetBox();
			Vector2f containing_block = parent_box.GetSize();

			if (computed.left.type != Left::Auto)
				relative_offset_position.x = ::Rocket::Core::ResolveProperty(computed.left, containing_block.x);
			else if (computed.right.type != Right::Auto)
				relative_offset_position.x = -1 * ::Rocket::Core::ResolveProperty(computed.right, containing_block.x);
			else
				relative_offset_position.x = 0;

			if (computed.top.type != Top::Auto)
				relative_offset_position.y = ::Rocket::Core::ResolveProperty(computed.top, containing_block.y);
			else if (computed.bottom.type != Bottom::Auto)
				relative_offset_position.y = -1 * ::Rocket::Core::ResolveProperty(computed.bottom, containing_block.y);
			else
				relative_offset_position.y = 0;
		}
	}
	else
	{
		relative_offset_position.x = 0;
		relative_offset_position.y = 0;
	}
}

void Element::BuildLocalStackingContext()
{
	stacking_context_dirty = false;
	stacking_context.clear();

	BuildStackingContext(&stacking_context);
	std::stable_sort(stacking_context.begin(), stacking_context.end(), ElementSortZIndex());
}

void Element::BuildStackingContext(ElementList* new_stacking_context)
{
	// Build the list of ordered children. Our child list is sorted within the stacking context so stacked elements
	// will render in the right order; ie, positioned elements will render on top of inline elements, which will render
	// on top of floated elements, which will render on top of block elements.
	std::vector< std::pair< Element*, float > > ordered_children;
	for (size_t i = 0; i < children.size(); ++i)
	{
		Element* child = children[i];

		if (!child->IsVisible())
			continue;

		std::pair< Element*, float > ordered_child;
		ordered_child.first = child;

		if (child->GetPosition() != Style::Position::Static)
			ordered_child.second = 3;
		else if (child->GetFloat() != Style::Float::None)
			ordered_child.second = 1;
		else if (child->GetDisplay() == Style::Display::Block)
			ordered_child.second = 0;
		else
			ordered_child.second = 2;

		ordered_children.push_back(ordered_child);
	}

	// Sort the list!
	std::stable_sort(ordered_children.begin(), ordered_children.end(), ElementSortZOrder());

	// Add the list of ordered children into the stacking context in order.
	for (size_t i = 0; i < ordered_children.size(); ++i)
	{
		new_stacking_context->push_back(ordered_children[i].first);

		if (!ordered_children[i].first->local_stacking_context)
			ordered_children[i].first->BuildStackingContext(new_stacking_context);
	}
}

void Element::DirtyStackingContext()
{
	// The first ancestor of ours that doesn't have an automatic z-index is the ancestor that is establishing our local
	// stacking context.
	Element* stacking_context_parent = this;
	while (stacking_context_parent != NULL &&
		   !stacking_context_parent->local_stacking_context)
		stacking_context_parent = stacking_context_parent->GetParentNode();

	if (stacking_context_parent != NULL)
		stacking_context_parent->stacking_context_dirty = true;
}

void Element::DirtyStructure()
{
	// Clear the cached owner document
	owner_document = NULL;
	structure_dirty = true;
}

void Element::DirtyParentStructure()
{
	owner_document = NULL;
	parent_structure_dirty = true;
}

void Element::UpdateStructure()
{
	if (parent_structure_dirty)
	{
		// If children depend on structured selectors, they may need to be updated
		const ElementDefinition* element_definition = GetStyle()->GetDefinition();
		if (element_definition != NULL && element_definition->IsStructurallyVolatile())
		{
			GetStyle()->DirtyDefinition();
		}
	}
	if (structure_dirty || parent_structure_dirty)
	{
		for (size_t i = 0; i < children.size(); ++i)
			children[i]->DirtyParentStructure();

		structure_dirty = false;
		parent_structure_dirty = false;
	}
}


bool Element::Animate(const String & property_name, const Property & target_value, float duration, Tween tween, int num_iterations, bool alternate_direction, float delay, const Property* start_value)
{
	bool result = false;

	if (auto it_animation = StartAnimation(property_name, start_value, num_iterations, alternate_direction, delay); it_animation != animations.end())
	{
		result = it_animation->AddKey(duration, target_value, *this, tween, true);
		if (!result)
			animations.erase(it_animation);
	}

	return result;
}


bool Element::AddAnimationKey(const String & property_name, const Property & target_value, float duration, Tween tween)
{
	ElementAnimation* animation = nullptr;

	for (auto& existing_animation : animations) {
		if (existing_animation.GetPropertyName() == property_name) {
			animation = &existing_animation;
			break;
		}
	}
	if (!animation)
		return false;

	bool result = animation->AddKey(animation->GetDuration() + duration, target_value, *this, tween, true);

	return result;
}


ElementAnimationList::iterator Element::StartAnimation(const String & property_name, const Property* start_value, int num_iterations, bool alternate_direction, float delay)
{
	auto it = std::find_if(animations.begin(), animations.end(), [&](const ElementAnimation& el) { return el.GetPropertyName() == property_name; });

	if (it == animations.end())
	{
		animations.emplace_back();
		it = animations.end() - 1;
	}

	Property value;

	if (start_value)
	{
		value = *start_value;
		if (!value.definition)
			if(auto default_value = GetProperty(property_name))
				value.definition = default_value->definition;	
	}
	else if (auto default_value = GetProperty(property_name))
	{
		value = *default_value;
	}

	if (value.definition)
	{
		double start_time = Clock::GetElapsedTime() + (double)delay;
		*it = ElementAnimation{ property_name, value, start_time, 0.0f, num_iterations, alternate_direction, false };
	}
	else
	{
		animations.erase(it);
		it = animations.end();
	}

	return it;
}


bool Element::AddAnimationKeyTime(const String & property_name, const Property* target_value, float time, Tween tween)
{
	if (!target_value)
		target_value = GetProperty(property_name);
	if (!target_value)
		return false;

	ElementAnimation* animation = nullptr;

	for (auto& existing_animation : animations) {
		if (existing_animation.GetPropertyName() == property_name) {
			animation = &existing_animation;
			break;
		}
	}
	if (!animation)
		return false;

	bool result = animation->AddKey(time, *target_value, *this, tween, true);

	return result;
}

bool Element::StartTransition(const Transition & transition, const Property& start_value, const Property & target_value)
{
	auto it = std::find_if(animations.begin(), animations.end(), [&](const ElementAnimation& el) { return el.GetPropertyName() == transition.name; });

	if (it != animations.end() && !it->IsTransition())
		return false;

	float duration = transition.duration;
	double start_time = Clock::GetElapsedTime() + (double)transition.delay;

	if (it == animations.end())
	{
		// Add transition as new animation
		animations.push_back(
			ElementAnimation{ transition.name, start_value, start_time, 0.0f, 1, false, true }
		);
		it = (animations.end() - 1);
	}
	else
	{
		// Compress the duration based on the progress of the current animation
		float f = it->GetInterpolationFactor();
		f = 1.0f - (1.0f - f)*transition.reverse_adjustment_factor;
		duration = duration * f;
		// Replace old transition
		*it = ElementAnimation{ transition.name, start_value, start_time, 0.0f, 1, false, true };
	}

	bool result = it->AddKey(duration, target_value, *this, transition.tween, true);

	if (result)
		SetProperty(transition.name, start_value);
	else
		animations.erase(it);

	return result;
}


void Element::DirtyAnimation()
{
	dirty_animation = true;
}

void Element::UpdateAnimation()
{
	if (dirty_animation)
	{
		const auto& animation_list = element_meta->computed_values.animation;
		StyleSheet* stylesheet = nullptr;

		if (!animation_list.empty() && (stylesheet = GetStyleSheet()))
		{
			for (auto& animation : animation_list)
			{
				Keyframes* keyframes_ptr = stylesheet->GetKeyframes(animation.name);
				if (keyframes_ptr && keyframes_ptr->blocks.size() >= 1 && !animation.paused)
				{
					auto& property_names = keyframes_ptr->property_names;
					auto& blocks = keyframes_ptr->blocks;

					bool has_from_key = (blocks[0].normalized_time == 0);
					bool has_to_key = (blocks.back().normalized_time == 1);

					// If the first key defines initial conditions for a given property, use those values, else, use this element's current values.
					for (auto& property : property_names)
						StartAnimation(property, (has_from_key ? blocks[0].properties.GetProperty(property) : nullptr), animation.num_iterations, animation.alternate, animation.delay);

					// Need to skip the first and last keys if they set the initial and end conditions, respectively.
					for (int i = (has_from_key ? 1 : 0); i < (int)blocks.size() + (has_to_key ? -1 : 0); i++)
					{
						// Add properties of current key to animation
						float time = blocks[i].normalized_time * animation.duration;
						for (auto& property : blocks[i].properties.GetProperties())
							AddAnimationKeyTime(property.first, &property.second, time, animation.tween);
					}

					// If the last key defines end conditions for a given property, use those values, else, use this element's current values.
					float time = animation.duration;
					for (auto& property : property_names)
						AddAnimationKeyTime(property, (has_to_key ? blocks.back().properties.GetProperty(property) : nullptr), time, animation.tween);
				}
			}
		}

		dirty_animation = false;
	}
}

void Element::AdvanceAnimations()
{
	if (!animations.empty())
	{
		double time = Clock::GetElapsedTime();

		for (auto& animation : animations)
		{
			Property property = animation.UpdateAndGetProperty(time, *this);
			if (property.unit != Property::UNKNOWN)
				SetProperty(animation.GetPropertyName(), property);
		}

		auto it_completed = std::remove_if(animations.begin(), animations.end(), [](const ElementAnimation& animation) { return animation.IsComplete(); });

		std::vector<Dictionary> dictionary_list;
		std::vector<bool> is_transition;
		dictionary_list.reserve(animations.end() - it_completed);
		is_transition.reserve(animations.end() - it_completed);

		for (auto it = it_completed; it != animations.end(); ++it)
		{
			dictionary_list.emplace_back().emplace("property", it->GetPropertyName());
			is_transition.push_back(it->IsTransition());
		}

		// Need to erase elements before submitting event, as iterators might be invalidated when calling external code.
		animations.erase(it_completed, animations.end());

		for (size_t i = 0; i < dictionary_list.size(); i++)
			DispatchEvent(is_transition[i] ? TRANSITIONEND : ANIMATIONEND, dictionary_list[i]);
	}
}



void Element::DirtyTransformState(bool perspective_changed, bool transform_changed, bool parent_transform_changed)
{
	for (size_t i = 0; i < children.size(); ++i)
	{
		children[i]->DirtyTransformState(false, false, transform_changed || parent_transform_changed);
	}

	if (perspective_changed)
	{
		this->transform_state_perspective_dirty = true;
	}
	if (transform_changed)
	{
		this->transform_state_transform_dirty = true;
	}
	if (parent_transform_changed)
	{
		this->transform_state_parent_transform_dirty = true;
	}
}


void Element::UpdateTransformState()
{
	if (!(transform_state_perspective_dirty || transform_state_transform_dirty || transform_state_parent_transform_dirty))
	{
		return;
	}

	const ComputedValues& computed = element_meta->computed_values;

	if (!computed.transform && computed.perspective <= 0)
	{
		transform_state.reset();
		transform_state_perspective_dirty = false;
		transform_state_transform_dirty = false;
		transform_state_parent_transform_dirty = false;
		return;
	}


	if(transform_state_perspective_dirty || transform_state_transform_dirty)
	{
		Context *context = GetContext();
		Vector2f pos = GetAbsoluteOffset(Box::BORDER);
		Vector2f size = GetBox().GetSize(Box::BORDER);


		if (transform_state_perspective_dirty)
		{
			bool have_perspective = false;
			TransformState::Perspective perspective_value;

			perspective_value.vanish = Vector2f(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

			if (computed.perspective > 0.0f)
			{
				have_perspective = true;

				// Compute the perspective value
				perspective_value.distance = computed.perspective;

				// Compute the perspective origin, if necessary
				if (computed.perspective_origin_x.type == Style::PerspectiveOrigin::Percentage)
					perspective_value.vanish.x = pos.x + computed.perspective_origin_x.value * 0.01f * size.x;
				else
					perspective_value.vanish.x = pos.x + computed.perspective_origin_x.value;

				if (computed.perspective_origin_y.type == Style::PerspectiveOrigin::Percentage)
					perspective_value.vanish.y = pos.y + computed.perspective_origin_y.value * 0.01f * size.y;
				else
					perspective_value.vanish.y = pos.y + computed.perspective_origin_y.value;
			}

			if (have_perspective && context)
			{
				if (!transform_state)
					transform_state = std::make_unique<TransformState>();
				perspective_value.view_size = context->GetDimensions();
				transform_state->SetPerspective(&perspective_value);
			}
			else if (transform_state)
			{
				transform_state->SetPerspective(nullptr);
			}

			transform_state_perspective_dirty = false;
		}

		if (transform_state_transform_dirty)
		{
			bool have_local_perspective = false;
			TransformState::LocalPerspective local_perspective;

			bool have_transform = false;
			Matrix4f transform_value = Matrix4f::Identity();
			Vector3f transform_origin(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, 0);

			if (computed.transform)
			{
				int n = computed.transform->GetNumPrimitives();
				for (int i = 0; i < n; ++i)
				{
					const Transforms::Primitive &primitive = computed.transform->GetPrimitive(i);

					if (primitive.ResolvePerspective(local_perspective.distance, *this))
					{
						have_local_perspective = true;
					}

					Matrix4f matrix;
					if (primitive.ResolveTransform(matrix, *this))
					{
						transform_value *= matrix;
						have_transform = true;
					}
				}

				// Compute the transform origin
				if (computed.transform_origin_x.type == Style::TransformOrigin::Percentage)
					transform_origin.x = pos.x + computed.transform_origin_x.value * size.x * 0.01f;
				else
					transform_origin.x = pos.x + computed.transform_origin_x.value;

				if (computed.transform_origin_y.type == Style::TransformOrigin::Percentage)
					transform_origin.y = pos.y + computed.transform_origin_y.value * size.y * 0.01f;
				else
					transform_origin.y = pos.y + computed.transform_origin_y.value;

				transform_origin.z = computed.transform_origin_z;
			}

			if (have_local_perspective && context)
			{
				if (!transform_state)
					transform_state = std::make_unique<TransformState>();
				local_perspective.view_size = context->GetDimensions();
				transform_state->SetLocalPerspective(&local_perspective);
			}
			else if(transform_state)
			{
				transform_state->SetLocalPerspective(nullptr);
			}

			if (have_transform)
			{
				// TODO: If we're using the global projection matrix
				// (perspective < 0), then scale the coordinates from
				// pixel space to 3D unit space.

				// Transform the Rocket context so that the computed `transform_origin'
				// lies at the coordinate system origin.
				transform_value =
					Matrix4f::Translate(transform_origin)
					* transform_value
					* Matrix4f::Translate(-transform_origin);

				if (!transform_state)
					transform_state = std::make_unique<TransformState>();
				transform_state->SetTransform(&transform_value);
			}
			else if (transform_state)
			{
				transform_state->SetTransform(nullptr);
			}

			transform_state_transform_dirty = false;
		}
	}


	if (transform_state_parent_transform_dirty)
	{
		// We need to clean up from the top-most to the bottom-most dirt.
		if (parent)
		{
			parent->UpdateTransformState();
		}

		if (transform_state)
		{
			// Store the parent's new full transform as our parent transform
			Element *node = nullptr;
			Matrix4f parent_transform;
			for (node = parent; node; node = node->parent)
			{
				if (node->GetTransformState() && node->GetTransformState()->GetRecursiveTransform(&parent_transform))
				{
					transform_state->SetParentRecursiveTransform(&parent_transform);
					break;
				}
			}
			if (!node)
			{
				transform_state->SetParentRecursiveTransform(nullptr);
			}
		}

		transform_state_parent_transform_dirty = false;
	}

	// If we neither have a local perspective, nor a perspective nor a
	// transform, we don't need to keep the large TransformState object
	// around. GetEffectiveTransformState() will then recursively visit
	// parents in order to find a non-trivial TransformState.
	if (transform_state && !transform_state->GetLocalPerspective(nullptr) && !transform_state->GetPerspective(nullptr) && !transform_state->GetTransform(nullptr))
	{
		transform_state.reset();
	}
}

}
}
