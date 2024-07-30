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

#ifndef RMLUI_CORE_ELEMENTDOCUMENT_H
#define RMLUI_CORE_ELEMENTDOCUMENT_H

#include "Element.h"

namespace Rml {

class Context;
class Stream;
class DocumentHeader;
class ElementText;
class StyleSheet;
class StyleSheetContainer;
enum class NavigationSearchDirection;

/** ModalFlag controls the modal state of the document. */
enum class ModalFlag {
	None,  // Remove modal state.
	Modal, // Set modal state, other documents cannot receive focus.
	Keep,  // Modal state unchanged.
};
/** FocusFlag controls the focus when showing the document. */
enum class FocusFlag {
	None,     // No focus.
	Document, // Focus the document.
	Keep,     // Focus the element in the document which last had focus.
	Auto,     // Focus the first tab element with the 'autofocus' attribute or else the document.
};

/**
    Represents a document in the dom tree.

    @author Lloyd Weehuizen
 */

class RMLUICORE_API ElementDocument : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementDocument, Element)

	ElementDocument(const String& tag);
	virtual ~ElementDocument();

	/// Process given document header
	void ProcessHeader(const DocumentHeader* header);

	/// Returns the document's context.
	Context* GetContext();

	/// Sets the document's title.
	void SetTitle(const String& title);
	/// Returns the title of this document.
	const String& GetTitle() const;

	/// Returns the source address of this document.
	const String& GetSourceURL() const;

	/// Returns the document's compiled style sheet.
	/// @note The style sheet may be regenerated when media query parameters change, invalidating the pointer.
	const StyleSheet* GetStyleSheet() const override;
	/// Reload the document's style sheet from source files.
	/// Styles will be reloaded from <style> tags and external style sheets, but not inline 'style' attributes.
	/// @note The source url originally used to load the document must still be a valid RML document.
	void ReloadStyleSheet();

	/// Returns the document's style sheet container.
	const StyleSheetContainer* GetStyleSheetContainer() const;
	/// Sets the style sheet this document, and all of its children, uses.
	void SetStyleSheetContainer(SharedPtr<StyleSheetContainer> style_sheet);

	/// Brings the document to the front of the document stack.
	void PullToFront();
	/// Sends the document to the back of the document stack.
	void PushToBack();

	/// Show the document.
	/// @param[in] modal_flag Flags controlling the modal state of the document, see the 'ModalFlag' description for details.
	/// @param[in] focus_flag Flags controlling the focus, see the 'FocusFlag' description for details.
	void Show(ModalFlag modal_flag = ModalFlag::None, FocusFlag focus_flag = FocusFlag::Auto);
	/// Hide the document.
	void Hide();
	/// Close the document.
	/// @note The destruction of the document is deferred until the next call to Context::Update().
	void Close();

	/// Creates the named element.
	/// @param[in] name The tag name of the element.
	ElementPtr CreateElement(const String& name);
	/// Create a text element with the given text content.
	/// @param[in] text The text content of the text element.
	ElementPtr CreateTextNode(const String& text);

	/// Does the document have modal display set.
	/// @return True if the document is hogging focus.
	bool IsModal() const;

	/// Load a inline script into the document. Note that the base implementation does nothing, scripting language addons hook
	/// this method.
	/// @param[in] content The script content.
	/// @param[in] source_path Path of the script the source comes from, useful for debug information.
	/// @param[in] source_line Line of the script the source comes from, useful for debug information.
	virtual void LoadInlineScript(const String& content, const String& source_path, int source_line);

	/// Load a external script into the document. Note that the base implementation does nothing, scripting language addons hook
	/// this method.
	/// @param[in] source_path The script file path.
	virtual void LoadExternalScript(const String& source_path);

	/// Updates the document, including its layout. Users must call this manually before requesting information such as
	/// size or position of an element if any element in the document was recently changed, unless Context::Update has
	/// already been called after the change. This has a perfomance penalty, only call when necessary.
	void UpdateDocument();

protected:
	/// Repositions the document if necessary.
	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

	/// Processes the 'onpropertychange' event, checking for a change in position or size.
	void ProcessDefaultAction(Event& event) override;

	/// Called during update if the element size has been changed.
	void OnResize() override;

	/// Returns whether the document can receive focus during click when another document is modal.
	bool IsFocusableFromModal() const;
	/// Sets whether the document can receive focus when another document is modal.
	void SetFocusableFromModal(bool focusable);

private:
	/// Find the next element to focus, starting at the current element
	Element* FindNextTabElement(Element* current_element, bool forward);
	/// Searches forwards or backwards for a focusable element in the given substree
	Element* SearchFocusSubtree(Element* element, bool forward);
	/// Find the next element to navigate to, starting at the current element.
	Element* FindNextNavigationElement(Element* current_element, NavigationSearchDirection direction, const Property& property);

	/// Sets the dirty flag on the layout so the document will format its children before the next render.
	void DirtyLayout() override;
	/// Returns true if the document has been marked as needing a re-layout.
	bool IsLayoutDirty() override;

	/// Notify the document that media query related properties have changed and that style sheets need to be re-evaluated.
	void DirtyMediaQueries();

	/// Updates all sizes defined by the 'vw' and the 'vh' units.
	void DirtyVwAndVhProperties();

	/// Updates the layout if necessary.
	void UpdateLayout();

	/// Updates the position of the document based on the style properties.
	void UpdatePosition();
	/// Sets the dirty flag for document positioning
	void DirtyPosition();

	String title;
	String source_url;

	SharedPtr<StyleSheetContainer> style_sheet_container;

	Context* context;

	bool modal;
	bool focusable_from_modal;

	bool layout_dirty;
	bool position_dirty;

	friend class Rml::Context;
	friend class Rml::Factory;
};

} // namespace Rml
#endif
