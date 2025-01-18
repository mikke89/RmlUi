#include <algorithm>

#include "SpriteSet.h"

namespace {
	constexpr unsigned nullIndex = -1;
	constexpr unsigned maxChangedPixels = 256 * 256;
	constexpr unsigned splitThreshold = 8;

	template<typename T>
	void initializePool(std::vector<T> &pool) {
		const unsigned
			poolSize = static_cast<unsigned>(pool.size()),
			lastPoolIndex = poolSize - 1;
		for (unsigned i = 0; i != lastPoolIndex; ++i) pool[i].nextIndex = i + 1;
		pool[lastPoolIndex].nextIndex = nullIndex;
	}

	template<typename T>
	unsigned allocateEntry(std::vector<T> &pool, unsigned &nextFreeIndex) {
		if (nextFreeIndex == nullIndex) {
			const unsigned
				oldPoolSize = static_cast<unsigned>(pool.size()),
				newPoolSize = oldPoolSize << 1,
				lastPoolIndex = newPoolSize - 1;
			pool.resize(newPoolSize);
			for (unsigned i = oldPoolSize; i != lastPoolIndex; ++i) pool[i].nextIndex = i + 1;
			pool[lastPoolIndex].nextIndex = nullIndex;
			nextFreeIndex = oldPoolSize;
		}
		const unsigned index = nextFreeIndex;
		nextFreeIndex = pool[index].nextIndex;
		return index;
	}

	template<typename T>
	void freeEntry(std::vector<T> &pool, unsigned &nextFreeIndex, const unsigned index) {
		pool[index].nextIndex = nextFreeIndex;
		nextFreeIndex = index;
	}
}

SpriteSet::SpriteSet(const unsigned bytesPerPixel, const unsigned pageSize, const unsigned spritePadding):
	bytesPerPixel(bytesPerPixel), pageSize(pageSize), spritePadding(spritePadding)
{
	initializePool(pagePool);
	initializePool(shelfPool);
	initializePool(slotPool);
}

void SpriteSet::tick() {
	unsigned changedPixels = 0;
	// Try compaction by moving sprites from the last page to the first page.
	while (changedPixels <= maxChangedPixels && firstPageIndex != lastPageIndex) {
		const Page &sourcePage = pagePool[lastPageIndex];
		unsigned sourceShelfIndex = sourcePage.firstShelfIndex;
		while (!shelfPool[sourceShelfIndex].allocated) sourceShelfIndex = shelfPool[sourceShelfIndex].nextIndex;
		const Shelf &sourceShelf = shelfPool[sourceShelfIndex];
		unsigned sourceSlotIndex = sourceShelf.firstSlotIndex;
		while (!slotPool[sourceSlotIndex].allocated) sourceSlotIndex = slotPool[sourceSlotIndex].nextIndex;
		const Slot &sourceSlot = slotPool[sourceSlotIndex];
		const unsigned destinationSlotIndex = tryAllocateInPage(firstPageIndex, sourceSlot.width, sourceSlot.height);
		if (destinationSlotIndex == nullIndex) break;
		const Slot &destinationSlot = slotPool[destinationSlotIndex];
		const Shelf &destinationShelf = shelfPool[destinationSlot.shelfIndex];
		Page &destinationPage = pagePool[firstPageIndex];
		const unsigned
			sourceX = sourceSlot.x, sourceY = sourceShelf.y,
			destinationX = destinationSlot.x, destinationY = destinationShelf.y,
			width = sourceSlot.width, height = sourceSlot.height;
		for (unsigned localY = 0; localY != height; ++localY) {
			const auto dataStart = sourcePage.textureData->begin() + ((sourceY + localY) * pageSize + sourceX);
			std::copy(
				dataStart, dataStart + width,
				destinationPage.textureData->begin() + ((destinationY + localY) * pageSize + destinationX)
			);
		}
		destinationPage.firstDirtyY = std::min(destinationPage.firstDirtyY, destinationY);
		destinationPage.pastLastDirtyY = std::max(destinationPage.pastLastDirtyY, destinationY + height);
		destinationPage.firstDirtyX = std::min(destinationPage.firstDirtyX, destinationX);
		destinationPage.pastLastDirtyX = std::max(destinationPage.pastLastDirtyX, destinationX + width);
		changedPixels += remove(sourceSlotIndex);
		if (migrationCallback) migrationCallback(sourceSlotIndex, {destinationSlotIndex, destinationSlot.epoch});
	}
}

