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

  
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementInstancer.h"
#include "../../Include/RmlUi/Core/ElementScroll.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Dictionary.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/PropertiesIteratorView.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "Clock.h"
#include "ComputeProperty.h"
#include "DataModel.h"
#include "ElementAnimation.h"
#include "ElementBackgroundBorder.h"
#include "ElementDefinition.h"
#include "ElementStyle.h"
#include "EventDispatcher.h"
#include "EventSpecification.h"
#include "ElementDecoration.h"
#include "LayoutEngine.h"
#include "PluginRegistry.h"
#include "PropertiesIterator.h"
#include "Pool.h"
#include "StyleSheetParser.h"
#include "StyleSheetNode.h"
#include "TransformState.h"
#include "TransformUtilities.h"
#include "XMLParseTools.h"
#include <algorithm>
#include <cmath>

namespace Rml {

// Determines how many levels up in the hierarchy the OnChildAdd and OnChildRemove are called (starting at the child itself)
static constexpr int ChildNotifyLevels = 2;

// Meta objects for element collected in a single struct to reduce memory allocations
struct ElementMeta
{
	ElementMeta(Element* el) : event_dispatcher(el), style(el), background_border(el), decoration(el), scroll(el) {}
	SmallUnorderedMap<EventId, EventListener*> attribute_event_listeners;
	EventDispatcher event_dispatcher;
	ElementStyle style;
	ElementBackgroundBorder background_border;
	ElementDecoration decoration;
	ElementScroll scroll;
	Style::ComputedValues computed_values;
};


static Pool< ElementMeta > element_meta_chunk_pool(200, true);


/// Constructs a new RmlUi element.
Element::Element(const String& tag) : tag(tag), relative_offset_base(0, 0), relative_offset_position(0, 0), absolute_offset(0, 0), scroll_offset(0, 0), content_offset(0, 0), content_box(0, 0), 
transform_state(), dirty_transform(false), dirty_perspective(false), dirty_animation(false), dirty_transition(false)
{
	RMLUI_ASSERT(tag == StringUtilities::ToLower(tag));
	parent = nullptr;
	focus = nullptr;
	instancer = nullptr;
	owner_document = nullptr;

	offset_fixed = false;
	offset_parent = nullptr;
	offset_dirty = true;

	client_area = Box::PADDING;

	baseline = 0.0f;

	num_non_dom_children = 0;

	visible = true;

	z_index = 0;

	local_stacking_context = false;
	local_stacking_context_forced = false;
	stacking_context_dirty = false;

	structure_dirty = false;

	computed_values_are_default_initialized = true;

	meta = element_meta_chunk_pool.AllocateAndConstruct(this);
	data_model = nullptr;
}

Element::~Element()
{
	RMLUI_ASSERT(parent == nullptr);	

	PluginRegistry::NotifyElementDestroy(this);

	// A simplified version of RemoveChild() for destruction.
	for (ElementPtr& child : children)
	{
		Element* child_ancestor = child.get();
		for (int i = 0; i <= ChildNotifyLevels && child_ancestor; i++, child_ancestor = child_ancestor->GetParentNode())
			child_ancestor->OnChildRemove(child.get());

		child->SetParent(nullptr);
	}

	children.clear();
	num_non_dom_children = 0;

	element_meta_chunk_pool.DestroyAndDeallocate(meta);
}

void Element::Update(float dp_ratio, Vector2f vp_dimensions)
{
#ifdef RMLUI_ENABLE_PROFILING
	auto name = GetAddress(false, false);
	RMLUI_ZoneScoped;
	RMLUI_ZoneText(name.c_str(), name.size());
#endif

	OnUpdate();

	UpdateStructure();

	HandleTransitionProperty();
	HandleAnimationProperty();
	AdvanceAnimations();

	meta->scroll.Update();

	UpdateProperties(dp_ratio, vp_dimensions);

	// Do en extra pass over the animations and properties if the 'animation' property was just changed.
	if (dirty_animation)
	{
		HandleAnimationProperty();
		AdvanceAnimations();
		UpdateProperties(dp_ratio, vp_dimensions);
	}

	meta->decoration.InstanceDecorators();

	for (size_t i = 0; i < children.size(); i++)
		children[i]->Update(dp_ratio, vp_dimensions);
}

void Element::UpdateProperties(const float dp_ratio, const Vector2f vp_dimensions)
{
	meta->style.UpdateDefinition();

	if (meta->style.AnyPropertiesDirty())
	{
		const ComputedValues* parent_values = parent ? &parent->GetComputedValues() : nullptr;
		const ComputedValues* document_values = owner_document ? &owner_document->GetComputedValues() : nullptr;

		// Compute values and clear dirty properties
		PropertyIdSet dirty_properties = meta->style.ComputeValues(meta->computed_values, parent_values, document_values, computed_values_are_default_initialized, dp_ratio, vp_dimensions);

		computed_values_are_default_initialized = false;

		// Computed values are just calculated and can safely be used in OnPropertyChange.
		// However, new properties set during this call will not be available until the next update loop.
		if (!dirty_properties.Empty())
			OnPropertyChange(dirty_properties);
	}
}

void Element::Render()
{
#ifdef RMLUI_ENABLE_PROFILING
	auto name = GetAddress(false, false);
	RMLUI_ZoneScoped;
	RMLUI_ZoneText(name.c_str(), name.size());
#endif

	// TODO: This is a work-around for the dirty offset not being properly updated when used by (stacking context?) children. This results
	// in scrolling not working properly. We don't care about the return value, the call is only used to force the absolute offset to update.
	if (offset_dirty)
		GetAbsoluteOffset(Box::BORDER);

	// Rebuild our stacking context if necessary.
	if (stacking_context_dirty)
		BuildLocalStackingContext();

	UpdateTransformState();

	// Render all elements in our local stacking context that have a z-index beneath our local index of 0.
	size_t i = 0;
	for (; i < stacking_context.size() && stacking_context[i]->z_index < 0; ++i)
		stacking_context[i]->Render();

	// Apply our transform
	ElementUtilities::ApplyTransform(*this);

	// Set up the clipping region for this element.
	if (ElementUtilities::SetClippingRegion(this))
	{
		meta->background_border.Render(this);
		meta->decoration.RenderDecorators();

		{
			RMLUI_ZoneScopedNC("OnRender", 0x228B22);

			OnRender();
		}
	}

	// Render the rest of the elements in the stacking context.
	for (; i < stacking_context.size(); ++i)
		stacking_context[i]->Render();
}

// Clones this element, returning a new, unparented element.
ElementPtr Element::Clone() const
{
	ElementPtr clone;

	if (instancer)
	{
		clone = instancer->InstanceElement(nullptr, GetTagName(), attributes);
		if (clone)
			clone->SetInstancer(instancer);
	}
	else
		clone = Factory::InstanceElement(nullptr, GetTagName(), GetTagName(), attributes);

	if (clone != nullptr)
	{
		// Set the attributes manually in case the instancer does not set them.
		clone->SetAttributes(attributes);

		String inner_rml;
		GetInnerRML(inner_rml);

		clone->SetInnerRML(inner_rml);
	}

	return clone;
}

// Sets or removes a class on the element.
void Element::SetClass(const String& class_name, bool activate)
{
	meta->style.SetClass(class_name, activate);
}

// Checks if a class is set on the element.
bool Element::IsClassSet(const String& class_name) const
{
	return meta->style.IsClassSet(class_name);
}

// Specifies the entire list of classes for this element. This will replace any others specified.
void Element::SetClassNames(const String& class_names)
{
	SetAttribute("class", class_names);
}

/// Return the active class list
String Element::GetClassNames() const
{
	return meta->style.GetClassNames();
}

// Returns the active style sheet for this element. This may be nullptr.
const StyleSheet* Element::GetStyleSheet() const
{
	if (ElementDocument * document = GetOwnerDocument())
		return document->GetStyleSheet();
	return nullptr;
}

// Fills an String with the full address of this element.
String Element::GetAddress(bool include_pseudo_classes, bool include_parents) const
{
	// Add the tag name onto the address.
	String address(tag);

	// Add the ID if we have one.
	if (!id.empty())
	{
		address += "#";
		address += id;
	}

	String classes = meta->style.GetClassNames();
	if (!classes.empty())
	{
		classes = StringUtilities::Replace(classes, ' ', '.');
		address += ".";
		address += classes;
	}

	if (include_pseudo_classes)
	{
		const PseudoClassMap& pseudo_classes = meta->style.GetActivePseudoClasses();
		for (auto& pseudo_class : pseudo_classes)
		{
			address += ":";
			address += pseudo_class.first;
		}
	}

	if (include_parents && parent)
	{
		address += " < ";
		return address + parent->GetAddress(include_pseudo_classes, true);
	}
	else
		return address;
}

// Sets the position of this element, as a two-dimensional offset from another element.
void Element::SetOffset(Vector2f offset, Element* _offset_parent, bool _offset_fixed)
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

