#ifndef RMLUI_CORE_LAYOUT_LAYOUTPOOLS_H
#define RMLUI_CORE_LAYOUT_LAYOUTPOOLS_H

#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

namespace LayoutPools {

	void Initialize();
	void Shutdown();

	void* AllocateLayoutChunk(size_t size);
	void DeallocateLayoutChunk(void* chunk, size_t size);

} // namespace LayoutPools

} // namespace Rml
#endif