SpriteSet::Handle SpriteSet::add(
	const unsigned width, const unsigned height, const unsigned char *const data, const unsigned rowStride
) {
	const unsigned paddedWidth = width + spritePadding * 2, paddedHeight = height + spritePadding * 2;
	const unsigned slotIndex = allocate(paddedWidth, paddedHeight);
	const Slot &slot = slotPool[slotIndex];
	const Shelf &shelf = shelfPool[slot.shelfIndex];
	Page &page = pagePool[shelf.pageIndex];
	for (
		unsigned i = 0, topPaddingY = shelf.y, bottomPaddingY = shelf.y + paddedHeight - 1;
		i != spritePadding; ++i, ++topPaddingY, --bottomPaddingY
	) {
		auto textureStart = page.textureData->begin() + (topPaddingY * pageSize + slot.x) * bytesPerPixel;
		std::fill(textureStart, textureStart + paddedWidth * bytesPerPixel, 0);
		textureStart = page.textureData->begin() + (bottomPaddingY * pageSize + slot.x) * bytesPerPixel;
		std::fill(textureStart, textureStart + paddedWidth * bytesPerPixel, 0);
	}
	const unsigned textureY = shelf.y + spritePadding;
	for (unsigned localY = 0; localY != height; ++localY) {
		const unsigned char *const dataStart = data + localY * rowStride * bytesPerPixel;
		const auto textureStart = page.textureData->begin() + ((textureY + localY) * pageSize + slot.x) * bytesPerPixel;
		std::fill(textureStart, textureStart + spritePadding * bytesPerPixel, 0);
		std::fill(
			textureStart + (spritePadding + width) * bytesPerPixel, textureStart + paddedWidth * bytesPerPixel, 0
		);
		std::copy(dataStart, dataStart + width * bytesPerPixel, textureStart + spritePadding * bytesPerPixel);
	}
	page.firstDirtyY = std::min(page.firstDirtyY, shelf.y);
	page.pastLastDirtyY = std::max(page.pastLastDirtyY, shelf.y + paddedHeight);
	page.firstDirtyX = std::min(page.firstDirtyX, slot.x);
	page.pastLastDirtyX = std::max(page.pastLastDirtyX, slot.x + paddedWidth);
	return {slotIndex, slot.epoch};
}

unsigned SpriteSet::allocate(const unsigned width, const unsigned height) {
	// Try to allocate in an existing page.
	if (firstPageIndex != nullIndex) {
		unsigned pageIndex = firstPageIndex;
		while (true) {
			const unsigned slotIndex = tryAllocateInPage(pageIndex, width, height);
			if (slotIndex != nullIndex) return slotIndex;
			if (pageIndex == lastPageIndex) break;
			pageIndex = pagePool[pageIndex].nextIndex;
		}
	}

	// No page could fit, allocate a new page.
	if (firstPageIndex == nullIndex) {
		firstPageIndex = lastPageIndex;
		Page &page = pagePool[lastPageIndex];
		page.previousIndex = nullIndex;
	} else {
		unsigned pageIndex = pagePool[lastPageIndex].nextIndex;
		if (pageIndex == nullIndex) {
			const unsigned
				oldPoolSize = static_cast<unsigned>(pagePool.size()),
				newPoolSize = oldPoolSize << 1,
				lastPoolIndex = newPoolSize - 1;
			pagePool.resize(newPoolSize);
			for (unsigned i = oldPoolSize; i != lastPoolIndex; ++i) pagePool[i].nextIndex = i + 1;
			pagePool[lastPoolIndex].nextIndex = nullIndex;
			pagePool[lastPageIndex].nextIndex = oldPoolSize;
			pageIndex = oldPoolSize;
		}
		pagePool[pageIndex].previousIndex = lastPageIndex;
		lastPageIndex = pageIndex;
	}

	Page &page = pagePool[lastPageIndex];
	page.textureId = pageCount;
	page.textureData = std::make_unique<std::vector<unsigned char>>(pageSize * pageSize * bytesPerPixel);
	page.firstDirtyY = pageSize;
	page.pastLastDirtyY = 0;
	page.firstDirtyX = pageSize;
	page.pastLastDirtyX = 0;

	const unsigned shelfIndex = allocateEntry<Shelf>(shelfPool, nextFreeShelfIndex);
	const unsigned slotIndex = allocateEntry<Slot>(slotPool, nextFreeSlotIndex);
	slotPool[slotIndex] = {shelfIndex, pageCount, 0, 0, pageSize, 0, 0, nullIndex, nullIndex, nullIndex, nullIndex, 0, false};
	shelfPool[shelfIndex] = {lastPageIndex, 0, pageSize, nullIndex, nullIndex, slotIndex, slotIndex, false};
	page.firstShelfIndex = shelfIndex;
	++pageCount;
	return tryAllocateInPage(lastPageIndex, width, height);
}

