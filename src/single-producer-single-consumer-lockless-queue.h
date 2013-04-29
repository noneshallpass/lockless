// A single-producer single-consumer lockless queue. Lockless queues have the
// advantage over locking queues in having much lower jitter for queue
// operations. Under the hood, the queue uses the C++0x atomic library and
// acquire-release memory ordering.

#ifndef SINGLE_PRODUCER_SINGLE_CONSUMER_LOCKLESS_QUEUE_H_
#define SINGLE_PRODUCER_SINGLE_CONSUMER_LOCKLESS_QUEUE_H_

#include <atomic>

using std::atomic;
using std::memory_order_acquire;
using std::memory_order_release;

namespace lockless {

template <typename Value>
class SingleProducerSingleConsumerLockLessQueue {
public:
	SingleProducerSingleConsumerLockLessQueue();
	~SingleProducerSingleConsumerLockLessQueue();

	// Returns whether this queue is lock free given the type of Value.
	bool IsLockFree() const;

	// Add a value to the end of the queue. Call on the producer thread only.
	void Push(Value value);

	// Remove a value to the front of the queue. Call on the consumer thread
	// only. Returns true iff there was a value to remove.
	bool Pop(Value* value);

	// Returns whether the queue is empty or not. Valid on the consumer thread.
	bool IsEmpty() const;

private:
	struct Node {
		Node(Value new_value): value(new_value), next(nullptr) {}
		Value value;
		Node* next;
	};

	void FreeQueueUntil(Node* until_ptr);

	// The nodes from first_ until divider_ (excluding the divider) are
	// consumed nodes which are to be deleted. These consumed nodes are
	// removed on the producer thread b/c this thread does not advance
	// the divider: whatever divider value is read delimits nodes
	// to-be-removed. The producer owns these nodes and also the last_ node
	// except for last_.value, which is owned by the consumer.
	//
	// The next value to be read from the queue is that immediately after
	// the divider. The end of the queue is indicated by last_. The consumer
	// owns the divider and all nodes through but excluding last_. The
	// consumer also owns last_.value.
	//
	// Parts of divider_ and last_ are touched by the consumer and producer
	// and so must be atomic. Since the first_ node is only touched by the
	// producer, it does not need to be atomic.
	Node* first_;
	atomic<Node*> divider_;
	atomic<Node*> last_;
};

// A convenience macro to keep lines short.
#define SPSC_LFQ SingleProducerSingleConsumerLockLessQueue

template <typename Value>
SPSC_LFQ<Value>::SingleProducerSingleConsumerLockLessQueue() {
	first_ = divider_ = last_ = new Node(Value());
}

template <typename Value>
SPSC_LFQ<Value>::~SingleProducerSingleConsumerLockLessQueue() {
	FreeQueueUntil(nullptr);
}

template <typename Value>
bool SPSC_LFQ<Value>::IsLockFree() const {
	return divider_.is_lock_free();
}

template <typename Value>
void SPSC_LFQ<Value>::FreeQueueUntil(Node* until_node) {
	while (first_ != until_node) {
		Node* current = first_;
		first_ = first_->next;
		delete current;
	}
}

template <typename Value>
void SPSC_LFQ<Value>::Push(Value value) {
	Node* last = last_.load(memory_order_acquire);
	last->next = new Node(value);
	last_.store(last->next, memory_order_release);
	FreeQueueUntil(divider_.load(memory_order_acquire));
}

template <typename Value>
bool SPSC_LFQ<Value>::Pop(Value* return_value) {
	Node* div = divider_.load(memory_order_acquire);
	if (div != last_.load(memory_order_acquire)) {
		*return_value = div->next->value;
		divider_.store(div->next, memory_order_release);
		return true;
	}
	return false;
}

template <typename Value>
bool SPSC_LFQ<Value>::IsEmpty() const {
	return divider_.load(memory_order_acquire) !=
			last_.load(memory_order_acquire);
}

#undef SPSC_LFQ

}  // lockless

#endif  // SINGLE_PRODUCER_SINGLE_CONSUMER_LOCKLESS_QUEUE_H_
