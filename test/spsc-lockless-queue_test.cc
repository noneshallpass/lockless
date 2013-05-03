#include "single-producer-single-consumer-bounded-lockless-queue.h"
#include "single-producer-single-consumer-lockless-queue.h"

#include <memory>
#include "gtest/gtest.h"

using lockless::SingleProducerSingleConsumerLockLessQueue;
using lockless::SingleProducerSingleConsumerBoundedLockLessQueue;
using std::unique_ptr;

typedef SingleProducerSingleConsumerLockLessQueue<int> SPSCLockLessQueue;
typedef SingleProducerSingleConsumerBoundedLockLessQueue<int>
  SPSCBoundedLockLessQueue;

// Tests of both the bounded and  non-bounded lockless queues.
template <typename Queue>
class QueueTest {
public:
	QueueTest() {}
	explicit QueueTest(int capacity, int num_loops) :
			queue_(capacity), num_loops_(num_loops) {}

	virtual ~QueueTest() {}

    void TestPopEmpty() {
    	int value = -1;
    	EXPECT_FALSE(queue_.Pop(&value));
    	EXPECT_EQ(-1, value);
    }

    void TestIsLockFree() {
    	EXPECT_TRUE(queue_.IsLockFree());
    }

    void TestPushAllThenPop() {
    	int value;
    	for (int i = 0; i < num_loops_; ++i) {
    		if (i == 0) {
    			EXPECT_TRUE(queue_.IsEmpty());
    		}
    		queue_.Push(i);
    		EXPECT_FALSE(queue_.IsEmpty());
    	}
    	for (int i = 0; i < num_loops_; ++i) {
    		value = -1;
    		EXPECT_TRUE(queue_.Pop(&value));
    		EXPECT_EQ(i, value);
    		if (i == num_loops_ - 1) {
    			EXPECT_TRUE(queue_.IsEmpty());
    		} else {
    			EXPECT_FALSE(queue_.IsEmpty());
    		}
    	}
    	// Cannot pop from an empty queue.
    	value = -1;
    	EXPECT_FALSE(queue_.Pop(&value));
    }

    void TestPushTwicePerPop() {
    	for (int i = 0; i < num_loops_; ++i) {
    		if (i == 0) {
    			EXPECT_TRUE(queue_.IsEmpty());
    		}
    		queue_.Push(i);
    		queue_.Push(-i);
    		EXPECT_FALSE(queue_.IsEmpty());
    		int value;
    		EXPECT_TRUE(queue_.Pop(&value));
    		int expected_value = (i % 2) ? -i/2 : i/2;
    		EXPECT_EQ(expected_value, value);
    		EXPECT_FALSE(queue_.IsEmpty());
    	}
    }

    // Valid only for the bounded queue when num_loops = capacity - 1.
    void TestCannotPushWhenFull() {
       	for (int i = 0; i < num_loops_; ++i) {
       		EXPECT_FALSE(queue_.IsFull());
        	EXPECT_TRUE(queue_.Push(i));
        }
       	EXPECT_TRUE(queue_.IsFull());
       	EXPECT_FALSE(queue_.Push(1));
       	int value = -1;
       	EXPECT_TRUE(queue_.Pop(&value));
       	EXPECT_EQ(0, value);
    }

    void TestGetCapacity(int capacity) {
    	EXPECT_EQ(capacity, queue_.GetCapacity());
    }

private:
	Queue queue_;
	int num_loops_;
};


class SPSCLockLessQueueTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		SetTestParams(64, 64);
	}

	void SetTestParams(int capacity, int num_loops) {
		queue_.reset(new QueueTest<SPSCLockLessQueue>(capacity, num_loops));
	}

	unique_ptr<QueueTest<SPSCLockLessQueue> > queue_;
};

TEST_F(SPSCLockLessQueueTest, TestPopEmpty) {
	queue_->TestPopEmpty();
}

TEST_F(SPSCLockLessQueueTest, TestIsLockFree) {
	queue_->TestIsLockFree();
}

TEST_F(SPSCLockLessQueueTest, TestPushAllThenPop) {
	queue_->TestPushAllThenPop();
	SetTestParams(16, 64);
	queue_->TestPushAllThenPop();
	SetTestParams(64, 16);
	queue_->TestPushAllThenPop();
}

TEST_F(SPSCLockLessQueueTest, TestPushTwicePerPop) {
	queue_->TestPushTwicePerPop();
	SetTestParams(16, 64);
	queue_->TestPushTwicePerPop();
	SetTestParams(64, 16);
	queue_->TestPushTwicePerPop();
}


class SPSCBoundedLockLessQueueTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		SetTestParams(64, 32);
	}

	void SetTestParams(int capacity, int num_loops) {
		queue_.reset(new QueueTest<SPSCBoundedLockLessQueue>(
				capacity, num_loops));
	}

	unique_ptr<QueueTest<SPSCBoundedLockLessQueue> > queue_;
};

TEST_F(SPSCBoundedLockLessQueueTest, TestPopEmpty) {
	queue_->TestPopEmpty();
}

TEST_F(SPSCBoundedLockLessQueueTest, TestIsLockFree) {
	queue_->TestIsLockFree();
}

TEST_F(SPSCBoundedLockLessQueueTest, TestPushAllThenPop) {
	queue_->TestPushAllThenPop();
	// The capacity of the bounded queue is one less than the actual buffer
	// size.
	SetTestParams(64, 63);
	queue_->TestPushAllThenPop();
}

TEST_F(SPSCBoundedLockLessQueueTest, TestPushTwicePerPop) {
	queue_->TestPushTwicePerPop();
}

TEST_F(SPSCBoundedLockLessQueueTest, TestCannotPushWhenFull) {
	SetTestParams(16, 15);
	queue_->TestCannotPushWhenFull();
}

TEST_F(SPSCBoundedLockLessQueueTest, TestGetCapacity) {
	queue_->TestGetCapacity(63);
}