unsigned SpriteSet::tryAllocateInPage(const unsigned pageIndex, const unsigned width, const unsigned height) {
	Page &page = pagePool[pageIndex];
	unsigned selectedShelfIndex = nullIndex, selectedSlotIndex = nullIndex, selectedShelfHeight = -1;
	for (
		unsigned shelfIndex = page.firstShelfIndex;
		shelfIndex != nullIndex; shelfIndex = shelfPool[shelfIndex].nextIndex
	) {
		const Shelf &shelf = shelfPool[shelfIndex];
		if (
			shelf.height < height || shelf.height >= selectedShelfHeight
			|| (shelf.allocated && shelf.height > height * 3 / 2)
		) continue;
		bool found = false;
		for (
			unsigned slotIndex = shelf.firstFreeSlotIndex;
			slotIndex != nullIndex; slotIndex = slotPool[slotIndex].nextFreeIndex
		) {
			const Slot &slot = slotPool[slotIndex];
			if (slot.width < width) continue;
			selectedShelfIndex = shelfIndex;
			selectedSlotIndex = slotIndex;
			selectedShelfHeight = shelf.height;
			found = true;
			break;
		}
		if (found && shelf.height == height) break;
	}
	if (selectedSlotIndex == nullIndex) return nullIndex;
	Shelf *shelf = &shelfPool[selectedShelfIndex];
	if (!shelf->allocated) {
		shelf->allocated = true;
		if (shelf->height - height >= splitThreshold) {
			const unsigned newShelfIndex = allocateEntry<Shelf>(shelfPool, nextFreeShelfIndex);
			const unsigned newSlotIndex = allocateEntry<Slot>(slotPool, nextFreeSlotIndex);
			shelf = &shelfPool[selectedShelfIndex];
			slotPool[newSlotIndex] = {
				newShelfIndex, page.textureId, 0, shelf->y + height, pageSize, 0, 0, nullIndex, nullIndex, nullIndex, nullIndex, 0, false
			};
			shelfPool[newShelfIndex] = {
				pageIndex, shelf->y + height, shelf->height - height,
				selectedShelfIndex, shelf->nextIndex,  newSlotIndex, newSlotIndex, false
			};
			if (shelf->nextIndex != nullIndex) shelfPool[shelf->nextIndex].previousIndex = newShelfIndex;
			shelf->nextIndex = newShelfIndex;
			shelf->height = height;
		}
	}
	Slot *slot = &slotPool[selectedSlotIndex];
	if (slot->width - width >= splitThreshold) {
		const unsigned newSlotIndex = allocateEntry<Slot>(slotPool, nextFreeSlotIndex);
		slot = &slotPool[selectedSlotIndex];
		slotPool[newSlotIndex] = {
			selectedShelfIndex, page.textureId, slot->x + width, shelf->y, slot->width - width, 0, 0,
			selectedSlotIndex, slot->nextIndex, slot->previousFreeIndex, slot->nextFreeIndex, 0, false
		};
		if (slot->nextIndex != nullIndex) slotPool[slot->nextIndex].previousIndex = newSlotIndex;
		slot->nextIndex = newSlotIndex;
		if (slot->previousFreeIndex == nullIndex) shelf->firstFreeSlotIndex = newSlotIndex;
		else slotPool[slot->previousFreeIndex].nextFreeIndex = newSlotIndex;
		if (slot->nextFreeIndex != nullIndex) slotPool[slot->nextFreeIndex].previousFreeIndex = newSlotIndex;
		slot->width = width;
	} else {
		if (slot->previousFreeIndex == nullIndex) shelf->firstFreeSlotIndex = slot->nextFreeIndex;
		else slotPool[slot->previousFreeIndex].nextFreeIndex = slot->nextFreeIndex;
		if (slot->nextFreeIndex != nullIndex) slotPool[slot->nextFreeIndex].previousFreeIndex = slot->previousFreeIndex;
	}
	slot->allocated = true;
	slot->actualWidth = width;
	slot->height = height;
	slot->epoch = currentEpoch;
	if (pageIndex == firstPageIndex) firstPageAllocatedPixels += width * shelf->height;
	return selectedSlotIndex;
}

