#pragma once

#include<vector>
#include<optional>
#include<algorithm>

namespace de {

	class RangeAllocator {
	public:
		struct Allocation {
			uint32_t offset = 0;
			uint32_t count = 0;
		};

		explicit RangeAllocator(uint32_t capacity) : capacity_{ capacity } {
			if (capacity > 0) {
				freeList_.push_back({ 0,capacity });
			}
		}

		std::optional<Allocation> allocate(uint32_t count) {
			if (count == 0) { return Allocation{ 0,0 }; }

			for (size_t i = 0; i < freeList_.size(); i++) {
				if (freeList_[i].count >= count) {
					const Allocation result{ freeList_[i].offset, count };

					freeList_[i].offset += count;
					freeList_[i].count -= count;
					if (freeList_[i].count == 0) {
						freeList_.erase(freeList_.begin() + static_cast<long>(i));
					}
					return result;
				}
			}
			return std::nullopt;
		}

		void free(Allocation alloc) {
			if (alloc.count == 0) { return; }

			auto it = std::lower_bound(freeList_.begin(), freeList_.end(), alloc.offset,
				[](const Allocation& r, uint32_t offset) {return r.offset < offset; });

			it = freeList_.insert(it, alloc);

			if (auto next = std::next(it); next != freeList_.end() &&
				it->offset + it->count == next->offset) {
				it->count += next->count;
				freeList_.erase(next);
			}
			if (it != freeList_.begin()) {
				auto prev = std::prev(it);
				if (prev->offset + prev->count == it->offset) {
					prev->count += it->count;
					freeList_.erase(it);
				}
			}
		}

		void grow(uint32_t newCapacity) {
			if (newCapacity <= capacity_) { return; }

			const uint32_t added = newCapacity - capacity_;

			if (!freeList_.empty() &&
				freeList_.back().offset + freeList_.back().count == capacity_) {
				freeList_.back().count += added;
			}
			else {
				freeList_.push_back({ capacity_,added });
			}
			capacity_ = newCapacity;
		}

		uint32_t getCapacity() const noexcept { return capacity_; }

		uint32_t freeCount() const noexcept {
			uint32_t sum = 0;
			for (const auto& r : freeList_) { sum += r.count; }
			return sum;
		}

		size_t freeBlockCount() const noexcept { return freeList_.size(); }
	private:

		uint32_t capacity_;
		std::vector<Allocation> freeList_; //sorted by offset

	};
}