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

	// Specify the minimum initial capacity for the memory pool.
	explicit SingleProducerSingleConsumerBoundedLockLessQueue(
			int initial_capacity);

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

private:
	void Init();

	// Advance the index by 1 in a circular queue.
	static int Advance(int index);

	// The index of the first element in the queue, provided the index is not
	// equal to the last.
	atomic<int> first_;

	// The index of the last element in the queue. Queue insertion happens at
	// the next element.
	atomic<int> last_;

	// The capacity of the queue.
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
		int initial_capacity) : capacity_(initial_capacity) {
	Init();
}

template <typename Value>
void SPSC_BLLQ<Value>::Init() {
	queue_ = new atomic<Value>[capacity_];
	first_ = last_ = 0;
}

template <typename Value>
int SPSC_BLLQ<Value>::Advance(int index) {
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
	return capacity_;
}

template <typename Value>
bool SPSC_BLLQ<Value>::Push(Value value) {
	int new_last = Advance(last_.load(memory_order_acquire));
	if (new_last == first_.load(memory_order_acquire)) {
		return false;
	}
	queue_[new_last].store(value, memory_order_release);
	last_.store(new_last, memory_order_release);
	return true;
}

template <typename Value>
bool SPSC_BLLQ<Value>::Pop(Value* return_value) {
	int first = first_.load(memory_order_acquire);
	if (first == last_.load(memory_order_acquire)) return false;
	*return_value = queue_[first].load(memory_order_acquire);
	first_.store(Advance(first), memory_order_release);
	return true;
}

template <typename Value>
bool SPSC_BLLQ<Value>::IsEmpty() const {
	return first_.load(memory_order_acquire) ==
			last_.load(memory_order_acquire);
}

#undef SPSC_LLQ

}  // lockless

#endif  // SINGLE_PRODUCER_SINGLE_CONSUMER_BOUNDED_LOCKLESS_QUEUE_H_

