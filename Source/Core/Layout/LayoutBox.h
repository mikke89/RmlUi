#pragma once

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

/*
    A box used to represent the formatting structure of the document, taking part in the box tree.
*/
class LayoutBox {
public:
	enum class Type { Root, BlockContainer, InlineContainer, FlexContainer, TableWrapper, Replaced };

	virtual ~LayoutBox() = default;

	Type GetType() const { return type; }
	Vector2f GetVisibleOverflowSize() const { return visible_overflow_size; }

	// Returns a pointer to the dimensions box if this layout box has one.
	virtual const Box* GetIfBox() const;
	// Returns the baseline of the last line of this box, if any. Returns true if a baseline was found, otherwise false.
	virtual bool GetBaselineOfLastLine(float& out_baseline) const;
	// Calculates the box's inner content width, i.e. the size used to calculate the shrink-to-fit width.
	virtual float GetShrinkToFitWidth() const;

	// Debug dump layout tree.
	String DumpLayoutTree(int depth = 0) const { return DebugDumpTree(depth); }

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

protected:
	LayoutBox(Type type) : type(type) {}

	void SetVisibleOverflowSize(Vector2f size) { visible_overflow_size = size; }

	virtual String DebugDumpTree(int depth) const = 0;

private:
	Type type;

	// Visible overflow size is the border size of this box, plus any overflowing content. If this is a scroll
	// container, then any overflow should be caught here, and not contribute to our visible overflow size.
	Vector2f visible_overflow_size;
};

} // namespace Rml