unsigned SpriteSet::remove(const unsigned slotIndex) {
	Slot &slot = slotPool[slotIndex];
	Shelf &shelf = shelfPool[slot.shelfIndex];
	const unsigned pageIndex = shelf.pageIndex;
	Page &page = pagePool[pageIndex];

	for (int offsetY = 0; offsetY < slot.height; ++offsetY) {
		for (int offsetX = 0; offsetX < slot.width; ++offsetX) {
			const auto pixel = page.textureData->begin() + ((slot.y + offsetY) * pageSize + slot.x + offsetX) * 4;
			pixel[0] = 0;
			pixel[1] = 0;
			pixel[2] = 255;
			pixel[3] = 255;
		}
	}

	const unsigned slotPixels = slot.width * slot.height;
	slot.allocated = false;
	slot.previousFreeIndex = nullIndex;
	if (shelf.firstFreeSlotIndex == nullIndex) {
		slot.nextFreeIndex = nullIndex;
	} else {
		slot.nextFreeIndex = shelf.firstFreeSlotIndex;
		slotPool[shelf.firstFreeSlotIndex].previousFreeIndex = slotIndex;
	}
	shelf.firstFreeSlotIndex = slotIndex;
	++slot.epoch;

	// Merge consecutive empty slots.
	if (slot.nextIndex != nullIndex) {
		Slot &nextSlot = slotPool[slot.nextIndex];
		if (!nextSlot.allocated) {
			slot.width += nextSlot.width;
			const unsigned nextIndex = slot.nextIndex;
			slot.nextIndex = nextSlot.nextIndex;
			if (nextSlot.previousFreeIndex != nullIndex)
				slotPool[nextSlot.previousFreeIndex].nextFreeIndex = nextSlot.nextFreeIndex;
			if (nextSlot.nextFreeIndex != nullIndex)
				slotPool[nextSlot.nextFreeIndex].previousFreeIndex = nextSlot.previousFreeIndex;
			freeEntry<Slot>(slotPool, nextFreeSlotIndex, nextIndex);
			if (slot.nextIndex != nullIndex) slotPool[slot.nextIndex].previousIndex = slotIndex;
		}
	}
	if (slot.previousIndex != nullIndex) {
		Slot &previousSlot = slotPool[slot.previousIndex];
		if (!previousSlot.allocated) {
			slot.x -= previousSlot.width;
			slot.width += previousSlot.width;
			const unsigned previousIndex = slot.previousIndex;
			slot.previousIndex = previousSlot.previousIndex;
			if (previousSlot.previousFreeIndex != nullIndex)
				slotPool[previousSlot.previousFreeIndex].nextFreeIndex = previousSlot.nextFreeIndex;
			if (previousSlot.nextFreeIndex != nullIndex)
				slotPool[previousSlot.nextFreeIndex].previousFreeIndex = previousSlot.previousFreeIndex;
			freeEntry<Slot>(slotPool, nextFreeSlotIndex, previousIndex);
			if (slot.previousIndex == nullIndex) {
				shelf.firstSlotIndex = slotIndex;
				if (slot.nextIndex == nullIndex) shelf.allocated = false;
			} else {
				slotPool[slot.previousIndex].nextIndex = slotIndex;
			}
		}
	}

	// Merge consecutive empty shelves.
	if (shelf.allocated) return slotPixels;
	if (shelf.nextIndex != nullIndex) {
		Shelf &nextShelf = shelfPool[shelf.nextIndex];
		if (!nextShelf.allocated) {
			shelf.height += nextShelf.height;
			const unsigned nextIndex = shelf.nextIndex;
			shelf.nextIndex = nextShelf.nextIndex;
			freeEntry<Slot>(slotPool, nextFreeSlotIndex, nextShelf.firstSlotIndex);
			freeEntry<Shelf>(shelfPool, nextFreeShelfIndex, nextIndex);
			if (shelf.nextIndex != nullIndex) shelfPool[shelf.nextIndex].previousIndex = slot.shelfIndex;
		}
	}
	if (shelf.previousIndex != nullIndex) {
		Shelf &previousShelf = shelfPool[shelf.previousIndex];
		if (!previousShelf.allocated) {
			shelf.y -= previousShelf.height;
			shelf.height += previousShelf.height;
			slot.y = shelf.y;
			const unsigned previousIndex = shelf.previousIndex;
			shelf.previousIndex = previousShelf.previousIndex;
			freeEntry<Slot>(slotPool, nextFreeSlotIndex, previousShelf.firstSlotIndex);
			freeEntry<Shelf>(shelfPool, nextFreeShelfIndex, previousIndex);
			if (shelf.previousIndex == nullIndex) page.firstShelfIndex = slot.shelfIndex;
			else shelfPool[shelf.previousIndex].nextIndex = slot.shelfIndex;
		}
	}

	// Deallocate the page if it becomes empty, except when it's the first one.
	if (pageIndex == firstPageIndex) {
		firstPageAllocatedPixels -= slot.width * shelf.height;
		return slotPixels;
	}
	if (shelf.height != pageSize) return slotPixels;
	freeEntry<Slot>(slotPool, nextFreeSlotIndex, slotIndex);
	freeEntry<Shelf>(shelfPool, nextFreeShelfIndex, page.firstShelfIndex);
	page.textureData.reset();
	pagePool[page.previousIndex].nextIndex = page.nextIndex;
	if (pageIndex != lastPageIndex) pagePool[page.nextIndex].previousIndex = page.previousIndex;
	pagePool[lastPageIndex].nextIndex = pageIndex;
	return slotPixels;
}

