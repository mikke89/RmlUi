#pragma once

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
/** ScrollFlag controls whether an element is scrolled into view when showing the document. */
enum class ScrollFlag {
	None, // Never scroll.
	Auto, // Scroll the focused element into view, if applicable.
};

/**
    Represents a document in the dom tree.
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
	/// @param[in] modal_flag Flag controlling the modal state of the document, see the 'ModalFlag' description for details.
	/// @param[in] focus_flag Flag controlling the focus, see the 'FocusFlag' description for details.
	/// @param[in] scroll_flag Flag controlling scrolling, see the 'ScrollFlag' description for details.
	void Show(ModalFlag modal_flag = ModalFlag::None, FocusFlag focus_flag = FocusFlag::Auto, ScrollFlag scroll_flag = ScrollFlag::Auto);
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

	/// Finds the next tabbable element in the document tree, starting at the given element, possibly wrapping around the document.
	/// @param[in] current_element The element to start from.
	/// @param[in] forward True to search forward, false to search backward.
	/// @return The next tabbable element, or nullptr if none could be found.
	Element* FindNextTabElement(Element* current_element, bool forward);

	/// Loads an inline script into the document. Note that the base implementation does nothing, but script plugins can hook into this method.
	/// @param[in] content The script content.
	/// @param[in] source_path Path of the script the source comes from, useful for debug information.
	/// @param[in] source_line Line of the script the source comes from, useful for debug information.
	virtual void LoadInlineScript(const String& content, const String& source_path, int source_line);
	/// Loads an external script into the document. Note that the base implementation does nothing, but script plugins can hook into this method.
	/// @param[in] source_path The script file path.
	virtual void LoadExternalScript(const String& source_path);

	/// Updates the document, including its layout. Users must call this manually before requesting information such as
	/// the size or position of an element if any element in the document was recently changed, unless Context::Update
	/// has already been called after the change. This has a performance penalty, only call when necessary.
	void UpdateDocument();

protected:
	/// Repositions the document if necessary.
	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

	/// Processes any events specially handled by the document.
	void ProcessDefaultAction(Event& event) override;

	/// Called during update if the element size has been changed.
	void OnResize() override;

	/// Returns whether the document can receive focus during click when another document is modal.
	bool IsFocusableFromModal() const;
	/// Sets whether the document can receive focus when another document is modal.
	void SetFocusableFromModal(bool focusable);

private:
	/// Searches forwards or backwards for a focusable element in the given subtree.
	Element* SearchFocusSubtree(Element* element, bool forward);
	/// Find the next element to navigate to, starting at the current element.
	Element* FindNextNavigationElement(Element* current_element, NavigationSearchDirection direction, const Property& property);

	/// Sets the dirty flag on the layout so the document will format its children before the next render.
	void DirtyLayout() override;
	/// Returns true if the document has been marked as needing a re-layout.
	bool IsLayoutDirty() override;

	/// Notify the document that media query-related properties have changed and that style sheets need to be re-evaluated.
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
