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

#ifndef RMLUICOREELEMENTDOCUMENT_H
#define RMLUICOREELEMENTDOCUMENT_H

#include "Element.h"

namespace Rml {
namespace Core {

class Stream;

}
}

namespace Rml {
namespace Core {

class Context;
class DocumentHeader;
class ElementText;
class StyleSheet;

/**
	Represents a document in the dom tree.

	@author Lloyd Weehuizen
 */

class RMLUICORE_API ElementDocument : public Element
{
public:
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

	/// Sets the style sheet this document, and all of its children, uses.
	void SetStyleSheet(SharedPtr<StyleSheet> style_sheet);
	/// Returns the document's style sheet.
	const SharedPtr<StyleSheet>& GetStyleSheet() const override;

	/// Brings the document to the front of the document stack.
	void PullToFront();
	/// Sends the document to the back of the document stack.
	void PushToBack();

	/**
		Flags used for displaying the document.
	 */
	enum FocusFlags
	{
		NONE = 0,
		FOCUS = (1 << 1),
		MODAL = (1 << 2)
	};

	/// Show the document.
	/// @param[in] focus_flags Flags controlling the changing of focus. Leave as FOCUS to switch focus to the document.
	void Show(int focus_flags = FOCUS);
	/// Hide the document.
	void Hide();
	/// Close the document.
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

	/// Load a script into the document. Note that the base implementation does nothing, scripting language addons hook
	/// this method.
	/// @param[in] stream Stream of code to process.
	/// @param[in] source_name Name of the the script the source comes from, useful for debug information.
	virtual void LoadScript(Stream* stream, const String& source_name);

	/// Updates the document, including its layout. Users must call this manually before requesting information such as 
	/// size or position of an element if any element in the document was recently changed, unless Context::Update has
	/// already been called after the change. This has a perfomance penalty, only call when strictly necessary.
	void UpdateDocument();
	
protected:
	/// Repositions the document if necessary.
	virtual void OnPropertyChange(const PropertyNameList& changed_properties) override;

	/// Processes the 'onpropertychange' event, checking for a change in position or size.
	virtual void ProcessDefaultAction(Event& event) override;

	/// Called during update if the element size has been changed.
	virtual void OnResize() override;

private:
	/// Find the next element to focus, starting at the current element
	bool FocusNextTabElement(Element* current_element, bool forward);
	/// Searches forwards or backwards for a focusable element in the given substree
	bool SearchFocusSubtree(Element* element, bool forward);

	/// Sets the dirty flag on the layout so the document will format its children before the next render.
	void DirtyLayout() override;

	/// Returns true if the document has been marked as needing a re-layout.
	bool IsLayoutDirty() override;

	/// Updates all sizes defined by the 'lp' unit.
	void DirtyDpProperties();

	/// Updates the layout if necessary.
	void UpdateLayout();

	/// Updates the position of the document based on the style properties.
	void UpdatePosition();

	/// Sets the dirty flag for document positioning
	void DirtyPosition();

	// Title of the document
	String title;

	// The original path this document came from
	String source_url;

	// The document's style sheet.
	SharedPtr<StyleSheet> style_sheet;

	Context* context;

	// Is the current display modal
	bool modal;

	// Is the layout dirty?
	bool layout_dirty;

	bool position_dirty;

	friend class Context;
	friend class Factory;

};

}
}

#endif
