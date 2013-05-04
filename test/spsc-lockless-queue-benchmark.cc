#include <assert.h>
#include <queue>
#include <random>
#include <thread>

#include "hayai.hpp"
#include "single-producer-single-consumer-bounded-lockless-queue.h"
#include "single-producer-single-consumer-lockless-queue.h"

using lockless::SingleProducerSingleConsumerBoundedLockLessQueue;
using lockless::SingleProducerSingleConsumerLockLessQueue;
using std::thread;
using std::uniform_int_distribution;
using std::unique_ptr;


template <typename Queue>
class QueueTest {
public:
	QueueTest() {
		Init();
	}

	explicit QueueTest(int capacity) : queue_(capacity) {
		Init();
	}

	virtual ~QueueTest() {}

	void Init() {
		push_count_ = 0;
		pop_count_ = 0;
		num_iterations_ = 1000 * 1000;
		distribution_.reset(
				new uniform_int_distribution<int>(1, 10000));
	}

	virtual void RandomPush() {
		auto random_number_gen = std::bind(*distribution_, generator_);
		for (int i = 0; i < num_iterations_; ++i) {
			int random_number = random_number_gen();
			queue_.Push(random_number);
			push_count_ += random_number;
		}
	}

	virtual void RandomPushWithSpin() {}

	virtual void RandomPop() {
		int num_times = 0;
		while (num_times < num_iterations_) {
			int value;
			if (queue_.Pop(&value)) {
				pop_count_ += value;
				num_times += 1;
			}
		}
	}

	Queue queue_;
	int num_iterations_;
	long push_count_;
	long pop_count_;
	std::default_random_engine generator_;
	unique_ptr<uniform_int_distribution<int> > distribution_;
};

template <typename BoundedQueue>
class BoundedQueueTest : public QueueTest<BoundedQueue> {
public:
	typedef QueueTest<BoundedQueue> Parent;

	explicit BoundedQueueTest(int capacity) :
		QueueTest<BoundedQueue>(capacity) {}

	virtual ~BoundedQueueTest() {}

	virtual void RandomPushWithSpin() {
		auto random_number_gen = std::bind(*Parent::distribution_,
				Parent::generator_);
		for (int i = 0; i < Parent::num_iterations_; ++i) {
			int random_number = random_number_gen();
			while (!Parent::queue_.Push(random_number))
				;  // spin
			Parent::push_count_ += random_number;
		}
	}
};

typedef BoundedQueueTest<SingleProducerSingleConsumerBoundedLockLessQueue<int> >
    SPSCBoundedLockLessQueueTest;
typedef QueueTest<SingleProducerSingleConsumerLockLessQueue<int> >
    SPSCLockLessQueueTest;


template <typename Value>
class LockingQueue {
public:
	LockingQueue() {}
	~LockingQueue() {}

	void Push(Value value) {
		std::lock_guard<std::mutex> guard(mutex_);
		queue_.push(value);
	}

	bool Pop(Value* value) {
		std::lock_guard<std::mutex> guard(mutex_);
		if (!queue_.empty()) {
			*value = queue_.front();
			queue_.pop();
			return true;
		}
		return false;
	}

private:
	std::queue<Value> queue_;
	std::mutex mutex_;
};

typedef QueueTest<LockingQueue<int> > LockingQueueTest;


class SPSCLocklessQueueBenchMarkTest : public Hayai::Fixture {
public:
	SPSCLocklessQueueBenchMarkTest() {}
};

BENCHMARK_F(SPSCLocklessQueueBenchMarkTest, UnBoundedLockLessRandomData, 50, 1) {
	SPSCLockLessQueueTest queue;
	thread push_thread(&SPSCLockLessQueueTest::RandomPush, &queue);
	thread pop_thread(&SPSCLockLessQueueTest::RandomPop, &queue);
	push_thread.join();
	pop_thread.join();
	assert(queue.push_count_ == queue.pop_count_);
}

BENCHMARK_F(SPSCLocklessQueueBenchMarkTest, BoundedLockLessRandomData, 50, 1) {
	unique_ptr<SPSCBoundedLockLessQueueTest> queue(new SPSCBoundedLockLessQueueTest(32));
	thread push_thread(&SPSCBoundedLockLessQueueTest::RandomPushWithSpin, queue.get());
	thread pop_thread(&SPSCBoundedLockLessQueueTest::RandomPop, queue.get());
	push_thread.join();
	pop_thread.join();
	assert(queue->push_count_ == queue->pop_count_);
}

BENCHMARK_F(SPSCLocklessQueueBenchMarkTest, LockingRandomData, 50, 1) {
	LockingQueueTest queue;
	thread push_thread(&LockingQueueTest::RandomPush, &queue);
	thread pop_thread(&LockingQueueTest::RandomPop, &queue);
	push_thread.join();
	pop_thread.join();
	assert(queue.push_count_ == queue.pop_count_);
}


int main() {
	Hayai::Benchmarker::RunAllTests();
	return 0;
}
