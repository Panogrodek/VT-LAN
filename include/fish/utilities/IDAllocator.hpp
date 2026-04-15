#pragma once
#include <stack>
#include <unordered_set>
#include <cstdint>

namespace fs {

	class IDAllocator {
	public:
		explicit IDAllocator(uint32_t startID = 1)
			: nextID(startID ? startID : 1) {
		}

		uint32_t Acquire() {
			// 1) Reuse only explicitly released IDs
			while (!freeIDs.empty()) {
				uint32_t id = freeIDs.top();
				freeIDs.pop();

				// Skip stale frees (e.g. freed, then occupied again)
				if (id != 0 && occupied.insert(id).second) {
					return id;
				}
			}

			// 2) Otherwise allocate monotonically increasing ids
			while (nextID == 0 || occupied.count(nextID) != 0) {
				++nextID;
			}

			uint32_t id = nextID++;
			occupied.insert(id);
			return id;
		}

		void Release(uint32_t id) {
			if (id == 0) return;

			auto it = occupied.find(id);
			if (it == occupied.end()) return; // only release what is actually occupied

			occupied.erase(it);
			freeIDs.push(id);
		}

		// Occupy a slot so Acquire will never hand it out as a "new" id.
		// (It can still appear in freeIDs as a stale entry; Acquire skips it.)
		void Occupy(uint32_t id) {
			if (id == 0) return;

			occupied.insert(id);

			// Critical: prevent future "new" allocations from returning this id
			if (id >= nextID) {
				nextID = id + 1;
				if (nextID == 0) ++nextID; // handle wrap to 0
			}
		}

		bool IsOccupied(uint32_t id) const {
			return id != 0 && occupied.count(id) != 0;
		}

	private:
		uint32_t nextID = 1;
		std::stack<uint32_t> freeIDs;
		std::unordered_set<uint32_t> occupied;
	};

}
