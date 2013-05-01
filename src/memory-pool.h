// Since memory allocation is generally not lock-free, we use pre-allocated
// chunks of memory of some type. This memory pool assumes that memory
// allocation and deallocation happens on the same thread, as with the
// SingleProducerSingleConsumerLockLessQueue. The initial minimum
// memory allocation is specified in the ctor. In the course of use, if
// memory is requested and the MemoryPool does not have any more,
// a system call is made to obtain another chunk of memory. Strictly speaking
// this may make the lock-free queue generally lock free: there may be an
// occasional latency blip via requesting additional memory from the system.
// All memory returned from this class is also cleaned up by it.

#ifndef MEMORY_POOL_H_
#define MEMORY_POOL_H_

#include <list>
#include <vector>

namespace lockless {

template <typename T>
class MemoryPool {
public:
	explicit MemoryPool(int initial_capacity);
	~MemoryPool();

	// Allocate a location for T. The MemoryPool retains ownership of the
	// pointer.
	T* Allocate();

	// Free a location T which is no longer needed.
	void Free(T* t);

private:
	void AddCapacity();

	std::vector<T*> memory_;
	std::list<T*> memory_chunk_starting_locations_;
	int capacity_increment_;
};

template <typename T>
MemoryPool<T>::MemoryPool(int initial_capacity) {
	memory_.reserve(initial_capacity);
	capacity_increment_ = memory_.capacity();
	AddCapacity();
}

template <typename T>
MemoryPool<T>::~MemoryPool() {
	for (auto* memory_chunk : memory_chunk_starting_locations_) {
		delete [] memory_chunk;
	}
}

template <typename T>
void MemoryPool<T>::AddCapacity() {
	T* t_array = new T[capacity_increment_];
	for (int i = 0; i < capacity_increment_; ++i) {
		memory_.push_back(&t_array[i]);
	}
	memory_chunk_starting_locations_.push_back(t_array);
}

template <typename T>
void MemoryPool<T>::Free(T* t) {
	memory_.push_back(t);
}

template <typename T>
T* MemoryPool<T>::Allocate() {
	if (memory_.size() == 0) {
		memory_.reserve(memory_.capacity() + capacity_increment_);
		AddCapacity();
	}
	T* free_location = memory_.back();
	memory_.pop_back();
	return free_location;
}

}  // lockless

#endif  // MEMORY_POOL_H_
