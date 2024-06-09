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

#ifndef RMLUI_CORE_CONTEXT_H
#define RMLUI_CORE_CONTEXT_H

#include "Header.h"
#include "Input.h"
#include "ScriptInterface.h"
#include "ScrollTypes.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class Stream;
class ContextInstancer;
class ElementDocument;
class EventListener;
class DataModel;
class DataModelConstructor;
class DataTypeRegister;
class ScrollController;
class RenderManager;
class TextInputHandler;
enum class EventId : uint16_t;

/**
    A context for storing, rendering and processing RML documents. Multiple contexts can exist simultaneously.

    @author Peter Curry
 */

class RMLUICORE_API Context : public ScriptInterface {
public:
	/// Constructs a new, uninitialised context. This should not be called directly, use CreateContext() instead.
	/// @param[in] name The name of the context.
	/// @param[in] render_manager The render manager used for this context.
	/// @param[in] text_input_handler The text input handler used for this context.
	Context(const String& name, RenderManager* render_manager, TextInputHandler* text_input_handler);
	/// Destroys a context.
	virtual ~Context();

	/// Returns the name of the context.
	/// @return The context's name.
	const String& GetName() const;

	/// Changes the dimensions of the context.
	/// @param[in] dimensions The new dimensions of the context.
	void SetDimensions(Vector2i dimensions);
	/// Returns the dimensions of the context.
	/// @return The current dimensions of the context.
	Vector2i GetDimensions() const;

	/// Changes the size ratio of 'dp' unit to 'px' unit
	/// @param[in] dp_ratio The new density-independent pixel ratio of the context.
	void SetDensityIndependentPixelRatio(float density_independent_pixel_ratio);
	/// Returns the size ratio of 'dp' unit to 'px' unit
	/// @return The current density-independent pixel ratio of the context.
	float GetDensityIndependentPixelRatio() const;

	/// Updates all elements in the context's documents.
	/// This must be called before Context::Render, but after any elements have been changed, added or removed.
	bool Update();
	/// Renders all visible elements in the context's documents.
	bool Render();

	/// Creates a new, empty document and places it into this context.
	/// @param[in] instancer_name The name of the instancer used to create the document.
	/// @return The new document, or nullptr if no document could be created.
	ElementDocument* CreateDocument(const String& instancer_name = "body");
	/// Load a document into the context.
	/// @param[in] document_path The path to the document to load. The path is passed directly to the file interface which is used to load the file.
	/// The default file interface accepts both absolute paths and paths relative to the working directory.
	/// @return The loaded document, or nullptr if no document was loaded.
	ElementDocument* LoadDocument(const String& document_path);
	/// Load a document into the context.
	/// @param[in] document_stream The opened stream, ready to read.
	/// @return The loaded document, or nullptr if no document was loaded.
	ElementDocument* LoadDocument(Stream* document_stream);
	/// Load a document into the context.
	/// @param[in] document_rml The string containing the document RML.
	/// @param[in] source_url Optional string used to set the document's source URL, or naming the document for log messages.
	/// @return The loaded document, or nullptr if no document was loaded.
	ElementDocument* LoadDocumentFromMemory(const String& document_rml, const String& source_url = "[document from memory]");
	/// Unload the given document.
	/// @param[in] document The document to unload.
	/// @note The destruction of the document is deferred until the next call to Context::Update().
	void UnloadDocument(ElementDocument* document);
	/// Unloads all loaded documents.
	/// @note The destruction of the documents is deferred until the next call to Context::Update().
	void UnloadAllDocuments();

	/// Enable or disable handling of the mouse cursor from this context.
	/// When enabled, changes to the cursor name is transmitted through the system interface.
	/// @param[in] show True to enable mouse cursor handling, false to disable.
	void EnableMouseCursor(bool enable);

	/// Activate or deactivate a media theme. Themes can be used in RCSS media queries.
	/// @param theme_name[in] The name of the theme to (de)activate.
	/// @param activate True to activate the given theme, false to deactivate.
	void ActivateTheme(const String& theme_name, bool activate);
	/// Check if a given media theme has been activated.
	/// @param theme_name The name of the theme.
	/// @return True if the theme is activated.
	bool IsThemeActive(const String& theme_name) const;

	/// Returns the first document in the context with the given id.
	/// @param[in] id The id of the desired document.
	/// @return The document (if it was found), or nullptr if no document exists with the ID.
	ElementDocument* GetDocument(const String& id);
	/// Returns a document in the context by index.
	/// @param[in] index The index of the desired document.
	/// @return The document (if one exists with this index), or nullptr if the index was invalid.
	ElementDocument* GetDocument(int index);
	/// Returns the number of documents in the context.
	int GetNumDocuments() const;

