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

#ifndef RMLUI_CORE_STYLESHEETNODE_H
#define RMLUI_CORE_STYLESHEETNODE_H

#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "StyleSheetSelector.h"

namespace Rml {

struct StyleSheetIndex;
class StyleSheetNode;
using StyleSheetNodeList = Vector<UniquePtr<StyleSheetNode>>;

/**
    A style sheet is composed of a tree of nodes.

    @author Pete / Lloyd
 */

class StyleSheetNode {
public:
	StyleSheetNode();
	StyleSheetNode(StyleSheetNode* parent, const CompoundSelector& selector);
	StyleSheetNode(StyleSheetNode* parent, CompoundSelector&& selector);

	/// Retrieves or creates a child node with requirements equivalent to the 'other' node.
	StyleSheetNode* GetOrCreateChildNode(const CompoundSelector& other);
	/// Retrieves a child node with the given requirements if they match an existing node, or else creates a new one.
	StyleSheetNode* GetOrCreateChildNode(CompoundSelector&& other);

	/// Merges an entire tree hierarchy into our hierarchy.
	void MergeHierarchy(StyleSheetNode* node, int specificity_offset = 0);
	/// Copy this node including all descendent nodes.
	UniquePtr<StyleSheetNode> DeepCopy(StyleSheetNode* parent = nullptr) const;
	/// Builds up a style sheet's index recursively.
	void BuildIndex(StyleSheetIndex& styled_node_index) const;

	/// Imports properties from a single rule definition into the node's properties and sets the appropriate specificity on them. Any existing
	/// attributes sharing a key with a new attribute will be overwritten if they are of a lower specificity.
	/// @param[in] properties The properties to import.
	/// @param[in] rule_specificity The specificity of the importing rule.
	void ImportProperties(const PropertyDictionary& properties, int rule_specificity);
	/// Returns the node's default properties.
	const PropertyDictionary& GetProperties() const;

	/// Returns true if this node is applicable to the given element, given its IDs, classes and heritage.
	/// @note For performance reasons this call does not check whether 'element' is a text element. The caller must manually check this condition and
	/// consider any text element not applicable.
	bool IsApplicable(const Element* element) const;

	/// Returns the specificity of this node.
	int GetSpecificity() const;

private:
	void CalculateAndSetSpecificity();

	// Match an element to the local node requirements.
	inline bool Match(const Element* element) const;
	inline bool MatchStructuralSelector(const Element* element) const;
	inline bool MatchAttributes(const Element* element) const;

	// Recursively traverse the nodes up towards the root to match the element and its hierarchy.
	bool TraverseMatch(const Element* element) const;

	// The parent of this node; is nullptr for the root node.
	StyleSheetNode* parent = nullptr;

	// Node requirements
	CompoundSelector selector;

	// A measure of specificity of this node; the attribute in a node with a higher value will override those of a node with a lower value.
	int specificity = 0;

	PropertyDictionary properties;

	StyleSheetNodeList children;
};

} // namespace Rml
#endif
