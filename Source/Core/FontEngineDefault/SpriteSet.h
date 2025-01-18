#ifndef GRAPHICS_SPRITESET_H
#define GRAPHICS_SPRITESET_H

#include <cstdlib>
#include <functional>
#include <vector>

class SpriteSet final {
	private:
		struct Page {
			unsigned previousIndex, nextIndex;
			unsigned firstShelfIndex;
			unsigned textureId;
			unsigned firstDirtyY, pastLastDirtyY, firstDirtyX, pastLastDirtyX;
			std::unique_ptr<std::vector<unsigned char>> textureData;
		};
		struct Shelf {
			unsigned pageIndex;
			unsigned y, height;
			unsigned previousIndex, nextIndex;
			unsigned firstSlotIndex, firstFreeSlotIndex;
			bool allocated;
		};
		struct Slot {
			unsigned shelfIndex;
			unsigned textureId;
			unsigned x, y, width, height, actualWidth;
			unsigned previousIndex, nextIndex;
			unsigned previousFreeIndex, nextFreeIndex;
			unsigned epoch;
			bool allocated;
		};

		unsigned bytesPerPixel, pageSize, spritePadding;
		std::vector<Page> pagePool{1 << 3};
		std::vector<Shelf> shelfPool{1 << 8};
		std::vector<Slot> slotPool{1 << 10};
		unsigned
			pageCount = 0, firstPageIndex = -1, lastPageIndex = 0,
			nextFreeShelfIndex = 0, nextFreeSlotIndex = 0;
		unsigned firstPageAllocatedPixels = 0;
		unsigned currentEpoch = 0;

		unsigned allocate(unsigned width, unsigned height);
		unsigned tryAllocateInPage(unsigned pageIndex, unsigned width, unsigned height);
		unsigned remove(unsigned slotIndex);
	public:
		struct Handle {
			unsigned slotIndex;
			unsigned epoch;
		};
		struct SpriteData {
			unsigned textureId, x, y, width, height;
		};

		std::function<void(unsigned)> removalCallback;
		std::function<void(unsigned, Handle)> migrationCallback;

		SpriteSet(unsigned bytesPerPixel, unsigned pageSize, unsigned spritePadding);
		void tick();
		Handle add(const unsigned width, const unsigned height, const unsigned char *const data) {
			return add(width, height, data, width);
		}
		Handle add(unsigned width, unsigned height, const unsigned char *data, unsigned rowStride);
		void remove(Handle handle);
		SpriteData get(Handle handle) const;
		std::vector<const unsigned char*> getTextures() const;
		void dump(std::vector<SpriteData> &sprites) const;
};

#endif // GRAPHICS_SPRITESET_H