	/// Returns the hover element.
	/// @return The element the mouse cursor is hovering over.
	Element* GetHoverElement();
	/// Returns the focus element.
	/// @return The element with input focus.
	Element* GetFocusElement();
	/// Returns the root element that holds all the documents
	/// @return The root element.
	Element* GetRootElement();

	// Returns the youngest descendent of the given element which is under the given point in screen coordinates.
	// @param[in] point The point to test.
	// @param[in] ignore_element If set, this element and its descendents will be ignored.
	// @param[in] element Used internally.
	// @return The element under the point, or nullptr if nothing is.
	Element* GetElementAtPoint(Vector2f point, const Element* ignore_element = nullptr, Element* element = nullptr) const;

	/// Brings the document to the front of the document stack.
	/// @param[in] document The document to pull to the front of the stack.
	void PullDocumentToFront(ElementDocument* document);
	/// Sends the document to the back of the document stack.
	/// @param[in] document The document to push to the bottom of the stack.
	void PushDocumentToBack(ElementDocument* document);
	/// Remove the document from the focus history and focus the previous document.
	/// @param[in] document The document to unfocus.
	void UnfocusDocument(ElementDocument* document);

	/// Adds an event listener to the context's root element.
	/// @param[in] event The name of the event to attach to.
	/// @param[in] listener Listener object to be attached.
	/// @param[in] in_capture_phase True if the listener is to be attached to the capture phase, false for the bubble phase.
	void AddEventListener(const String& event, EventListener* listener, bool in_capture_phase = false);
	/// Removes an event listener from the context's root element.
	/// @param[in] event The name of the event to detach from.
	/// @param[in] listener Listener object to be detached.
	/// @param[in] in_capture_phase True to detach from the capture phase, false from the bubble phase.
	void RemoveEventListener(const String& event, EventListener* listener, bool in_capture_phase = false);

	/// Sends a key down event into this context.
	/// @param[in] key_identifier The key pressed.
	/// @param[in] key_modifier_state The state of key modifiers (shift, control, caps-lock, etc) keys; this should be generated by ORing together
	/// members of the Input::KeyModifier enumeration.
	/// @return True if the event was not consumed (ie, was prevented from propagating by an element), false if it was.
	bool ProcessKeyDown(Input::KeyIdentifier key_identifier, int key_modifier_state);
	/// Sends a key up event into this context.
	/// @param[in] key_identifier The key released.
	/// @param[in] key_modifier_state The state of key modifiers (shift, control, caps-lock, etc) keys; this should be generated by ORing together
	/// members of the Input::KeyModifier enumeration.
	/// @return True if the event was not consumed (ie, was prevented from propagating by an element), false if it was.
	bool ProcessKeyUp(Input::KeyIdentifier key_identifier, int key_modifier_state);

	/// Sends a single unicode character as text input into this context.
	/// @param[in] character The unicode code point to send into this context.
	/// @return True if the event was not consumed (ie, was prevented from propagating by an element), false if it was.
	bool ProcessTextInput(Character character);
	/// Sends a single ascii character as text input into this context.
	bool ProcessTextInput(char character);
	/// Sends a string of text as text input into this context.
	/// @param[in] string The UTF-8 string to send into this context.
	/// @return True if the event was not consumed (ie, was prevented from propagating by an element), false if it was.
	bool ProcessTextInput(const String& string);

