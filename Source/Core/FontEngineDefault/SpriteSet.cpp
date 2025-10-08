#include <algorithm>

#include "../../../Include/RmlUi/Core/Types.h"

#include "SpriteSet.h"

namespace Rml {

namespace {
	constexpr unsigned int null_index = static_cast<unsigned int>(-1);
	constexpr unsigned int max_changed_pixels = 256 * 256;
	constexpr unsigned int split_threshold = 8;

	template<typename T>
	void InitializePool(Vector<T> &pool)
	{
		const unsigned int pool_size = static_cast<unsigned int>(pool.size());
		const unsigned int last_pool_index = pool_size - 1;
		for (unsigned int i = 0; i < last_pool_index; ++i)
			pool[i].next_index = i + 1;
		pool[last_pool_index].next_index = null_index;
	}

	template<typename T>
	unsigned int AllocateEntry(Vector<T>& pool, unsigned int& next_free_index)
	{
		if (next_free_index == null_index)
		{
			const unsigned int old_pool_size = static_cast<unsigned int>(pool.size());
			const unsigned int new_pool_size = old_pool_size << 1;
			const unsigned int last_pool_index = new_pool_size - 1;
			pool.resize(new_pool_size);
			for (unsigned int i = old_pool_size; i < last_pool_index; ++i)
				pool[i].next_index = i + 1;
			pool[last_pool_index].next_index = null_index;
			next_free_index = old_pool_size;
		}
		const unsigned int index = next_free_index;
		next_free_index = pool[index].next_index;
		return index;
	}

