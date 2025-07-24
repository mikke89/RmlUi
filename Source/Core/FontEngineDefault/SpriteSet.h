#ifndef GRAPHICS_SPRITESET_H // TODO for PR: Change the header guard when the name and location are finalised.
#define GRAPHICS_SPRITESET_H

#include "../../../Include/RmlUi/Core/Types.h"
#include <cstdlib>

namespace Rml {

/**
    A texture atlas allocator that uses the shelf packing algorithm and supports fast addition and
    removal of images.

    @author Lê Duy Quang
*/

// TODO for PR: Give a better name.
class SpriteSet final {
public:
	struct Handle {
		unsigned int slot_index;
		unsigned int epoch;
	};
	struct SpriteData {
		unsigned int texture_id;
		unsigned int x;
		unsigned int y;
		unsigned int width;
		unsigned int height;
	};

	/// Callback for when an image is migrated from one page to another.
	Function<void(unsigned int, Handle)> migration_callback;

	/// @param[in] bytes_per_pixel The number of bytes for each pixel (the pixel format is not needed for operation).
	/// @param[in] page_size The edge length in pixels of each texture page.
	/// @param[in] sprite_padding Number of blank pixels surrounding each image.
	SpriteSet(unsigned int bytes_per_pixel, unsigned int page_size, unsigned int sprite_padding);

	void Tick();

	/// Adds an image to the texture atlas.
	/// @param[in] width The width of the image.
	/// @param[in] height The height of the image.
	/// @param[in] data The image data.
	/// @return The handle to the image.
	Handle Add(const unsigned int width, const unsigned int height, const unsigned char* data) { return Add(width, height, data, width); }

	/// Adds a subimage to the texture atlas.
	/// @param[in] width The width of the subimage.
	/// @param[in] height The height of the subimage.
	/// @param[in] data Pointer to the first pixel of the subimage.
	/// @param[in] row_stride The width of the whole image that contains the subimage.
	/// @return The handle to the image.
	Handle Add(unsigned int width, unsigned int height, const unsigned char* data, unsigned int row_stride);

	/// Removes an image from the texture atlas.
	/// @param[in] handle The handle to the image.
	void Remove(Handle handle);

	/// Retrieves information about an image in the texture atlas.
	/// @param[in] handle The handle to the image.
	/// @return The information about the image.
	SpriteData Get(Handle handle) const;

	/// Retrieves texture data for all pages of the texture atlas.
	/// @return An array of pointers to each page's texture data.
	Vector<const unsigned char*> GetTextures() const;

private:
	struct Page {
		unsigned int previous_index, next_index;
		unsigned int first_shelf_index;
		unsigned int texture_id;
		unsigned int first_dirty_y;
		unsigned int past_last_dirty_y;
		unsigned int first_dirty_x;
		unsigned int past_last_dirty_x;
		UniquePtr<unsigned char[]> texture_data;
	};
	struct Shelf {
		unsigned int page_index;
		unsigned int y;
		unsigned int height;
		unsigned int previous_index;
		unsigned int next_index;
		unsigned int first_slot_index;
		unsigned int first_free_slot_index;
		bool allocated;
	};
	struct Slot {
		unsigned int shelf_index;
		unsigned int texture_id;
		unsigned int x;
		unsigned int y;
		unsigned int width;
		unsigned int height;
		unsigned int actual_width;
		unsigned int previous_index;
		unsigned int next_index;
		unsigned int previous_free_index;
		unsigned int next_free_index;
		unsigned int epoch;
		bool allocated;
	};

	unsigned int bytes_per_pixel;
	unsigned int page_size;
	unsigned int sprite_padding;
	Vector<Page> page_pool{1 << 3};
	Vector<Shelf> shelf_pool{1 << 8};
	Vector<Slot> slot_pool{1 << 10};
	unsigned int page_count = 0;
	unsigned int first_page_index = static_cast<unsigned int>(-1);
	unsigned int last_page_index = 0;
	unsigned int next_free_shelf_index = 0;
	unsigned int next_free_slot_index = 0;
	unsigned int first_page_allocated_pixels = 0;
	unsigned int current_epoch = 0;

	unsigned int Allocate(unsigned int width, unsigned int height);
	unsigned int TryAllocateInPage(unsigned int page_index, unsigned int width, unsigned int height);
	unsigned int Remove(unsigned int slot_index);
};

} // namespace Rml
#endif
