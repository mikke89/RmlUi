#ifndef GRAPHICS_LRULIST_H
#define GRAPHICS_LRULIST_H

#include <cstdlib>
#include <vector>

/*
	Implementation details:
	- The first entry has the previous index pointing to itself to simplify eviction of the last entry.
	- Invalidation is simply done by incrementing the entry's epoch. This is trivially safe.
*/

template<typename T>
class LruList final {
	private:
		struct Entry {
			unsigned previousIndex, nextIndex;
			unsigned epoch, lastUsed;
			T data;
		};

		std::vector<Entry> pool{1 << 10};
		std::size_t size = 0;
		unsigned headIndex = 0, tailIndex = 0;
		unsigned currentEpoch = 0;
	public:
		struct Handle {
			unsigned index;
			unsigned epoch;
		};

		LruList();
		void tick();
		Handle add(T data);
		bool isAlive(Handle handle);
		bool ping(Handle handle);
		void remove(Handle handle);
		void evictLast();
		T* getData(Handle handle);
		const T* getData(Handle handle) const;
		T* getLast();
		const T* getLast() const;
		unsigned getLastEntryAge() const;
};

template<typename T>
LruList<T>::LruList() {
	const unsigned poolSize = static_cast<unsigned>(pool.size());
	for (unsigned i = 0; i != poolSize; ++i) pool[i].nextIndex = i + 1;
}

template<typename T>
void LruList<T>::tick() {
	++currentEpoch;
}

template<typename T>
typename LruList<T>::Handle LruList<T>::add(const T data) {
	unsigned poolSize = static_cast<unsigned>(pool.size());
	if (size == poolSize) {
		poolSize <<= 1;
		pool.resize(poolSize);
		for (unsigned i = static_cast<unsigned>(size); i != poolSize; ++i) pool[i].nextIndex = i + 1;
		pool[tailIndex].nextIndex = static_cast<unsigned>(size);
	}

	if (size == 0) {
		Entry &newEntry = pool[headIndex];
		newEntry.previousIndex = headIndex;
		newEntry.epoch = currentEpoch;
		newEntry.lastUsed = currentEpoch;
		newEntry.data = data;
		size = 1;
		// No need to update head and tail indices here, they are already correct.
		return {headIndex, currentEpoch};
	}

	Entry &tail = pool[tailIndex];
	const unsigned newIndex = tail.nextIndex;
	Entry &newEntry = pool[newIndex];
	tail.nextIndex = newEntry.nextIndex;
	newEntry = {newIndex, headIndex, currentEpoch, currentEpoch, data};
	pool[headIndex].previousIndex = newIndex;
	headIndex = newIndex;
	++size;
	return {newIndex, currentEpoch};
}

template<typename T>
bool LruList<T>::isAlive(const Handle handle) {
	return pool[handle.index] == handle.epoch;
}

template<typename T>
bool LruList<T>::ping(const Handle handle) {
	Entry &entry = pool[handle.index];
	if (entry.epoch != handle.epoch) return false;
	if (entry.lastUsed == currentEpoch) return true;
	entry.lastUsed = currentEpoch;
	if (handle.index == headIndex) return true;
	pool[entry.previousIndex].nextIndex = entry.nextIndex;
	if (handle.index == tailIndex) tailIndex = entry.previousIndex;
	else pool[entry.nextIndex].previousIndex = entry.previousIndex;
	pool[headIndex].previousIndex = handle.index;
	entry.previousIndex = handle.index;
	entry.nextIndex = headIndex;
	headIndex = handle.index;
	return true;
}

template<typename T>
void LruList<T>::remove(const Handle handle) {
	Entry &entry = pool[handle.index];
	if (entry.epoch != handle.epoch) return;
	++entry.epoch;
	if (size == 1) {
		size = 0;
		return;
	}
	if (handle.index == headIndex) {
		Entry &tail = pool[tailIndex];
		entry.nextIndex = tail.nextIndex;
		tail.nextIndex = headIndex;
		headIndex = entry.nextIndex;
	} else if (handle.index == tailIndex) {
		tailIndex = entry.previousIndex;
	} else {
		pool[entry.previousIndex].nextIndex = entry.nextIndex;
		pool[entry.nextIndex].previousIndex = entry.previousIndex;
		Entry &tail = pool[tailIndex];
		entry.nextIndex = tail.nextIndex;
		tail.nextIndex = handle.index;
	}
	--size;
}

template<typename T>
void LruList<T>::evictLast() {
	if (size == 0) return;
	Entry &entry = pool[tailIndex];
	++entry.epoch;
	tailIndex = entry.previousIndex;
	--size;
}

template<typename T>
T* LruList<T>::getData(const Handle handle) {
	Entry &entry = pool[handle.index];
	return entry.epoch == handle.epoch ? &entry.data : nullptr;
}

template<typename T>
const T* LruList<T>::getData(const Handle handle) const {
	Entry &entry = pool[handle.index];
	return entry.epoch == handle.epoch ? &entry.data : nullptr;
}

template<typename T>
T* LruList<T>::getLast() {
	return size == 0 ? nullptr : &pool[tailIndex].data;
}

template<typename T>
const T* LruList<T>::getLast() const {
	return size == 0 ? nullptr : &pool[tailIndex].data;
}

template<typename T>
unsigned LruList<T>::getLastEntryAge() const {
	return size == 0 ? 0 : currentEpoch - pool[tailIndex].lastUsed;
}

#endif // GRAPHICS_LRULIST_H