	/// Sends a mouse movement event into this context.
	/// @param[in] x The x-coordinate of the mouse cursor, in window-coordinates (ie, 0 should be the left of the client area).
	/// @param[in] y The y-coordinate of the mouse cursor, in window-coordinates (ie, 0 should be the top of the client area).
	/// @param[in] key_modifier_state The state of key modifiers (shift, control, caps-lock, etc) keys; this should be generated by ORing together
	/// members of the Input::KeyModifier enumeration.
	/// @return True if the mouse is not interacting with any elements in the context (see 'IsMouseInteracting'), otherwise false.
	bool ProcessMouseMove(int x, int y, int key_modifier_state);
	/// Sends a mouse-button down event into this context.
	/// @param[in] button_index The index of the button that was pressed; 0 for the left button, 1 for right, and 2 for middle button.
	/// @param[in] key_modifier_state The state of key modifiers (shift, control, caps-lock, etc) keys; this should be generated by ORing together
	/// members of the Input::KeyModifier enumeration.
	/// @return True if the mouse is not interacting with any elements in the context (see 'IsMouseInteracting'), otherwise false.
	bool ProcessMouseButtonDown(int button_index, int key_modifier_state);
	/// Sends a mouse-button up event into this context.
	/// @param[in] button_index The index of the button that was release; 0 for the left button, 1 for right, and 2 for middle button.
	/// @param[in] key_modifier_state The state of key modifiers (shift, control, caps-lock, etc) keys; this should be generated by ORing together
	/// members of the Input::KeyModifier enumeration.
	/// @return True if the mouse is not interacting with any elements in the context (see 'IsMouseInteracting'), otherwise false.
	bool ProcessMouseButtonUp(int button_index, int key_modifier_state);
	/// Sends a mousescroll event into this context.
	/// @deprecated Please use the Vector2f version of this function.
	bool ProcessMouseWheel(float wheel_delta, int key_modifier_state);
	/// Sends a mousescroll event into this context, and scrolls the document unless the event was stopped from propagating.
	/// @param[in] wheel_delta The mouse-wheel movement this frame, with positive values being directed right and down.
	/// @param[in] key_modifier_state The state of key modifiers (shift, control, caps-lock, etc) keys; this should be generated by ORing together
	/// members of the Input::KeyModifier enumeration.
	/// @return True if the event was not consumed (ie, was prevented from propagating by an element), false if it was.
	bool ProcessMouseWheel(Vector2f wheel_delta, int key_modifier_state);
	/// Tells the context the mouse has left the window. This removes any hover state from all elements and prevents 'Update()' from setting the hover
	/// state for elements under the mouse.
	/// @return True if the mouse is not interacting with any elements in the context (see 'IsMouseInteracting'), otherwise false.
	/// @note The mouse is considered activate again after the next call to 'ProcessMouseMove()'.
	bool ProcessMouseLeave();

	/// Returns a hint on whether the mouse is currently interacting with any elements in this context, based on previously submitted
	/// 'ProcessMouse...()' commands.
	/// @note Interaction is determined irrespective of background and opacity. See the RCSS property 'pointer-events' to disable interaction for
	/// specific elements.
	/// @return True if the mouse hovers over or has activated an element in this context, otherwise false.
	bool IsMouseInteracting() const;

	/// Sets the default scroll behavior, such as for mouse wheel processing and scrollbar interaction.
	/// @param[in] scroll_behavior The default smooth scroll behavior, set to instant to disable smooth scrolling.
	/// @param[in] speed_factor A factor for adjusting the final smooth scrolling speed, must be strictly positive, defaults to 1.0.
	void SetDefaultScrollBehavior(ScrollBehavior scroll_behavior, float speed_factor);

	/// Retrieves the render manager which can be used to submit changes to the render state.
	RenderManager& GetRenderManager();

	/// Obtains the text input handler.
	TextInputHandler* GetTextInputHandler() const;

	/// Sets the instancer to use for releasing this object.
	/// @param[in] instancer The context's instancer.
	void SetInstancer(ContextInstancer* instancer);

	/// Creates a data model.
	/// The returned constructor can be used to bind data variables. Elements can bind to the model using the attribute 'data-model="name"'.
	/// @param[in] name The name of the data model.
	/// @param[in] data_type_register The data type register to use for the data model, or null to use the default register.
	/// @return A constructor for the data model, or empty if it could not be created.
	DataModelConstructor CreateDataModel(const String& name, DataTypeRegister* data_type_register = nullptr);
	/// Retrieves the constructor for an existing data model.
	/// The returned constructor can be used to add additional bindings to an existing model.
	/// @param[in] name The name of the data model.
	/// @return A constructor for the data model, or empty if it could not be found.
	DataModelConstructor GetDataModel(const String& name);
	/// Removes the given data model.
	/// This also removes all data views, controllers and bindings contained by the data model.
	/// @warning Invalidates all handles and constructors pointing to the data model.
	/// @param[in] name The name of the data model.
	/// @return True if succesfully removed, false if no data model was found.
	bool RemoveDataModel(const String& name);

	/// This will set the documents base <tag> before creation. Default = "body"
	/// @param[in] tag The name of the base tag. Example: "html"
	void SetDocumentsBaseTag(const String& tag);
	/// Gets the name of the documents base tag.
	/// @return The current documents base tag name.
	const String& GetDocumentsBaseTag();

	/// Updates the time until Update should get called again. This can be used by elements
	/// and the app to implement on demand rendering and thus drastically save CPU/GPU and
	/// reduce power consumption during inactivity. The context stores the lowest requested
	/// timestamp, which can later retrieved using GetNextUpdateDelay().
	/// @param[in] delay Maximum time until next update
	void RequestNextUpdate(double delay);

	/// Get the max delay until update and render should get called again. An application can choose
	/// to only call update and render once the time has elapsed, but theres no harm in doing so
	/// more often. The returned value can be infinity, in which case update should be invoked after
	/// user input was received. A value of 0 means "render as fast as possible", for example if
	/// an animation is playing.
	/// @return Time until next update is expected.
	double GetNextUpdateDelay() const;

protected:
	void Release() override;

private:
	String name;
	Vector2i dimensions;
	float density_independent_pixel_ratio = 1.f;
	String documents_base_tag = "body";