	template<typename T>
	void FreeEntry(Vector<T>& pool, unsigned int& next_free_index, const unsigned int index)
	{
		pool[index].next_index = next_free_index;
		next_free_index = index;
	}
}

SpriteSet::SpriteSet(
	const unsigned int bytes_per_pixel, const unsigned int page_size, const unsigned int sprite_padding
) : bytes_per_pixel(bytes_per_pixel), page_size(page_size), sprite_padding(sprite_padding)
{
	InitializePool(page_pool);
	InitializePool(shelf_pool);
	InitializePool(slot_pool);
}

void SpriteSet::Tick()
{
	unsigned int changed_pixels = 0;
	// Try compaction by moving sprites from the last page to the first page.
	while (changed_pixels <= max_changed_pixels && first_page_index != last_page_index)
	{
		const Page& source_page = page_pool[last_page_index];
		unsigned int source_shelf_index = source_page.first_shelf_index;
		while (!shelf_pool[source_shelf_index].allocated)
			source_shelf_index = shelf_pool[source_shelf_index].next_index;
		const Shelf& source_shelf = shelf_pool[source_shelf_index];
		unsigned int source_slot_index = source_shelf.first_slot_index;
		while (!slot_pool[source_slot_index].allocated)
			source_slot_index = slot_pool[source_slot_index].next_index;
		const Slot& source_slot = slot_pool[source_slot_index];
		const unsigned int destination_slot_index = TryAllocateInPage(
			first_page_index, source_slot.width, source_slot.height
		);
		if (destination_slot_index == null_index)
			break;
		const Slot& destination_slot = slot_pool[destination_slot_index];
		const Shelf& destination_shelf = shelf_pool[destination_slot.shelf_index];
		Page& destination_page = page_pool[first_page_index];
		const unsigned int source_x = source_slot.x;
		const unsigned int source_y = source_shelf.y;
		const unsigned int destination_x = destination_slot.x;
		const unsigned int destination_y = destination_shelf.y;
		const unsigned int width = source_slot.width;
		const unsigned int height = source_slot.height;
		for (unsigned int local_y = 0; local_y != height; ++local_y)
		{
			const auto data_start = source_page.texture_data->begin() + ((source_y + local_y) * page_size + source_x);
			std::copy(
				data_start, data_start + width,
				destination_page.texture_data->begin() + ((destination_y + local_y) * page_size + destination_x)
			);
		}
		destination_page.first_dirty_y = std::min(destination_page.first_dirty_y, destination_y);
		destination_page.past_last_dirty_y = std::max(destination_page.past_last_dirty_y, destination_y + height);
		destination_page.first_dirty_x = std::min(destination_page.first_dirty_x, destination_x);
		destination_page.past_last_dirty_x = std::max(destination_page.past_last_dirty_x, destination_x + width);
		changed_pixels += Remove(source_slot_index);
		if (migration_callback)
			migration_callback(source_slot_index, {destination_slot_index, destination_slot.epoch});
	}
}

SpriteSet::Handle SpriteSet::Add(
	const unsigned int width, const unsigned int height, const unsigned char* const data, const unsigned int row_stride
)
{
	const unsigned int padded_width = width + sprite_padding * 2;
	const unsigned int padded_height = height + sprite_padding * 2;
	const unsigned int slot_index = Allocate(padded_width, padded_height);
	const Slot& slot = slot_pool[slot_index];
	const Shelf& shelf = shelf_pool[slot.shelf_index];
	Page& page = page_pool[shelf.page_index];
	for (
		unsigned int i = 0, top_padding_y = shelf.y, bottom_padding_y = shelf.y + padded_height - 1;
		i < sprite_padding; ++i, ++top_padding_y, --bottom_padding_y
	)
	{
		auto texture_start = page.texture_data->begin() + (top_padding_y * page_size + slot.x) * bytes_per_pixel;
		std::fill(texture_start, texture_start + padded_width * bytes_per_pixel, static_cast<unsigned char>(0));
		texture_start = page.texture_data->begin() + (bottom_padding_y * page_size + slot.x) * bytes_per_pixel;
		std::fill(texture_start, texture_start + padded_width * bytes_per_pixel, static_cast<unsigned char>(0));
	}
	const unsigned int texture_y = shelf.y + sprite_padding;
	for (unsigned int local_y = 0; local_y != height; ++local_y)
	{
		const unsigned char* const data_start = data + local_y * row_stride * bytes_per_pixel;
		const auto texture_start = page.texture_data->begin()
			+ ((texture_y + local_y) * page_size + slot.x) * bytes_per_pixel;
		std::fill(texture_start, texture_start + sprite_padding * bytes_per_pixel, static_cast<unsigned char>(0));
		std::fill(
			texture_start + (sprite_padding + width) * bytes_per_pixel, texture_start + padded_width * bytes_per_pixel,
			static_cast<unsigned char>(0)
		);
		std::copy(data_start, data_start + width * bytes_per_pixel, texture_start + sprite_padding * bytes_per_pixel);
	}
	page.first_dirty_y = std::min(page.first_dirty_y, shelf.y);
	page.past_last_dirty_y = std::max(page.past_last_dirty_y, shelf.y + padded_height);
	page.first_dirty_x = std::min(page.first_dirty_x, slot.x);
	page.past_last_dirty_x = std::max(page.past_last_dirty_x, slot.x + padded_width);
	return {slot_index, slot.epoch};
}

unsigned int SpriteSet::Allocate(const unsigned int width, const unsigned int height)
{
	// Try to allocate in an existing page.
	if (first_page_index != null_index)
	{
		unsigned int page_index = first_page_index;
		while (true)
		{
			const unsigned int slot_index = TryAllocateInPage(page_index, width, height);
			if (slot_index != null_index)
				return slot_index;
			if (page_index == last_page_index)
				break;
			page_index = page_pool[page_index].next_index;
		}
	}

	// No page could fit, allocate a new page.
	if (first_page_index == null_index)
	{
		first_page_index = last_page_index;
		Page& page = page_pool[last_page_index];
		page.previous_index = null_index;
	}
	else
	{
		unsigned int page_index = page_pool[last_page_index].next_index;
		if (page_index == null_index)
		{
			const unsigned int old_pool_size = static_cast<unsigned int>(page_pool.size());
			const unsigned int new_pool_size = old_pool_size << 1;
			const unsigned int last_pool_index = new_pool_size - 1;
			page_pool.resize(new_pool_size);
			for (unsigned int i = old_pool_size; i != last_pool_index; ++i)
				page_pool[i].next_index = i + 1;
			page_pool[last_pool_index].next_index = null_index;
			page_pool[last_page_index].next_index = old_pool_size;
			page_index = old_pool_size;
		}
		page_pool[page_index].previous_index = last_page_index;
		last_page_index = page_index;
	}

	Page& page = page_pool[last_page_index];
	page.texture_id = page_count;
	page.texture_data = MakeUnique<Vector<unsigned char>>(page_size * page_size * bytes_per_pixel);
	page.first_dirty_y = page_size;
	page.past_last_dirty_y = 0;
	page.first_dirty_x = page_size;
	page.past_last_dirty_x = 0;

	const unsigned int shelf_index = AllocateEntry<Shelf>(shelf_pool, next_free_shelf_index);
	const unsigned int slot_index = AllocateEntry<Slot>(slot_pool, next_free_slot_index);
	slot_pool[slot_index] = {shelf_index, page_count, 0, 0, page_size, 0, 0, null_index, null_index, null_index, null_index, 0, false};
	shelf_pool[shelf_index] = {last_page_index, 0, page_size, null_index, null_index, slot_index, slot_index, false};
	page.first_shelf_index = shelf_index;
	++page_count;
	return TryAllocateInPage(last_page_index, width, height);
}

unsigned int SpriteSet::TryAllocateInPage(const unsigned int page_index, const unsigned int width, const unsigned int height)
{
	Page& page = page_pool[page_index];
	unsigned int selected_shelf_index = null_index;
	unsigned int selected_slot_index = null_index;
	unsigned int selected_shelf_height = static_cast<unsigned int>(-1);
	for (
		unsigned int shelf_index = page.first_shelf_index;
		shelf_index != null_index; shelf_index = shelf_pool[shelf_index].next_index
	)
	{
		const Shelf& shelf = shelf_pool[shelf_index];
		if (
			shelf.height < height || shelf.height >= selected_shelf_height
			|| (shelf.allocated && shelf.height > height * 3 / 2)
		)
			continue;
		bool found = false;
		for (
			unsigned int slot_index = shelf.first_free_slot_index;
			slot_index != null_index; slot_index = slot_pool[slot_index].next_free_index
		)
		{
			const Slot& slot = slot_pool[slot_index];
			if (slot.width < width)
				continue;
			selected_shelf_index = shelf_index;
			selected_slot_index = slot_index;
			selected_shelf_height = shelf.height;
			found = true;
			break;
		}
		if (found && shelf.height == height)
			break;
	}
	if (selected_slot_index == null_index)
		return null_index;
	Shelf* shelf = &shelf_pool[selected_shelf_index];
	if (!shelf->allocated)
	{
		shelf->allocated = true;
		if (shelf->height - height >= split_threshold)
		{
			const unsigned int new_shelf_index = AllocateEntry<Shelf>(shelf_pool, next_free_shelf_index);
			const unsigned int new_slot_index = AllocateEntry<Slot>(slot_pool, next_free_slot_index);
			shelf = &shelf_pool[selected_shelf_index];
			slot_pool[new_slot_index] = {
				new_shelf_index, page.texture_id, 0, shelf->y + height, page_size, 0, 0, null_index, null_index, null_index, null_index, 0, false
			};
			shelf_pool[new_shelf_index] = {
				page_index, shelf->y + height, shelf->height - height,
				selected_shelf_index, shelf->next_index,  new_slot_index, new_slot_index, false
			};
			if (shelf->next_index != null_index)
				shelf_pool[shelf->next_index].previous_index = new_shelf_index;
			shelf->next_index = new_shelf_index;
			shelf->height = height;
		}
	}
	Slot* slot = &slot_pool[selected_slot_index];
	if (slot->width - width >= split_threshold)
	{
		const unsigned int new_slot_index = AllocateEntry<Slot>(slot_pool, next_free_slot_index);
		slot = &slot_pool[selected_slot_index];
		slot_pool[new_slot_index] = {
			selected_shelf_index, page.texture_id, slot->x + width, shelf->y, slot->width - width, 0, 0,
			selected_slot_index, slot->next_index, slot->previous_free_index, slot->next_free_index, 0, false
		};
		if (slot->next_index != null_index)
			slot_pool[slot->next_index].previous_index = new_slot_index;
		slot->next_index = new_slot_index;
		if (slot->previous_free_index == null_index)
			shelf->first_free_slot_index = new_slot_index;
		else
			slot_pool[slot->previous_free_index].next_free_index = new_slot_index;
		if (slot->next_free_index != null_index)
			slot_pool[slot->next_free_index].previous_free_index = new_slot_index;
		slot->width = width;
	}
	else
	{
		if (slot->previous_free_index == null_index)
			shelf->first_free_slot_index = slot->next_free_index;
		else
			slot_pool[slot->previous_free_index].next_free_index = slot->next_free_index;
		if (slot->next_free_index != null_index)
			slot_pool[slot->next_free_index].previous_free_index = slot->previous_free_index;
	}
	slot->allocated = true;
	slot->actual_width = width;
	slot->height = height;
	slot->epoch = current_epoch;
	if (page_index == first_page_index)
		first_page_allocated_pixels += width * shelf->height;
	return selected_slot_index;
}

unsigned int SpriteSet::Remove(const unsigned int slot_index)
{
	Slot& slot = slot_pool[slot_index];
	Shelf& shelf = shelf_pool[slot.shelf_index];
	const unsigned int page_index = shelf.page_index;
	Page& page = page_pool[page_index];

	/* DEBUG: Fill the removed area with blue.
	for (unsigned int offset_y = 0; offset_y < slot.height; ++offset_y)
	{
		for (unsigned int offset_x = 0; offset_x < slot.width; ++offset_x)
		{
			const auto pixel = page.texture_data->begin() + ((slot.y + offset_y) * page_size + slot.x + offset_x) * 4;
			pixel[0] = 0;
			pixel[1] = 0;
			pixel[2] = 255;
			pixel[3] = 255;
		}
	}
	*/

	const unsigned int slot_pixels = slot.width * slot.height;
	slot.allocated = false;
	slot.previous_free_index = null_index;
	if (shelf.first_free_slot_index == null_index)
	{
		slot.next_free_index = null_index;
	}
	else
	{
		slot.next_free_index = shelf.first_free_slot_index;
		slot_pool[shelf.first_free_slot_index].previous_free_index = slot_index;
	}
	shelf.first_free_slot_index = slot_index;
	++slot.epoch;

	// Merge consecutive empty slots.
	if (slot.next_index != null_index)
	{
		Slot& next_slot = slot_pool[slot.next_index];
		if (!next_slot.allocated)
		{
			slot.width += next_slot.width;
			const unsigned int next_index = slot.next_index;
			slot.next_index = next_slot.next_index;
			if (next_slot.previous_free_index != null_index)
				slot_pool[next_slot.previous_free_index].next_free_index = next_slot.next_free_index;
			if (next_slot.next_free_index != null_index)
				slot_pool[next_slot.next_free_index].previous_free_index = next_slot.previous_free_index;
			FreeEntry<Slot>(slot_pool, next_free_slot_index, next_index);
			if (slot.next_index != null_index)
				slot_pool[slot.next_index].previous_index = slot_index;
		}
	}
	if (slot.previous_index != null_index)
	{
		Slot& previous_slot = slot_pool[slot.previous_index];
		if (!previous_slot.allocated)
		{
			slot.x -= previous_slot.width;
			slot.width += previous_slot.width;
			const unsigned int previous_index = slot.previous_index;
			slot.previous_index = previous_slot.previous_index;
			if (previous_slot.previous_free_index != null_index)
				slot_pool[previous_slot.previous_free_index].next_free_index = previous_slot.next_free_index;
			if (previous_slot.next_free_index != null_index)
				slot_pool[previous_slot.next_free_index].previous_free_index = previous_slot.previous_free_index;
			FreeEntry<Slot>(slot_pool, next_free_slot_index, previous_index);
			if (slot.previous_index == null_index) {
				shelf.first_slot_index = slot_index;
				if (slot.next_index == null_index)
					shelf.allocated = false;
			}
			else
			{
				slot_pool[slot.previous_index].next_index = slot_index;
			}
		}
	}

	// Merge consecutive empty shelves.
	if (shelf.allocated)
		return slot_pixels;
	if (shelf.next_index != null_index)
	{
		Shelf& next_shelf = shelf_pool[shelf.next_index];
		if (!next_shelf.allocated)
		{
			shelf.height += next_shelf.height;
			const unsigned int next_index = shelf.next_index;
			shelf.next_index = next_shelf.next_index;
			FreeEntry<Slot>(slot_pool, next_free_slot_index, next_shelf.first_slot_index);
			FreeEntry<Shelf>(shelf_pool, next_free_shelf_index, next_index);
			if (shelf.next_index != null_index)
				shelf_pool[shelf.next_index].previous_index = slot.shelf_index;
		}
	}
	if (shelf.previous_index != null_index)
	{
		Shelf& previous_shelf = shelf_pool[shelf.previous_index];
		if (!previous_shelf.allocated)
		{
			shelf.y -= previous_shelf.height;
			shelf.height += previous_shelf.height;
			slot.y = shelf.y;
			const unsigned int previous_index = shelf.previous_index;
			shelf.previous_index = previous_shelf.previous_index;
			FreeEntry<Slot>(slot_pool, next_free_slot_index, previous_shelf.first_slot_index);
			FreeEntry<Shelf>(shelf_pool, next_free_shelf_index, previous_index);
			if (shelf.previous_index == null_index)
				page.first_shelf_index = slot.shelf_index;
			else
				shelf_pool[shelf.previous_index].next_index = slot.shelf_index;
		}
	}

	// Deallocate the page if it becomes empty, except when it's the first one.
	if (page_index == first_page_index)
	{
		first_page_allocated_pixels -= slot.width * shelf.height;
		return slot_pixels;
	}
	if (shelf.height != page_size)
		return slot_pixels;
	FreeEntry<Slot>(slot_pool, next_free_slot_index, slot_index);
	FreeEntry<Shelf>(shelf_pool, next_free_shelf_index, page.first_shelf_index);
	page.texture_data.reset();
	page_pool[page.previous_index].next_index = page.next_index;
	if (page_index != last_page_index)
		page_pool[page.next_index].previous_index = page.previous_index;
	page_pool[last_page_index].next_index = page_index;
	return slot_pixels;
}

void SpriteSet::Remove(const Handle handle)
{
	Slot& slot = slot_pool[handle.slot_index];
	if (slot.epoch != handle.epoch)
		return;
	Remove(handle.slot_index);
}

SpriteSet::SpriteData SpriteSet::Get(const Handle handle) const
{
	const Slot& slot = slot_pool[handle.slot_index];
	return {
		slot.texture_id, slot.x + sprite_padding, slot.y + sprite_padding,
		slot.actual_width - sprite_padding * 2, slot.height - sprite_padding * 2
	};
}

Vector<const unsigned char*> SpriteSet::GetTextures() const
{
	if (first_page_index == null_index)
		return {};
	Vector<const unsigned char*> textures;
	unsigned int page_index = first_page_index;
	while (true)
	{
		const Page& page = page_pool[page_index];
		textures.push_back(page.texture_data->data());
		if (page_index == last_page_index)
			break;
		page_index = page.next_index;
	}
	return textures;
}

} // namespace Rml