void SpriteSet::remove(const Handle handle) {
	Slot &slot = slotPool[handle.slotIndex];
	if (slot.epoch != handle.epoch) return;
	remove(handle.slotIndex);
}

SpriteSet::SpriteData SpriteSet::get(const Handle handle) const {
	const Slot &slot = slotPool[handle.slotIndex];
	return {
		slot.textureId, slot.x + spritePadding, slot.y + spritePadding,
		slot.actualWidth - spritePadding * 2, slot.height - spritePadding * 2
	};
}

std::vector<const unsigned char*> SpriteSet::getTextures() const {
	if (firstPageIndex == nullIndex)
		return {};
	std::vector<const unsigned char*> textures;
	unsigned pageIndex = firstPageIndex;
	while (true) {
		const Page &page = pagePool[pageIndex];
		textures.push_back(page.textureData->data());
		if (pageIndex == lastPageIndex) break;
		pageIndex = page.nextIndex;
	}
	return textures;
}

void SpriteSet::dump(std::vector<SpriteSet::SpriteData> &sprites) const {
	if (firstPageIndex == nullIndex) return;
	for (
		unsigned shelfIndex = pagePool[firstPageIndex].firstShelfIndex;
		shelfIndex != nullIndex; shelfIndex = shelfPool[shelfIndex].nextIndex
	) {
		const unsigned shelfY = shelfPool[shelfIndex].y;
		for (
			unsigned slotIndex = shelfPool[shelfIndex].firstSlotIndex;
			slotIndex != nullIndex; slotIndex = slotPool[slotIndex].nextIndex
		) {
			const Slot &slot = slotPool[slotIndex];
			if (slot.allocated) sprites.push_back({slot.x, shelfY, slot.width, slot.height});
		}
	}
}