	// Wrapper around the render interface for tracking the render state.
	RenderManager* render_manager;

	SmallUnorderedSet<String> active_themes;

	ContextInstancer* instancer;

	using ElementSet = SmallOrderedSet<Element*>;
	using ElementList = Vector<Element*>;
	// Set of elements that are currently in hover state.
	ElementSet hover_chain;
	// List of elements that are currently in active state.
	ElementList active_chain;
	// History of windows that have had focus
	ElementList document_focus_history;

	// Documents that have been unloaded from the context but not yet released.
	OwnedElementList unloaded_documents;

	// Root of the element tree.
	ElementPtr root;
	// The element that currently has input focus.
	Element* focus;
	// The top-most element being hovered over.
	Element* hover;
	// The element that was being hovered over when the primary mouse button was pressed most recently.
	Element* active;

	// The element that was clicked on last.
	Element* last_click_element;
	// The time the last click occurred.
	double last_click_time;
	// Mouse position during the last mouse_down event.
	Vector2i last_click_mouse_position;

	// Input state; stored from the most recent input events we receive from the application.
	Vector2i mouse_position;
	bool mouse_active;

	// Controller for various scroll behavior modes.
	UniquePtr<ScrollController> scroll_controller; // [not-null]

	// Enables cursor handling.
	bool enable_cursor;
	String cursor_name;
	// Document attached to cursor (e.g. while dragging).
	ElementPtr cursor_proxy;

	// The element that is currently being dragged (or about to be dragged).
	Element* drag;
	// True if a drag has begun (ie, the ondragstart event has been fired for the drag element), false otherwise.
	bool drag_started;
	// True if the current drag is a verbose drag (ie, sends ondragover, ondragout, ondragdrop, etc, events).
	bool drag_verbose;
	// Used when dragging a cloned object.
	Element* drag_clone;

	// The element currently being dragged over; this is equivalent to hover, but only set while an element is being
	// dragged, and excludes the dragged element.
	Element* drag_hover;
	// Set of elements that are currently being dragged over; this differs from the hover state as the dragged element
	// itself can't be part of it.
	ElementSet drag_hover_chain;

	using DataModels = UnorderedMap<String, UniquePtr<DataModel>>;
	DataModels data_models;

	UniquePtr<DataTypeRegister> default_data_type_register;

	TextInputHandler* text_input_handler;

	// Time in seconds until Update and Render should be called again. This allows applications to only redraw the ui if needed.
	// See RequestNextUpdate() and NextUpdateRequested() for details.
	double next_update_timeout = 0;

	// Internal callback for when an element is detached or removed from the hierarchy.
	void OnElementDetach(Element* element);
	// Internal callback for when a new element gains focus.
	bool OnFocusChange(Element* element, bool focus_visible);

	// Generates an event for faking clicks on an element.
	void GenerateClickEvent(Element* element);

	// Updates the current hover elements, sending required events.
	void UpdateHoverChain(Vector2i old_mouse_position, int key_modifier_state = 0, Dictionary* out_parameters = nullptr,
		Dictionary* out_drag_parameters = nullptr);

	// Creates the drag clone from the given element. The old drag clone will be released if necessary.
	void CreateDragClone(Element* element);
	// Releases the drag clone, if one exists.
	void ReleaseDragClone();

	// Scroll the target by the given amount, using smooth scrolling.
	void PerformSmoothscrollOnTarget(Element* target, Vector2f delta_offset, ScrollBehavior scroll_behavior);

	// Returns the data model with the provided name, or nullptr if it does not exist.
	DataModel* GetDataModelPtr(const String& name) const;

	// Builds the parameters for a generic key event.
	void GenerateKeyEventParameters(Dictionary& parameters, Input::KeyIdentifier key_identifier);
	// Builds the parameters for a generic mouse event.
	void GenerateMouseEventParameters(Dictionary& parameters, int button_index = -1);
	// Builds the parameters for the key modifier state.
	void GenerateKeyModifierEventParameters(Dictionary& parameters, int key_modifier_state);
	// Builds the parameters for a drag event.
	void GenerateDragEventParameters(Dictionary& parameters);

	// Releases all unloaded documents pending destruction.
	void ReleaseUnloadedDocuments();

	// Sends the specified event to all elements in new_items that don't appear in old_items.
	static void SendEvents(const ElementSet& old_items, const ElementSet& new_items, EventId id, const Dictionary& parameters);

	friend class Rml::Element;
};

} // namespace Rml
#endif