		if (offset_parent != nullptr)
			absolute_offset = offset_parent->GetAbsoluteOffset(Box::BORDER) + relative_offset_base + relative_offset_position;
		else
			absolute_offset = relative_offset_base + relative_offset_position;

		// Add any parent scrolling onto our position as well. Could cache this if required.
		if (!offset_fixed)
		{
			Element* scroll_parent = parent;
			while (scroll_parent != nullptr)
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
void Element::SetContentBox(Vector2f _content_offset, Vector2f _content_box)
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

		OnResize();

		meta->background_border.DirtyBackground();
		meta->background_border.DirtyBorder();
		meta->decoration.DirtyDecoratorsData();
	}
}

// Adds a box to the end of the list describing this element's geometry.
void Element::AddBox(const Box& box, Vector2f offset)
{
	additional_boxes.emplace_back(PositionedBox{ box, offset });

	OnResize();

	meta->background_border.DirtyBackground();
	meta->background_border.DirtyBorder();
	meta->decoration.DirtyDecoratorsData();
}

// Returns one of the boxes describing the size of the element.
const Box& Element::GetBox()
{
	return main_box;
}

// Returns one of the boxes describing the size of the element.
const Box& Element::GetBox(int index, Vector2f& offset)
{
	offset = Vector2f(0);

	if (index < 1)
		return main_box;
	
	const int additional_box_index = index - 1;
	if (additional_box_index >= (int)additional_boxes.size())
		return main_box;

	offset = additional_boxes[additional_box_index].offset;

	return additional_boxes[additional_box_index].box;
}

// Returns the number of boxes making up this element's geometry.
int Element::GetNumBoxes()
{
	return 1 + (int)additional_boxes.size();
}

// Returns the baseline of the element, in pixels offset from the bottom of the element's content area.
float Element::GetBaseline() const
{
	return baseline;
}

// Gets the intrinsic dimensions of this element, if it is of a type that has an inherent size.
bool Element::GetIntrinsicDimensions(Vector2f& RMLUI_UNUSED_PARAMETER(dimensions), float& RMLUI_UNUSED_PARAMETER(ratio))
{
	RMLUI_UNUSED(dimensions);
	RMLUI_UNUSED(ratio);
	return false;
}

