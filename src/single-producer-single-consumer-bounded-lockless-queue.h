// A single-producer single-consumer bounded lockless queue. Lockless queues
// have the advantage over locking queues in having much lower jitter for queue
// operations. Under the hood, the queue uses the C++0x atomic library and
// acquire-release memory ordering.

#ifndef SINGLE_PRODUCER_SINGLE_CONSUMER_BOUNDED_LOCKLESS_QUEUE_H_
#define SINGLE_PRODUCER_SINGLE_CONSUMER_BOUNDED_LOCKLESS_QUEUE_H_

#include <atomic>

using std::atomic;
using std::memory_order_acquire;
using std::memory_order_release;

namespace lockless {

template <typename Value>
class SingleProducerSingleConsumerBoundedLockLessQueue {
public:
	static const int kDefaultCapacity = 16;

	//Use kDefaultCapacity as the default queue capacity.
	SingleProducerSingleConsumerBoundedLockLessQueue();

	// Specify the memory capacity of the queue. The actual capacity is one
	// less.
	explicit SingleProducerSingleConsumerBoundedLockLessQueue(
			int capacity);

	~SingleProducerSingleConsumerBoundedLockLessQueue();

	// Get the capacity of the queue.
	int GetCapacity() const;

	// Returns whether this queue is lock free given the type of Value.
	bool IsLockFree() const;

	// Add a value to the end of the queue. Call on the producer thread only.
	// If there is no capacity to add value to the queue, false is returned.
	bool Push(Value value);

	// Remove a value from the front of the queue. Call on the consumer thread
	// only. Returns true iff there was a value to remove. Value must not be
	// null.
	bool Pop(Value* value);

	// Returns whether the queue is empty or not. Valid on the consumer thread.
	bool IsEmpty() const;

	// Returns whether the queue is full or not. Valid on the producer thread.
	bool IsFull() const;

private:
	void Init();

	// Advance the index by 1 in a circular queue.
	int Advance(int index) const;

	// Indexing in the queue works as follows. When the first_ and next_write_
	// have the same value, the queue is empty. When the next_write_ is one
	// behind the first_ (modulo the size), then the queue is full. The
	// actual queue capacity is one less than capacity_: there is an unused
	// space.

	// The index of the first element in the queue.
	atomic<int> first_;

	// The index of the element one after the last element in the queue.
	atomic<int> next_write_;

	// The capacity of the buffer. The actual store capacity is one less.
	int capacity_;

	// A circular queue.
	atomic<Value>* queue_;
};

// A convenience macro to keep lines short.
#define SPSC_BLLQ SingleProducerSingleConsumerBoundedLockLessQueue

template <typename Value>
SPSC_BLLQ<Value>::SingleProducerSingleConsumerBoundedLockLessQueue() :
		capacity_(kDefaultCapacity) {
	Init();
}

template <typename Value>
SPSC_BLLQ<Value>::SingleProducerSingleConsumerBoundedLockLessQueue(
		int capacity) : capacity_(capacity) {
	Init();
}

template <typename Value>
void SPSC_BLLQ<Value>::Init() {
	queue_ = new atomic<Value>[capacity_];
	first_ = 0;
	next_write_ = 0;
}

template <typename Value>
int SPSC_BLLQ<Value>::Advance(int index) const {
	++index;
	if (index < capacity_) return index;
	return 0;
}

template <typename Value>
SPSC_BLLQ<Value>::~SingleProducerSingleConsumerBoundedLockLessQueue() {
	delete [] queue_;
}

template <typename Value>
bool SPSC_BLLQ<Value>::IsLockFree() const {
	return queue_[0].is_lock_free() && first_.is_lock_free();
}

template <typename Value>
int SPSC_BLLQ<Value>::GetCapacity() const {
	return capacity_ - 1;
}

template <typename Value>
bool SPSC_BLLQ<Value>::Push(Value value) {
	int first = first_.load(memory_order_acquire);
	int next_write = next_write_.load(memory_order_acquire);
	int next_write_next = Advance(next_write);
	if (first == next_write_next) return false;
	queue_[next_write].store(value, memory_order_release);
	next_write_.store(next_write_next, memory_order_release);
}

template <typename Value>
bool SPSC_BLLQ<Value>::Pop(Value* return_value) {
	int first = first_.load(memory_order_acquire);
	int next_write = next_write_.load(memory_order_acquire);
	if (first == next_write) return false;
	*return_value = queue_[first].load(memory_order_acquire);
	first_.store(Advance(first), memory_order_release);
	return true;
}

template <typename Value>
bool SPSC_BLLQ<Value>::IsEmpty() const {
	return first_.load(memory_order_acquire) ==
			next_write_.load(memory_order_acquire);
}

template <typename Value>
bool SPSC_BLLQ<Value>::IsFull() const {
	return first_.load(memory_order_acquire) ==
			Advance(next_write_.load(memory_order_acquire));
}

#undef SPSC_BLLQ

}  // lockless

#endif  // SINGLE_PRODUCER_SINGLE_CONSUMER_BOUNDED_LOCKLESS_QUEUE_H_