// Checks if a given point in screen coordinates lies within the bordered area of this element.
bool Element::IsPointWithinElement(const Vector2f point)
{
	const Vector2f position = GetAbsoluteOffset(Box::BORDER);

	for (int i = 0; i < GetNumBoxes(); ++i)
	{
		Vector2f box_offset;
		const Box& box = GetBox(i, box_offset);

		const Vector2f box_position = position + box_offset;
		const Vector2f box_dimensions = box.GetSize(Box::BORDER);
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
FontFaceHandle Element::GetFontFaceHandle() const
{
	return meta->computed_values.font_face_handle;
}

// Sets a local property override on the element.
bool Element::SetProperty(const String& name, const String& value)
{
	// The name may be a shorthand giving us multiple underlying properties
	PropertyDictionary properties;
	if (!StyleSheetSpecification::ParsePropertyDeclaration(properties, name, value))
	{
		Log::Message(Log::LT_WARNING, "Syntax error parsing inline property declaration '%s: %s;'.", name.c_str(), value.c_str());
		return false;
	}
	for (auto& property : properties.GetProperties())
	{
		if (!meta->style.SetProperty(property.first, property.second))
			return false;
	}
	return true;
}

// Sets a local property override on the element to a pre-parsed value.
bool Element::SetProperty(PropertyId id, const Property& property)
{
	return meta->style.SetProperty(id, property);
}

// Removes a local property override on the element.
void Element::RemoveProperty(const String& name)
{
	meta->style.RemoveProperty(StyleSheetSpecification::GetPropertyId(name));
}

// Removes a local property override on the element.
void Element::RemoveProperty(PropertyId id)
{
	meta->style.RemoveProperty(id);
}

// Returns one of this element's properties.
const Property* Element::GetProperty(const String& name)
{
	return meta->style.GetProperty(StyleSheetSpecification::GetPropertyId(name));
}

// Returns one of this element's properties.
const Property* Element::GetProperty(PropertyId id)
{
	return meta->style.GetProperty(id);
}

// Returns one of this element's properties.
const Property* Element::GetLocalProperty(const String& name)
{
	return meta->style.GetLocalProperty(StyleSheetSpecification::GetPropertyId(name));
}

const Property* Element::GetLocalProperty(PropertyId id)
{
	return meta->style.GetLocalProperty(id);
}

const PropertyMap& Element::GetLocalStyleProperties()
{
	return meta->style.GetLocalStyleProperties();
}

float Element::ResolveNumericProperty(const Property *property, float base_value)
{
	return meta->style.ResolveNumericProperty(property, base_value);
}

float Element::ResolveNumericProperty(const String& property_name)
{
	auto property = meta->style.GetProperty(StyleSheetSpecification::GetPropertyId(property_name));
	if (!property)
		return 0.0f;

	if (property->unit & Property::ANGLE)
		return ComputeAngle(*property);

	RelativeTarget relative_target = RelativeTarget::None;
	if (property->definition)
		relative_target = property->definition->GetRelativeTarget();

	float result = meta->style.ResolveLength(property, relative_target);
	
	return result;
}

Vector2f Element::GetContainingBlock()
{
	Vector2f containing_block(0, 0);

	if (offset_parent != nullptr)
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
	return meta->computed_values.position;
}

Style::Float Element::GetFloat()
{
	return meta->computed_values.float_;
}

Style::Display Element::GetDisplay()
{
	return meta->computed_values.display;
}

float Element::GetLineHeight()
{
	return meta->computed_values.line_height.value;
}

// Returns this element's TransformState
const TransformState *Element::GetTransformState() const noexcept
{
	return transform_state.get();
}

// Project a 2D point in pixel coordinates onto the element's plane.
bool Element::Project(Vector2f& point) const noexcept
{
	if(!transform_state || !transform_state->GetTransform())
		return true;

	// The input point is in window coordinates. Need to find the projection of the point onto the current element plane,
	// taking into account the full transform applied to the element.

	if (const Matrix4f* inv_transform = transform_state->GetInverseTransform())
	{
		// Pick two points forming a line segment perpendicular to the window.
		Vector4f window_points[2] = {{ point.x, point.y, -10, 1}, { point.x, point.y, 10, 1 }};

		// Project them into the local element space.
		window_points[0] = *inv_transform * window_points[0];
		window_points[1] = *inv_transform * window_points[1];

		Vector3f local_points[2] = {
			window_points[0].PerspectiveDivide(),
			window_points[1].PerspectiveDivide()
		};

		// Construct a ray from the two projected points in the local space of the current element.
		// Find the intersection with the z=0 plane to produce our destination point.
		Vector3f ray = local_points[1] - local_points[0];

		// Only continue if we are not close to parallel with the plane.
		if(std::fabs(ray.z) > 1.0f)
		{
			// Solving the line equation p = p0 + t*ray for t, knowing that p.z = 0, produces the following.
			float t = -local_points[0].z / ray.z;
			Vector3f p = local_points[0] + ray * t;

			point = Vector2f(p.x, p.y);
			return true;
		}
	}

	// The transformation matrix is either singular, or the ray is parallel to the element's plane.
	return false;
}

PropertiesIteratorView Element::IterateLocalProperties() const
{
	return PropertiesIteratorView(MakeUnique<PropertiesIterator>(meta->style.Iterate()));
}


// Sets or removes a pseudo-class on the element.
void Element::SetPseudoClass(const String& pseudo_class, bool activate)
{
	if (meta->style.SetPseudoClass(pseudo_class, activate, false))
		OnPseudoClassChange(pseudo_class, activate);
}

// Checks if a specific pseudo-class has been set on the element.
bool Element::IsPseudoClassSet(const String& pseudo_class) const
{
	return meta->style.IsPseudoClassSet(pseudo_class);
}

// Checks if a complete set of pseudo-classes are set on the element.
bool Element::ArePseudoClassesSet(const StringList& pseudo_classes) const
{
	for (const String& pseudo_class : pseudo_classes)
	{
		if (!IsPseudoClassSet(pseudo_class))
			return false;
	}

	return true;
}

// Gets a list of the current active pseudo classes
StringList Element::GetActivePseudoClasses() const
{
	const PseudoClassMap& pseudo_classes = meta->style.GetActivePseudoClasses();
	StringList names;
	names.reserve(pseudo_classes.size());
	for (auto& pseudo_class : pseudo_classes)
	{
		names.push_back(pseudo_class.first);
	}

	return names;
}

void Element::OverridePseudoClass(Element* element, const String& pseudo_class, bool activate)
{
	RMLUI_ASSERT(element);
	element->GetStyle()->SetPseudoClass(pseudo_class, activate, true);
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
	auto it = attributes.find(name);
	if (it != attributes.end())
	{
		attributes.erase(it);

		ElementAttributes changed_attributes;
		changed_attributes.emplace(name, Variant());
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
Context* Element::GetContext() const
{
	ElementDocument* document = GetOwnerDocument();
	if (document != nullptr)
		return document->GetContext();

	return nullptr;
}

// Set a group of attributes
void Element::SetAttributes(const ElementAttributes& _attributes)
{
	attributes.reserve(attributes.size() + _attributes.size());
	for (auto& pair : _attributes)
		attributes[pair.first] = pair.second;

	OnAttributeChange(_attributes);
}

// Returns the number of attributes on the element.
int Element::GetNumAttributes() const
{
	return (int)attributes.size();
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
	return GetBox().GetSize(client_area).x - meta->scroll.GetScrollbarSize(ElementScroll::VERTICAL);
}

// Gets the inner height of the element.
float Element::GetClientHeight()
{
	return GetBox().GetSize(client_area).y - meta->scroll.GetScrollbarSize(ElementScroll::HORIZONTAL);
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
	const float new_offset = Math::Clamp(Math::RoundFloat(scroll_left), 0.0f, GetScrollWidth() - GetClientWidth());
	if (new_offset != scroll_offset.x)
	{
		scroll_offset.x = new_offset;
		meta->scroll.UpdateScrollbar(ElementScroll::HORIZONTAL);
		DirtyOffset();

		DispatchEvent(EventId::Scroll, Dictionary());
	}
}

// Gets the top scroll offset of the element.
float Element::GetScrollTop()
{
	return scroll_offset.y;
}

// Sets the top scroll offset of the element.
void Element::SetScrollTop(float scroll_top)
{
	const float new_offset = Math::Clamp(Math::RoundFloat(scroll_top), 0.0f, GetScrollHeight() - GetClientHeight());
	if(new_offset != scroll_offset.y)
	{
		scroll_offset.y = new_offset;
		meta->scroll.UpdateScrollbar(ElementScroll::VERTICAL);
		DirtyOffset();

		DispatchEvent(EventId::Scroll, Dictionary());
	}
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
ElementStyle* Element::GetStyle() const
{
	return &meta->style;
}

// Gets the document this element belongs to.
ElementDocument* Element::GetOwnerDocument() const
{
#ifdef RMLUI_DEBUG
	if (parent && !owner_document)
	{
		// Since we have a parent but no owner_document, then we must be a 'loose' element -- that is, constructed
		// outside of a document and not attached to a child of any element in the hierarchy of a document.
		// This check ensures that we didn't just forget to set the owner document.
		RMLUI_ASSERT(!parent->GetOwnerDocument());
	}
#endif

	return owner_document;
}

// Gets this element's parent node.
Element* Element::GetParentNode() const
{
	return parent;
}

// Recursively search for a ancestor of this node matching the given selector.
Element* Element::Closest(const String& selectors) const
{
	StyleSheetNode root_node;
	StyleSheetNodeListRaw leaf_nodes = StyleSheetParser::ConstructNodes(root_node, selectors);

	if (leaf_nodes.empty())
	{
		Log::Message(Log::LT_WARNING, "Query selector '%s' is empty. In element %s", selectors.c_str(), GetAddress().c_str());
		return nullptr;
	}

	Element* parent = GetParentNode();

	while(parent)
	{
		for (const StyleSheetNode* node : leaf_nodes)
		{
			if (node->IsApplicable(parent, false))
			{
				return parent;
			}
		}
		
		parent = parent->GetParentNode();
	}

	return nullptr;
}

// Gets the element immediately following this one in the tree.
Element* Element::GetNextSibling() const
{
	if (parent == nullptr)
		return nullptr;

	for (size_t i = 0; i < parent->children.size() - (parent->num_non_dom_children + 1); i++)
	{
		if (parent->children[i].get() == this)
			return parent->children[i + 1].get();
	}

	return nullptr;
}

// Gets the element immediately preceding this one in the tree.
Element* Element::GetPreviousSibling() const
{
	if (parent == nullptr)
		return nullptr;

	for (size_t i = 1; i < parent->children.size() - parent->num_non_dom_children; i++)
	{
		if (parent->children[i].get() == this)
			return parent->children[i - 1].get();
	}

	return nullptr;
}

// Returns the first child of this element.
Element* Element::GetFirstChild() const
{
	if (GetNumChildren() > 0)
		return children[0].get();

	return nullptr;
}

// Gets the last child of this element.
Element* Element::GetLastChild() const
{
	if (GetNumChildren() > 0)
		return (children.end() - (num_non_dom_children + 1))->get();

	return nullptr;
}

Element* Element::GetChild(int index) const
{
	if (index < 0 || index >= (int) children.size())
		return nullptr;

	return children[index].get();
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
	RMLUI_ZoneScopedC(0x6495ED);

	// Remove all DOM children.
	while ((int) children.size() > num_non_dom_children)
		RemoveChild(children.front().get());

	if(!rml.empty())
		Factory::InstanceElementText(this, rml);
}

// Sets the current element as the focus object.
bool Element::Focus()
{
	// Are we allowed focus?
	Style::Focus focus_property = meta->computed_values.focus;
	if (focus_property == Style::Focus::None)
		return false;

	// Ask our context if we can switch focus.
	Context* context = GetContext();
	if (context == nullptr)
		return false;

	if (!context->OnFocusChange(this))
		return false;

	// Set this as the end of the focus chain.
	focus = nullptr;

	// Update the focus chain up the hierarchy.
	Element* element = this;
	while (Element* parent = element->GetParentNode())
	{
		parent->focus = element;
		element = parent;
	}

	return true;
}

// Removes focus from from this element.
void Element::Blur()
{
	if (parent)
	{
		Context* context = GetContext();
		if (context == nullptr)
			return;

		if (context->GetFocusElement() == this)
		{
			parent->Focus();
		}
		else if (parent->focus == this)
		{
			parent->focus = nullptr;
		}
	}
}

// Fakes a mouse click on this element.
void Element::Click()
{
	Context* context = GetContext();
	if (context == nullptr)
		return;

	context->GenerateClickEvent(this);
}

// Adds an event listener
void Element::AddEventListener(const String& event, EventListener* listener, const bool in_capture_phase)
{
	const EventId id = EventSpecificationInterface::GetIdOrInsert(event);
	meta->event_dispatcher.AttachEvent(id, listener, in_capture_phase);
}

// Adds an event listener
void Element::AddEventListener(const EventId id, EventListener* listener, const bool in_capture_phase)
{
	meta->event_dispatcher.AttachEvent(id, listener, in_capture_phase);
}

// Removes an event listener from this element.
void Element::RemoveEventListener(const String& event, EventListener* listener, bool in_capture_phase)
{
	EventId id = EventSpecificationInterface::GetIdOrInsert(event);
	meta->event_dispatcher.DetachEvent(id, listener, in_capture_phase);
}

// Removes an event listener from this element.
void Element::RemoveEventListener(EventId id, EventListener* listener, bool in_capture_phase)
{
	meta->event_dispatcher.DetachEvent(id, listener, in_capture_phase);
}


// Dispatches the specified event
bool Element::DispatchEvent(const String& type, const Dictionary& parameters)
{
	const EventSpecification& specification = EventSpecificationInterface::GetOrInsert(type);
	return EventDispatcher::DispatchEvent(this, specification.id, type, parameters, specification.interruptible, specification.bubbles, specification.default_action_phase);
}

// Dispatches the specified event
bool Element::DispatchEvent(const String& type, const Dictionary& parameters, bool interruptible, bool bubbles)
{
	const EventSpecification& specification = EventSpecificationInterface::GetOrInsert(type);
	return EventDispatcher::DispatchEvent(this, specification.id, type, parameters, interruptible, bubbles, specification.default_action_phase);
}

// Dispatches the specified event
bool Element::DispatchEvent(EventId id, const Dictionary& parameters)
{
	const EventSpecification& specification = EventSpecificationInterface::Get(id);
	return EventDispatcher::DispatchEvent(this, specification.id, specification.type, parameters, specification.interruptible, specification.bubbles, specification.default_action_phase);
}

// Scrolls the parent element's contents so that this element is visible.
void Element::ScrollIntoView(bool align_with_top)
{
	Vector2f size(0, 0);
	if (!align_with_top)
		size.y = main_box.GetSize(Box::BORDER).y;

	Element* scroll_parent = parent;
	while (scroll_parent != nullptr)
	{
		using Style::Overflow;
		const ComputedValues& computed = scroll_parent->GetComputedValues();
		const bool scrollable_box_x = (computed.overflow_x != Overflow::Visible && computed.overflow_x != Overflow::Hidden);
		const bool scrollable_box_y = (computed.overflow_y != Overflow::Visible && computed.overflow_y != Overflow::Hidden);

		const Vector2f parent_scroll_size = { scroll_parent->GetScrollWidth(), scroll_parent->GetScrollHeight() };
		const Vector2f parent_client_size = { scroll_parent->GetClientWidth(), scroll_parent->GetClientHeight() };

		if ((scrollable_box_x && parent_scroll_size.x > parent_client_size.x) ||
			(scrollable_box_y && parent_scroll_size.y > parent_client_size.y))
		{
			const Vector2f relative_offset = scroll_parent->GetAbsoluteOffset(Box::BORDER) - GetAbsoluteOffset(Box::BORDER);

			Vector2f scroll_offset(scroll_parent->GetScrollLeft(), scroll_parent->GetScrollTop());
			scroll_offset -= relative_offset;
			scroll_offset.x += scroll_parent->GetClientLeft();
			scroll_offset.y += scroll_parent->GetClientTop();

			if (!align_with_top)
				scroll_offset.y -= (parent_client_size.y - size.y);

			if (scrollable_box_x)
				scroll_parent->SetScrollLeft(scroll_offset.x);
			if (scrollable_box_y)
				scroll_parent->SetScrollTop(scroll_offset.y);
		}

		scroll_parent = scroll_parent->GetParentNode();
	}
}

// Appends a child to this element
Element* Element::AppendChild(ElementPtr child, bool dom_element)
{
	RMLUI_ASSERT(child);
	Element* child_ptr = child.get();
	if (dom_element)
		children.insert(children.end() - num_non_dom_children, std::move(child));
	else
	{
		children.push_back(std::move(child));
		num_non_dom_children++;
	}
	// Set parent just after inserting into children. This allows us to eg. get our previous sibling in SetParent.
	child_ptr->SetParent(this);

	Element* ancestor = child_ptr;
	for (int i = 0; i <= ChildNotifyLevels && ancestor; i++, ancestor = ancestor->GetParentNode())
		ancestor->OnChildAdd(child_ptr);

	DirtyStackingContext();
	DirtyStructure();

	if (dom_element)
		DirtyLayout();

	return child_ptr;
}

// Adds a child to this element, directly after the adjacent element. Inherits
// the dom/non-dom status from the adjacent element.
Element* Element::InsertBefore(ElementPtr child, Element* adjacent_element)
{
	RMLUI_ASSERT(child);
	// Find the position in the list of children of the adjacent element. If
	// it's nullptr or we can't find it, then we insert it at the end of the dom
	// children, as a dom element.
	size_t child_index = 0;
	bool found_child = false;
	if (adjacent_element)
	{
		for (child_index = 0; child_index < children.size(); child_index++)
		{
			if (children[child_index].get() == adjacent_element)
			{
				found_child = true;
				break;
			}
		}
	}

	Element* child_ptr = nullptr;

	if (found_child)
	{
		child_ptr = child.get();

		if ((int) child_index >= GetNumChildren())
			num_non_dom_children++;
		else
			DirtyLayout();

		children.insert(children.begin() + child_index, std::move(child));
		child_ptr->SetParent(this);

		Element* ancestor = child_ptr;
		for (int i = 0; i <= ChildNotifyLevels && ancestor; i++, ancestor = ancestor->GetParentNode())
			ancestor->OnChildAdd(child_ptr);

		DirtyStackingContext();
		DirtyStructure();
	}
	else
	{
		child_ptr = AppendChild(std::move(child));
	}	

	return child_ptr;
}

// Replaces the second node with the first node.
ElementPtr Element::ReplaceChild(ElementPtr inserted_element, Element* replaced_element)
{
	RMLUI_ASSERT(inserted_element);
	auto insertion_point = children.begin();
	while (insertion_point != children.end() && insertion_point->get() != replaced_element)
	{
		++insertion_point;
	}

	Element* inserted_element_ptr = inserted_element.get();

	if (insertion_point == children.end())
	{
		AppendChild(std::move(inserted_element));
		return nullptr;
	}

	children.insert(insertion_point, std::move(inserted_element));
	inserted_element_ptr->SetParent(this);

	ElementPtr result = RemoveChild(replaced_element);

	Element* ancestor = inserted_element_ptr;
	for (int i = 0; i <= ChildNotifyLevels && ancestor; i++, ancestor = ancestor->GetParentNode())
		ancestor->OnChildAdd(inserted_element_ptr);

	return result;
}

// Removes the specified child
ElementPtr Element::RemoveChild(Element* child)
{
	size_t child_index = 0;

	for (auto itr = children.begin(); itr != children.end(); ++itr)
	{
		// Add the element to the delete list
		if (itr->get() == child)
		{
			Element* ancestor = child;
			for (int i = 0; i <= ChildNotifyLevels && ancestor; i++, ancestor = ancestor->GetParentNode())
				ancestor->OnChildRemove(child);

			if (child_index >= children.size() - num_non_dom_children)
				num_non_dom_children--;

			ElementPtr detached_child = std::move(*itr);
			children.erase(itr);

			// Remove the child element as the focused child of this element.
			if (child == focus)
			{
				focus = nullptr;

				// If this child (or a descendant of this child) is the context's currently
				// focused element, set the focus to us instead.
				if (Context * context = GetContext())
				{
					Element* focus_element = context->GetFocusElement();
					while (focus_element)
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

			detached_child->SetParent(nullptr);

			DirtyLayout();
			DirtyStackingContext();
			DirtyStructure();

			return detached_child;
		}

		child_index++;
	}

	return nullptr;
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
		if (search_root == nullptr)
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

static Element* QuerySelectorMatchRecursive(const StyleSheetNodeListRaw& nodes, Element* element)
{
	const int num_children = element->GetNumChildren();

	for (int i = 0; i < num_children; i++)
	{
		Element* child = element->GetChild(i);

		for (const StyleSheetNode* node : nodes)
		{
			if (node->IsApplicable(child, false))
				return child;
		}

		Element* matching_element = QuerySelectorMatchRecursive(nodes, child);
		if (matching_element)
			return matching_element;
	}

	return nullptr;
}

static void QuerySelectorAllMatchRecursive(ElementList& matching_elements, const StyleSheetNodeListRaw& nodes, Element* element)
{
	const int num_children = element->GetNumChildren();

	for (int i = 0; i < num_children; i++)
	{
		Element* child = element->GetChild(i);

		for (const StyleSheetNode* node : nodes)
		{
			if (node->IsApplicable(child, false))
			{
				matching_elements.push_back(child);
				break;
			}
		}

		QuerySelectorAllMatchRecursive(matching_elements, nodes, child);
	}
}

Element* Element::QuerySelector(const String& selectors)
{
	StyleSheetNode root_node;
	StyleSheetNodeListRaw leaf_nodes = StyleSheetParser::ConstructNodes(root_node, selectors);

	if (leaf_nodes.empty())
	{
		Log::Message(Log::LT_WARNING, "Query selector '%s' is empty. In element %s", selectors.c_str(), GetAddress().c_str());
		return nullptr;
	}

	return QuerySelectorMatchRecursive(leaf_nodes, this);
}

void Element::QuerySelectorAll(ElementList& elements, const String& selectors)
{
	StyleSheetNode root_node;
	StyleSheetNodeListRaw leaf_nodes = StyleSheetParser::ConstructNodes(root_node, selectors);

	if (leaf_nodes.empty())
	{
		Log::Message(Log::LT_WARNING, "Query selector '%s' is empty. In element %s", selectors.c_str(), GetAddress().c_str());
		return;
	}

	QuerySelectorAllMatchRecursive(elements, leaf_nodes, this);
}

// Access the event dispatcher
EventDispatcher* Element::GetEventDispatcher() const
{
	return &meta->event_dispatcher;
}

String Element::GetEventDispatcherSummary() const
{
	return meta->event_dispatcher.ToString();
}

// Access the element decorators
ElementDecoration* Element::GetElementDecoration() const
{
	return &meta->decoration;
}

// Returns the element's scrollbar functionality.
ElementScroll* Element::GetElementScroll() const
{
	return &meta->scroll;
}

DataModel* Element::GetDataModel() const
{
	return data_model;
}
	
int Element::GetClippingIgnoreDepth()
{
	return GetComputedValues().clip.number;
}
	
bool Element::IsClippingEnabled()
{
	const auto& computed = GetComputedValues();
	return computed.overflow_x != Style::Overflow::Visible || computed.overflow_y != Style::Overflow::Visible;
}

// Gets the render interface owned by this element's context.
RenderInterface* Element::GetRenderInterface()
{
	if (Context* context = GetContext())
		return context->GetRenderInterface();

	return ::Rml::GetRenderInterface();
}

void Element::SetInstancer(ElementInstancer* _instancer)
{
	// Only record the first instancer being set as some instancers call other instancers to do their dirty work, in
	// which case we don't want to update the lowest level instancer.
	if (!instancer)
	{
		instancer = _instancer;
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

void Element::OnDpRatioChange()
{
}

void Element::OnStyleSheetChange()
{
}

// Called when attributes on the element are changed.
void Element::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	for (const auto& element_attribute : changed_attributes)
	{
		const auto& attribute = element_attribute.first;
		const auto& value = element_attribute.second;
		if (attribute == "id")
		{
			id = value.Get<String>();
			meta->style.DirtyDefinition();
		}
		else if (attribute == "class")
		{
			meta->style.SetClassNames(value.Get<String>());
		}
		else if (((attribute == "colspan" || attribute == "rowspan") && meta->computed_values.display == Style::Display::TableCell)
			|| (attribute == "span" && (meta->computed_values.display == Style::Display::TableColumn || meta->computed_values.display == Style::Display::TableColumnGroup)))
		{
			DirtyLayout();
		}
		else if (attribute.size() > 2 && attribute[0] == 'o' && attribute[1] == 'n')
		{
			static constexpr bool IN_CAPTURE_PHASE = false;

			auto& attribute_event_listeners = meta->attribute_event_listeners;
			auto& event_dispatcher = meta->event_dispatcher;
			const auto event_id = EventSpecificationInterface::GetIdOrInsert(attribute.substr(2));
			const auto remove_event_listener_if_exists = [&attribute_event_listeners, &event_dispatcher, event_id]()
			{
				const auto listener_it = attribute_event_listeners.find(event_id);
				if (listener_it != attribute_event_listeners.cend())
				{
					event_dispatcher.DetachEvent(event_id, listener_it->second, IN_CAPTURE_PHASE);
					attribute_event_listeners.erase(listener_it);
				}
			};

			if (value.GetType() == Variant::Type::STRING)
			{
				remove_event_listener_if_exists();

				const auto value_as_string = value.Get<String>();
				auto insertion_result = attribute_event_listeners.emplace(event_id, Factory::InstanceEventListener(value_as_string, this));
				if (auto* listener = insertion_result.first->second)
					event_dispatcher.AttachEvent(event_id, listener, IN_CAPTURE_PHASE);
			}
			else if (value.GetType() == Variant::Type::NONE)
				remove_event_listener_if_exists();
		}
		else if (attribute == "style")
		{
			if (value.GetType() == Variant::STRING)
			{
				PropertyDictionary properties;
				StyleSheetParser parser;
				parser.ParseProperties(properties, value.GetReference<String>());

				for (const auto& name_value : properties.GetProperties())
					meta->style.SetProperty(name_value.first, name_value.second);
			}
			else if (value.GetType() != Variant::NONE)
				Log::Message(Log::LT_WARNING, "Invalid 'style' attribute, string type required. In element: %s", GetAddress().c_str());
		}
	}
}

// Called when properties on the element are changed.
void Element::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	RMLUI_ZoneScoped;

	if (!IsLayoutDirty())
	{
		// Force a relayout if any of the changed properties require it.
		const PropertyIdSet changed_properties_forcing_layout = (changed_properties & StyleSheetSpecification::GetRegisteredPropertiesForcingLayout());
		
		if(!changed_properties_forcing_layout.Empty())
			DirtyLayout();
	}

	const bool border_radius_changed = (
		changed_properties.Contains(PropertyId::BorderTopLeftRadius) ||
		changed_properties.Contains(PropertyId::BorderTopRightRadius) ||
		changed_properties.Contains(PropertyId::BorderBottomRightRadius) ||
		changed_properties.Contains(PropertyId::BorderBottomLeftRadius)
	);


	// Update the visibility.
	if (changed_properties.Contains(PropertyId::Visibility) ||
		changed_properties.Contains(PropertyId::Display))
	{
		bool new_visibility = (meta->computed_values.display != Style::Display::None && meta->computed_values.visibility == Style::Visibility::Visible);
			
		if (visible != new_visibility)
		{
			visible = new_visibility;

			if (parent != nullptr)
				parent->DirtyStackingContext();

			if (!visible)
				Blur();
		}

		if (changed_properties.Contains(PropertyId::Display))
		{
			// Due to structural pseudo-classes, this may change the element definition in siblings and parent.
			// However, the definitions will only be changed on the next update loop which may result in jarring behavior for one @frame.
			// A possible workaround is to add the parent to a list of elements that need to be updated again.
			if (parent != nullptr)
				parent->DirtyStructure();
		}
	}

	// Update the position.
	if (changed_properties.Contains(PropertyId::Left) ||
		changed_properties.Contains(PropertyId::Right) ||
		changed_properties.Contains(PropertyId::Top) ||
		changed_properties.Contains(PropertyId::Bottom))
	{
		// TODO: This should happen during/after layout, as the containing box is not properly defined yet. Off-by-one @frame issue.
		UpdateOffset();
		DirtyOffset();
	}

	// Update the z-index.
	if (changed_properties.Contains(PropertyId::ZIndex))
	{
		Style::ZIndex z_index_property = meta->computed_values.z_index;

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

				if (parent != nullptr)
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
    if (border_radius_changed ||
		changed_properties.Contains(PropertyId::BackgroundColor) ||
		changed_properties.Contains(PropertyId::Opacity) ||
		changed_properties.Contains(PropertyId::ImageColor))
	{
		meta->background_border.DirtyBackground();
    }

	// Dirty the border if it's changed.
	if (border_radius_changed ||
		changed_properties.Contains(PropertyId::BorderTopWidth) ||
		changed_properties.Contains(PropertyId::BorderRightWidth) ||
		changed_properties.Contains(PropertyId::BorderBottomWidth) ||
		changed_properties.Contains(PropertyId::BorderLeftWidth) ||
		changed_properties.Contains(PropertyId::BorderTopColor) ||
		changed_properties.Contains(PropertyId::BorderRightColor) ||
		changed_properties.Contains(PropertyId::BorderBottomColor) ||
		changed_properties.Contains(PropertyId::BorderLeftColor) ||
		changed_properties.Contains(PropertyId::Opacity))
	{
		meta->background_border.DirtyBorder();
	}
	
	// Dirty the decoration if it's changed.
	if (changed_properties.Contains(PropertyId::Decorator))
	{
		meta->decoration.DirtyDecorators();
	}

	// Dirty the decoration data when its visual looks may have changed.
	if (border_radius_changed ||
		changed_properties.Contains(PropertyId::Opacity) ||
		changed_properties.Contains(PropertyId::ImageColor))
	{
		meta->decoration.DirtyDecoratorsData();
	}

	// Check for `perspective' and `perspective-origin' changes
	if (changed_properties.Contains(PropertyId::Perspective) ||
		changed_properties.Contains(PropertyId::PerspectiveOriginX) ||
		changed_properties.Contains(PropertyId::PerspectiveOriginY))
	{
		DirtyTransformState(true, false);
	}

	// Check for `transform' and `transform-origin' changes
	if (changed_properties.Contains(PropertyId::Transform) ||
		changed_properties.Contains(PropertyId::TransformOriginX) ||
		changed_properties.Contains(PropertyId::TransformOriginY) ||
		changed_properties.Contains(PropertyId::TransformOriginZ))
	{
		DirtyTransformState(false, true);
	}

	// Check for `animation' changes
	if (changed_properties.Contains(PropertyId::Animation))
	{
		dirty_animation = true;
	}
	// Check for `transition' changes
	if (changed_properties.Contains(PropertyId::Transition))
	{
		dirty_transition = true;
	}
}

void Element::OnPseudoClassChange(const String& /*pseudo_class*/, bool /*activate*/)
{
}

// Called when a child node has been added somewhere in the hierarchy
void Element::OnChildAdd(Element* /*child*/)
{
}

// Called when a child node has been removed somewhere in the hierarchy
void Element::OnChildRemove(Element* /*child*/)
{
}

// Forces a re-layout of this element, and any other children required.
void Element::DirtyLayout()
{
	Element* document = GetOwnerDocument();
	if (document != nullptr)
		document->DirtyLayout();
}

// Forces a re-layout of this element, and any other children required.
bool Element::IsLayoutDirty()
{
	Element* document = GetOwnerDocument();
	if (document != nullptr)
		return document->IsLayoutDirty();
	return false;
}

void Element::ProcessDefaultAction(Event& event)
{
	if (event == EventId::Mousedown)
	{
		const Vector2f mouse_pos(event.GetParameter("mouse_x", 0.f), event.GetParameter("mouse_y", 0.f));

		if (IsPointWithinElement(mouse_pos) && event.GetParameter("button", 0) == 0)
			SetPseudoClass("active", true);
	}

	if (event == EventId::Mousescroll)
	{
		if (GetScrollHeight() > GetClientHeight())
		{
			Style::Overflow overflow_property = meta->computed_values.overflow_y;
			if (overflow_property == Style::Overflow::Auto ||
				overflow_property == Style::Overflow::Scroll)
			{
				// Stop the propagation if the current element has scrollbars.
				// This prevents scrolling in parent elements, which is often unintended. If instead desired behavior is
				// to scroll in parent elements when reaching top/bottom, move StopPropagation inside the next if statement.
				event.StopPropagation();

				const float wheel_delta = event.GetParameter< float >("wheel_delta", 0.f);

				if ((wheel_delta < 0 && GetScrollTop() > 0) ||
					(wheel_delta > 0 && GetScrollHeight() > GetScrollTop() + GetClientHeight()))
				{
					// Defined as three times the default line-height, multiplied by the dp ratio.
					float default_scroll_length = 3.f * DefaultComputedValues.line_height.value;
					if (const Context* context = GetContext())
						default_scroll_length *= context->GetDensityIndependentPixelRatio();

					SetScrollTop(GetScrollTop() + wheel_delta * default_scroll_length);
				}
			}
		}

		return;
	}

	if (event.GetPhase() == EventPhase::Target)
	{
		switch (event.GetId())
		{
		case EventId::Mouseover:
			SetPseudoClass("hover", true);
			break;
		case EventId::Mouseout:
			SetPseudoClass("hover", false);
			break;
		case EventId::Focus:
			SetPseudoClass("focus", true);
			break;
		case EventId::Blur:
			SetPseudoClass("focus", false);
			break;
		default:
			break;
		}
	}
}

const Style::ComputedValues& Element::GetComputedValues() const
{
	return meta->computed_values;
}

void Element::GetRML(String& content)
{
	// First we start the open tag, add the attributes then close the open tag.
	// Then comes the children in order, then we add our close tag.
	content += "<";
	content += tag;

	for (auto& pair : attributes)
	{
		auto& name = pair.first;
		auto& variant = pair.second;
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

void Element::SetOwnerDocument(ElementDocument* document)
{
	if (owner_document && !document)
	{
		// We are detaching from the document and thereby also the context.
		if (Context* context = owner_document->GetContext())
			context->OnElementDetach(this);
	}

	// If this element is a document, then never change owner_document.
	if (owner_document != this && owner_document != document)
	{
		owner_document = document;
		for (ElementPtr& child : children)
			child->SetOwnerDocument(document);
	}
}

void Element::SetDataModel(DataModel* new_data_model) 
{
	RMLUI_ASSERTMSG(!data_model || !new_data_model, "We must either attach a new data model, or detach the old one.");

	if (data_model == new_data_model)
		return;

	if (data_model)
		data_model->OnElementRemove(this);

	data_model = new_data_model;

	if (data_model)
		ElementUtilities::ApplyDataViewsControllers(this);

	for (ElementPtr& child : children)
		child->SetDataModel(new_data_model);
}

void Element::Release()
{
	if (instancer)
		instancer->ReleaseElement(this);
	else
		Log::Message(Log::LT_WARNING, "Leak detected: element %s not instanced via RmlUi Factory. Unable to release.", GetAddress().c_str());
}

void Element::SetParent(Element* _parent)
{	
	// Assumes we are already detached from the hierarchy or we are detaching now.
	RMLUI_ASSERT(!parent || !_parent);

	parent = _parent;

	if (parent)
	{
		// We need to update our definition and make sure we inherit the properties of our new parent.
		meta->style.DirtyDefinition();
		meta->style.DirtyInheritedProperties();
	}

	// The transform state may require recalculation.
	if (transform_state || (parent && parent->transform_state))
		DirtyTransformState(true, true);

	SetOwnerDocument(parent ? parent->GetOwnerDocument() : nullptr);

	if (!parent)
	{
		if (data_model)
			SetDataModel(nullptr);
	}
	else 
	{
		auto it = attributes.find("data-model");
		if (it == attributes.end())
		{
			SetDataModel(parent->data_model);
		}
		else if (parent->data_model)
		{
			String name = it->second.Get<String>();
			Log::Message(Log::LT_ERROR, "Nested data models are not allowed. Data model '%s' given in element %s.", name.c_str(), GetAddress().c_str());
		}
		else if (Context* context = GetContext())
		{
			String name = it->second.Get<String>();
			if (DataModel* model = context->GetDataModelPtr(name))
			{
				model->AttachModelRootElement(this);
				SetDataModel(model);
			}
			else
				Log::Message(Log::LT_ERROR, "Could not locate data model '%s' in element %s.", name.c_str(), GetAddress().c_str());
		}
	}
}

void Element::DirtyOffset()
{
	if(!offset_dirty)
	{
		offset_dirty = true;

		if(transform_state)
			DirtyTransformState(true, true);

		// Not strictly true ... ?
		for (size_t i = 0; i < children.size(); i++)
			children[i]->DirtyOffset();
	}
}

void Element::UpdateOffset()
{
	using namespace Style;
	const auto& computed = meta->computed_values;
	Position position_property = computed.position;

	if (position_property == Position::Absolute ||
		position_property == Position::Fixed)
	{
		if (offset_parent != nullptr)
		{
			const Box& parent_box = offset_parent->GetBox();
			Vector2f containing_block = parent_box.GetSize(Box::PADDING);

			// If the element is anchored left, then the position is offset by that resolved value.
			if (computed.left.type != Left::Auto)
				relative_offset_base.x = parent_box.GetEdge(Box::BORDER, Box::LEFT) + (ResolveValue(computed.left, containing_block.x) + GetBox().GetEdge(Box::MARGIN, Box::LEFT));

			// If the element is anchored right, then the position is set first so the element's right-most edge
			// (including margins) will render up against the containing box's right-most content edge, and then
			// offset by the resolved value.
			else if (computed.right.type != Right::Auto)
				relative_offset_base.x = containing_block.x + parent_box.GetEdge(Box::BORDER, Box::LEFT) - (ResolveValue(computed.right, containing_block.x) + GetBox().GetSize(Box::BORDER).x + GetBox().GetEdge(Box::MARGIN, Box::RIGHT));

			// If the element is anchored top, then the position is offset by that resolved value.
			if (computed.top.type != Top::Auto)
				relative_offset_base.y = parent_box.GetEdge(Box::BORDER, Box::TOP) + (ResolveValue(computed.top, containing_block.y) + GetBox().GetEdge(Box::MARGIN, Box::TOP));

			// If the element is anchored bottom, then the position is set first so the element's right-most edge
			// (including margins) will render up against the containing box's right-most content edge, and then
			// offset by the resolved value.
			else if (computed.bottom.type != Bottom::Auto)
				relative_offset_base.y = containing_block.y + parent_box.GetEdge(Box::BORDER, Box::TOP) - (ResolveValue(computed.bottom, containing_block.y) + GetBox().GetSize(Box::BORDER).y + GetBox().GetEdge(Box::MARGIN, Box::BOTTOM));
		}
	}
	else if (position_property == Position::Relative)
	{
		if (offset_parent != nullptr)
		{
			const Box& parent_box = offset_parent->GetBox();
			Vector2f containing_block = parent_box.GetSize();

			if (computed.left.type != Left::Auto)
				relative_offset_position.x = ResolveValue(computed.left, containing_block.x);
			else if (computed.right.type != Right::Auto)
				relative_offset_position.x = -1 * ResolveValue(computed.right, containing_block.x);
			else
				relative_offset_position.x = 0;

			if (computed.top.type != Top::Auto)
				relative_offset_position.y = ResolveValue(computed.top, containing_block.y);
			else if (computed.bottom.type != Bottom::Auto)
				relative_offset_position.y = -1 * ResolveValue(computed.bottom, containing_block.y);
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

void Element::SetBaseline(float in_baseline)
{
	baseline = in_baseline;
}

void Element::BuildLocalStackingContext()
{
	stacking_context_dirty = false;
	stacking_context.clear();

	BuildStackingContext(&stacking_context);
	std::stable_sort(stacking_context.begin(), stacking_context.end(), [](const Element* lhs, const Element* rhs) { return lhs->GetZIndex() < rhs->GetZIndex(); });
}

enum class RenderOrder { Block, TableColumnGroup, TableColumn, TableRowGroup, TableRow, TableCell, Inline, Floating, Positioned };
struct StackingOrderedChild {
	Element* element;
	RenderOrder order;
	bool include_children;
};

void Element::BuildStackingContext(ElementList* new_stacking_context)
{
	RMLUI_ZoneScoped;

	// Build the list of ordered children. Our child list is sorted within the stacking context so stacked elements
	// will render in the right order; ie, positioned elements will render on top of inline elements, which will render
	// on top of floated elements, which will render on top of block elements.
	Vector< StackingOrderedChild > ordered_children;

	const size_t num_children = children.size();
	ordered_children.reserve(num_children);

	if (GetDisplay() == Style::Display::Table)
	{
		BuildStackingContextForTable(ordered_children, this);
	}
	else
	{
		for (size_t i = 0; i < num_children; ++i)
		{
			Element* child = children[i].get();

			if (!child->IsVisible())
				continue;

			ordered_children.emplace_back();
			StackingOrderedChild& ordered_child = ordered_children.back();

			ordered_child.element = child;
			ordered_child.order = RenderOrder::Inline;
			ordered_child.include_children = !child->local_stacking_context;

			const Style::Display child_display = child->GetDisplay();

			if (child->GetPosition() != Style::Position::Static)
				ordered_child.order = RenderOrder::Positioned;
			else if (child->GetFloat() != Style::Float::None)
				ordered_child.order = RenderOrder::Floating;
			else if (child_display == Style::Display::Block || child_display == Style::Display::Table)
				ordered_child.order = RenderOrder::Block;
			else
				ordered_child.order = RenderOrder::Inline;
		}
	}

	// Sort the list!
	std::stable_sort(ordered_children.begin(), ordered_children.end(), [](const StackingOrderedChild& lhs, const StackingOrderedChild& rhs) { return int(lhs.order) < int(rhs.order); });

	// Add the list of ordered children into the stacking context in order.
	for (size_t i = 0; i < ordered_children.size(); ++i)
	{
		new_stacking_context->push_back(ordered_children[i].element);

		if (ordered_children[i].include_children)
			ordered_children[i].element->BuildStackingContext(new_stacking_context);
	}
}

void Element::BuildStackingContextForTable(Vector<StackingOrderedChild>& ordered_children, Element* parent)
{
	const size_t num_children = parent->children.size();

	for (size_t i = 0; i < num_children; ++i)
	{
		Element* child = parent->children[i].get();

		if (!child->IsVisible())
			continue;

		ordered_children.emplace_back();
		StackingOrderedChild& ordered_child = ordered_children.back();
		ordered_child.element = child;
		ordered_child.order = RenderOrder::Inline;
		ordered_child.include_children = false;

		bool recurse_into_children = false;

		switch (child->GetDisplay())
		{
		case Style::Display::TableRow:
			ordered_child.order = RenderOrder::TableRow;
			recurse_into_children = true;
			break;
		case Style::Display::TableRowGroup:
			ordered_child.order = RenderOrder::TableRowGroup;
			recurse_into_children = true;
			break;
		case Style::Display::TableColumn:
			ordered_child.order = RenderOrder::TableColumn;
			break;
		case Style::Display::TableColumnGroup:
			ordered_child.order = RenderOrder::TableColumnGroup;
			recurse_into_children = true;
			break;
		case Style::Display::TableCell:
			ordered_child.order = RenderOrder::TableCell;
			ordered_child.include_children = !child->local_stacking_context;
			break;
		default:
			ordered_child.order = RenderOrder::Positioned;
			ordered_child.include_children = !child->local_stacking_context;
			break;
		}

		if (recurse_into_children)
			BuildStackingContextForTable(ordered_children, child);
	}
}

void Element::DirtyStackingContext()
{
	// Find the first ancestor that has a local stacking context, that is our stacking context parent.
	Element* stacking_context_parent = this;
	while (stacking_context_parent && !stacking_context_parent->local_stacking_context)
	{
		stacking_context_parent = stacking_context_parent->GetParentNode();
	}

	if (stacking_context_parent)
		stacking_context_parent->stacking_context_dirty = true;
}

void Element::DirtyStructure()
{
	structure_dirty = true;
}

void Element::UpdateStructure()
{
	if (structure_dirty)
	{
		structure_dirty = false;

		// If this element or its children depend on structured selectors, they may need to be updated.
		GetStyle()->DirtyDefinition();
	}
}


bool Element::Animate(const String & property_name, const Property & target_value, float duration, Tween tween, int num_iterations, bool alternate_direction, float delay, const Property* start_value)
{
	bool result = false;
	PropertyId property_id = StyleSheetSpecification::GetPropertyId(property_name);

	auto it_animation = StartAnimation(property_id, start_value, num_iterations, alternate_direction, delay, false);
	if (it_animation != animations.end())
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

	PropertyId property_id = StyleSheetSpecification::GetPropertyId(property_name);

	for (auto& existing_animation : animations) {
		if (existing_animation.GetPropertyId() == property_id) {
			animation = &existing_animation;
			break;
		}
	}
	if (!animation)
		return false;

	bool result = animation->AddKey(animation->GetDuration() + duration, target_value, *this, tween, true);

	return result;
}


ElementAnimationList::iterator Element::StartAnimation(PropertyId property_id, const Property* start_value, int num_iterations, bool alternate_direction, float delay, bool initiated_by_animation_property)
{
	auto it = std::find_if(animations.begin(), animations.end(), [&](const ElementAnimation& el) { return el.GetPropertyId() == property_id; });

	if (it != animations.end())
	{
		*it = ElementAnimation{};
	}
	else
	{
		animations.emplace_back();
		it = animations.end() - 1;
	}

	Property value;

	if (start_value)
	{
		value = *start_value;
		if (!value.definition)
			if(auto default_value = GetProperty(property_id))
				value.definition = default_value->definition;	
	}
	else if (auto default_value = GetProperty(property_id))
	{
		value = *default_value;
	}

	if (value.definition)
	{
		ElementAnimationOrigin origin = (initiated_by_animation_property ? ElementAnimationOrigin::Animation : ElementAnimationOrigin::User);
		double start_time = Clock::GetElapsedTime() + (double)delay;
		*it = ElementAnimation{ property_id, origin, value, *this, start_time, 0.0f, num_iterations, alternate_direction };
	}
	
	if(!it->IsInitalized())
	{
		animations.erase(it);
		it = animations.end();
	}

	return it;
}


bool Element::AddAnimationKeyTime(PropertyId property_id, const Property* target_value, float time, Tween tween)
{
	if (!target_value)
		target_value = meta->style.GetProperty(property_id);
	if (!target_value)
		return false;

	ElementAnimation* animation = nullptr;

	for (auto& existing_animation : animations) {
		if (existing_animation.GetPropertyId() == property_id) {
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
	auto it = std::find_if(animations.begin(), animations.end(), [&](const ElementAnimation& el) { return el.GetPropertyId() == transition.id; });

	if (it != animations.end() && !it->IsTransition())
		return false;

	float duration = transition.duration;
	double start_time = Clock::GetElapsedTime() + (double)transition.delay;

	if (it == animations.end())
	{
		// Add transition as new animation
		animations.push_back(
			ElementAnimation{ transition.id, ElementAnimationOrigin::Transition, start_value, *this, start_time, 0.0f, 1, false }
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
		*it = ElementAnimation{ transition.id, ElementAnimationOrigin::Transition, start_value, *this, start_time, 0.0f, 1, false };
	}

	bool result = it->AddKey(duration, target_value, *this, transition.tween, true);

	if (result)
		SetProperty(transition.id, start_value);
	else
		animations.erase(it);

	return result;
}

void Element::HandleTransitionProperty()
{
	if(dirty_transition)
	{
		dirty_transition = false;

		// Remove all transitions that are no longer in our local list
		const TransitionList& keep_transitions = GetComputedValues().transition;

		if (keep_transitions.all)
			return;

		auto it_remove = animations.end();

		if (keep_transitions.none)
		{
			// All transitions should be removed, but only touch the animations that originate from the 'transition' property.
			// Move all animations to be erased in a valid state at the end of the list, and erase later.
			it_remove = std::partition(animations.begin(), animations.end(),
				[](const ElementAnimation& animation) -> bool { return !animation.IsTransition(); }
			);
		}
		else
		{
			// Only remove the transitions that are not in our keep list.
			const auto& keep_transitions_list = keep_transitions.transitions;

			it_remove = std::partition(animations.begin(), animations.end(),
				[&keep_transitions_list](const ElementAnimation& animation) -> bool {
					if (!animation.IsTransition())
						return true;
					auto it = std::find_if(keep_transitions_list.begin(), keep_transitions_list.end(),
						[&animation](const Transition& transition) { return animation.GetPropertyId() == transition.id; }
					);
					bool keep_animation = (it != keep_transitions_list.end());
					return keep_animation;
				}
			);
		}

		// We can decide what to do with cancelled transitions here.
		for (auto it = it_remove; it != animations.end(); ++it)
			RemoveProperty(it->GetPropertyId());

		animations.erase(it_remove, animations.end());
	}
}

void Element::HandleAnimationProperty()
{
	// Note: We are effectively restarting all animations whenever 'dirty_animation' is set. Use the dirty flag with care,
	// or find another approach which only updates actual "dirty" animations.
	if (dirty_animation)
	{
		dirty_animation = false;

		const AnimationList& animation_list = meta->computed_values.animation;
		bool element_has_animations = (!animation_list.empty() || !animations.empty());
		const StyleSheet* stylesheet = nullptr;

		if (element_has_animations)
			stylesheet = GetStyleSheet();

		if (stylesheet)
		{
			// Remove existing animations
			{
				// We only touch the animations that originate from the 'animation' property.
				auto it_remove = std::partition(animations.begin(), animations.end(), 
					[](const ElementAnimation & animation) { return animation.GetOrigin() != ElementAnimationOrigin::Animation; }
				);

				// We can decide what to do with cancelled animations here.
				for (auto it = it_remove; it != animations.end(); ++it)
					RemoveProperty(it->GetPropertyId());

				animations.erase(it_remove, animations.end());
			}

			// Start animations
			for (const auto& animation : animation_list)
			{
				const Keyframes* keyframes_ptr = stylesheet->GetKeyframes(animation.name);
				if (keyframes_ptr && keyframes_ptr->blocks.size() >= 1 && !animation.paused)
				{
					auto& property_ids = keyframes_ptr->property_ids;
					auto& blocks = keyframes_ptr->blocks;

					bool has_from_key = (blocks[0].normalized_time == 0);
					bool has_to_key = (blocks.back().normalized_time == 1);

					// If the first key defines initial conditions for a given property, use those values, else, use this element's current values.
					for (PropertyId id : property_ids)
						StartAnimation(id, (has_from_key ? blocks[0].properties.GetProperty(id) : nullptr), animation.num_iterations, animation.alternate, animation.delay, true);

					// Add middle keys: Need to skip the first and last keys if they set the initial and end conditions, respectively.
					for (int i = (has_from_key ? 1 : 0); i < (int)blocks.size() + (has_to_key ? -1 : 0); i++)
					{
						// Add properties of current key to animation
						float time = blocks[i].normalized_time * animation.duration;
						for (auto& property : blocks[i].properties.GetProperties())
							AddAnimationKeyTime(property.first, &property.second, time, animation.tween);
					}

					// If the last key defines end conditions for a given property, use those values, else, use this element's current values.
					float time = animation.duration;
					for (PropertyId id : property_ids)
						AddAnimationKeyTime(id, (has_to_key ? blocks.back().properties.GetProperty(id) : nullptr), time, animation.tween);
				}
			}
		}
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
				SetProperty(animation.GetPropertyId(), property);
		}

		// Move all completed animations to the end of the list
		auto it_completed = std::partition(animations.begin(), animations.end(), [](const ElementAnimation& animation) { return !animation.IsComplete(); });

		Vector<Dictionary> dictionary_list;
		Vector<bool> is_transition;
		dictionary_list.reserve(animations.end() - it_completed);
		is_transition.reserve(animations.end() - it_completed);

		for (auto it = it_completed; it != animations.end(); ++it)
		{
			const String& property_name = StyleSheetSpecification::GetPropertyName(it->GetPropertyId());

			dictionary_list.emplace_back();
			dictionary_list.back().emplace("property", Variant(property_name));
			is_transition.push_back(it->IsTransition());

			// Remove completed transition- and animation-initiated properties.
			// Should behave like in HandleTransitionProperty() and HandleAnimationProperty() respectively.
			if (it->GetOrigin() != ElementAnimationOrigin::User)
				RemoveProperty(it->GetPropertyId());
		}

		// Need to erase elements before submitting event, as iterators might be invalidated when calling external code.
		animations.erase(it_completed, animations.end());

		for (size_t i = 0; i < dictionary_list.size(); i++)
			DispatchEvent(is_transition[i] ? EventId::Transitionend : EventId::Animationend, dictionary_list[i]);
	}
}



void Element::DirtyTransformState(bool perspective_dirty, bool transform_dirty)
{
	dirty_perspective |= perspective_dirty;
	dirty_transform |= transform_dirty;
}


void Element::UpdateTransformState()
{
	if (!dirty_perspective && !dirty_transform)
		return;

	const ComputedValues& computed = meta->computed_values;

	const Vector2f pos = GetAbsoluteOffset(Box::BORDER);
	const Vector2f size = GetBox().GetSize(Box::BORDER);
	
	bool perspective_or_transform_changed = false;

	if (dirty_perspective)
	{
		// If perspective is set on this element, then it applies to our children. We just calculate it here, 
		// and let the children's transform update merge it with their transform.
		bool had_perspective = (transform_state && transform_state->GetLocalPerspective());

		float distance = computed.perspective;
		Vector2f vanish = Vector2f(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
		bool have_perspective = false;

		if (distance > 0.0f)
		{
			have_perspective = true;

			// Compute the vanishing point from the perspective origin
			if (computed.perspective_origin_x.type == Style::PerspectiveOrigin::Percentage)
				vanish.x = pos.x + computed.perspective_origin_x.value * 0.01f * size.x;
			else
				vanish.x = pos.x + computed.perspective_origin_x.value;

			if (computed.perspective_origin_y.type == Style::PerspectiveOrigin::Percentage)
				vanish.y = pos.y + computed.perspective_origin_y.value * 0.01f * size.y;
			else
				vanish.y = pos.y + computed.perspective_origin_y.value;
		}

		if (have_perspective)
		{
			// Equivalent to: Translate(x,y,0) * Perspective(distance) * Translate(-x,-y,0)
			Matrix4f perspective = Matrix4f::FromRows(
				{ 1, 0, -vanish.x / distance, 0 },
				{ 0, 1, -vanish.y / distance, 0 },
				{ 0, 0, 1, 0 },
				{ 0, 0, -1 / distance, 1 }
			);

			if (!transform_state)
				transform_state = MakeUnique<TransformState>();

			perspective_or_transform_changed |= transform_state->SetLocalPerspective(&perspective);
		}
		else if (transform_state)
			transform_state->SetLocalPerspective(nullptr);

		perspective_or_transform_changed |= (have_perspective != had_perspective);

		dirty_perspective = false;
	}


	if (dirty_transform)
	{
		// We want to find the accumulated transform given all our ancestors. It is assumed here that the parent transform is already updated,
		// so that we only need to consider our local transform and combine it with our parent's transform and perspective matrices.
		bool had_transform = (transform_state && transform_state->GetTransform());

		bool have_transform = false;
		Matrix4f transform = Matrix4f::Identity();

		if (computed.transform)
		{
			// First find the current element's transform
			const int n = computed.transform->GetNumPrimitives();
			for (int i = 0; i < n; ++i)
			{
				const TransformPrimitive& primitive = computed.transform->GetPrimitive(i);
				Matrix4f matrix = TransformUtilities::ResolveTransform(primitive, *this);
				transform *= matrix;
				have_transform = true;
			}

			if(have_transform)
			{
				// Compute the transform origin
				Vector3f transform_origin(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, 0);

				if (computed.transform_origin_x.type == Style::TransformOrigin::Percentage)
					transform_origin.x = pos.x + computed.transform_origin_x.value * size.x * 0.01f;
				else
					transform_origin.x = pos.x + computed.transform_origin_x.value;

				if (computed.transform_origin_y.type == Style::TransformOrigin::Percentage)
					transform_origin.y = pos.y + computed.transform_origin_y.value * size.y * 0.01f;
				else
					transform_origin.y = pos.y + computed.transform_origin_y.value;

				transform_origin.z = computed.transform_origin_z;

				// Make the transformation apply relative to the transform origin
				transform = Matrix4f::Translate(transform_origin) * transform * Matrix4f::Translate(-transform_origin);
			}

			// We may want to include the local offsets here, as suggested by the CSS specs, so that the local transform is applied after the offset I believe
			// the motivation is. Then we would need to subtract the absolute zero-offsets during geometry submit whenever we have transforms.
		}

		if (parent && parent->transform_state)
		{
			// Apply the parent's local perspective and transform.
			// @performance: If we have no local transform and no parent perspective, we can effectively just point to the parent transform instead of copying it.
			const TransformState& parent_state = *parent->transform_state;

			if (auto parent_perspective = parent_state.GetLocalPerspective())
			{
				transform = *parent_perspective * transform;
				have_transform = true;
			}

			if (auto parent_transform = parent_state.GetTransform())
			{
				transform = *parent_transform * transform;
				have_transform = true;
			}
		}

		if (have_transform)
		{
			if (!transform_state)
				transform_state = MakeUnique<TransformState>();

			perspective_or_transform_changed |= transform_state->SetTransform(&transform);
		}
		else if (transform_state)
			transform_state->SetTransform(nullptr);

		perspective_or_transform_changed |= (had_transform != have_transform);
	}

	// A change in perspective or transform will require an update to children transforms as well.
	if (perspective_or_transform_changed)
	{
		for (size_t i = 0; i < children.size(); i++)
			children[i]->DirtyTransformState(false, true);
	}

	// No reason to keep the transform state around if transform and perspective have been removed.
	if (transform_state && !transform_state->GetTransform() && !transform_state->GetLocalPerspective())
	{
		transform_state.reset();
	}
}

void Element::OnStyleSheetChangeRecursive()
{
	GetElementDecoration()->DirtyDecorators();

	OnStyleSheetChange();

	// Now dirty all of our descendants.
	const int num_children = GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		GetChild(i)->OnStyleSheetChangeRecursive();
}

void Element::OnDpRatioChangeRecursive()
{
	GetElementDecoration()->DirtyDecorators();
	GetStyle()->DirtyPropertiesWithUnits(Property::DP);

	OnDpRatioChange();

	// Now dirty all of our descendants.
	const int num_children = GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		GetChild(i)->OnDpRatioChangeRecursive();
}

} // namespace Rml
