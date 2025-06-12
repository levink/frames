#include <gtest/gtest.h>
#include "util/circlebuffer.h"

TEST(HelloTest, CircleBuffer_accessByIndex) {	
	CircleBuffer<int, 3, 0> buf;
	ASSERT_FALSE(buf.full());

	buf.pushBack(1);
	buf.pushBack(2);
	buf.pushBack(3);
	ASSERT_TRUE(buf.full());

	ASSERT_EQ(1, buf[0]);
	ASSERT_EQ(2, buf[1]);
	ASSERT_EQ(3, buf[2]);

	int prev = buf.pushBack(4);
	ASSERT_EQ(1, prev);
	ASSERT_EQ(2, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(4, buf[2]);

	prev = buf.pushBack(5);
	ASSERT_EQ(2, prev);
	ASSERT_EQ(3, buf[0]);
	ASSERT_EQ(4, buf[1]);
	ASSERT_EQ(5, buf[2]);
}

TEST(HelloTest, CircleBuffer_pushBack) {
	CircleBuffer<int, 3, 0> buf;
	int prev = buf.pushBack(1);
	ASSERT_FALSE(buf.full());
	ASSERT_EQ(1, buf.size());
	ASSERT_EQ(0, prev);
	ASSERT_EQ(1, buf[0]);

	prev = buf.pushBack(2);
	ASSERT_FALSE(buf.full());
	ASSERT_EQ(2, buf.size());
	ASSERT_EQ(0, prev);
	ASSERT_EQ(1, buf[0]);
	ASSERT_EQ(2, buf[1]);

	prev = buf.pushBack(3);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(0, prev);
	ASSERT_EQ(1, buf[0]);
	ASSERT_EQ(2, buf[1]);
	ASSERT_EQ(3, buf[2]);

	prev = buf.pushBack(4);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(1, prev);
	ASSERT_EQ(2, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(4, buf[2]);
	
	prev = buf.pushBack(5);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(2, prev);
	ASSERT_EQ(3, buf[0]);
	ASSERT_EQ(4, buf[1]);
	ASSERT_EQ(5, buf[2]);

	prev = buf.pushBack(6);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(3, prev);
	ASSERT_EQ(4, buf[0]);
	ASSERT_EQ(5, buf[1]);
	ASSERT_EQ(6, buf[2]);
}

TEST(HelloTest, CircleBuffer_pushFront) {
	CircleBuffer<int, 3, 0> buf;
	int prev = buf.pushFront(1);
	ASSERT_FALSE(buf.full());
	ASSERT_EQ(1, buf.size());
	ASSERT_EQ(1, buf[0]);
	ASSERT_EQ(0, prev);

	prev = buf.pushFront(2);
	ASSERT_FALSE(buf.full());
	ASSERT_EQ(2, buf.size());
	ASSERT_EQ(2, buf[0]);
	ASSERT_EQ(1, buf[1]);
	ASSERT_EQ(0, prev);

	prev = buf.pushFront(3);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(3, buf[0]);
	ASSERT_EQ(2, buf[1]);
	ASSERT_EQ(1, buf[2]);
	ASSERT_EQ(0, prev);

	prev = buf.pushFront(4);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(4, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(2, buf[2]);
	ASSERT_EQ(1, prev);

	prev = buf.pushFront(5);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(5, buf[0]);
	ASSERT_EQ(4, buf[1]);
	ASSERT_EQ(3, buf[2]);
	ASSERT_EQ(2, prev);

	prev = buf.pushFront(6);
	ASSERT_TRUE(buf.full());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(6, buf[0]);
	ASSERT_EQ(5, buf[1]);
	ASSERT_EQ(4, buf[2]);
	ASSERT_EQ(3, prev);
}

TEST(HelloTest, CircleBuffer_pushTwoSides) {
	CircleBuffer<int, 3, 0> buf;

	int prev = buf.pushBack(1); // [1] -> 0
	ASSERT_EQ(1, buf[0]);
	ASSERT_EQ(0, prev);

	prev = buf.pushBack(2); // [12] -> 0
	ASSERT_EQ(1, buf[0]);
	ASSERT_EQ(2, buf[1]);
	ASSERT_EQ(0, prev);

	prev = buf.pushFront(3); // 0 <- [312]
	ASSERT_EQ(0, prev);
	ASSERT_EQ(3, buf[0]);
	ASSERT_EQ(1, buf[1]);
	ASSERT_EQ(2, buf[2]);

	prev = buf.pushFront(4); // [431] -> 2
	ASSERT_EQ(4, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(1, buf[2]);
	ASSERT_EQ(2, prev);

	prev = buf.pushFront(5); // [543] -> 1
	ASSERT_EQ(5, buf[0]);
	ASSERT_EQ(4, buf[1]);
	ASSERT_EQ(3, buf[2]);
	ASSERT_EQ(1, prev);

	prev = buf.pushBack(6); // 5 <- [436]
	ASSERT_EQ(5, prev);
	ASSERT_EQ(4, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(6, buf[2]);

	prev = buf.pushBack(7); // 4 <- [367]
	ASSERT_EQ(4, prev);
	ASSERT_EQ(3, buf[0]);
	ASSERT_EQ(6, buf[1]);
	ASSERT_EQ(7, buf[2]);

	prev = buf.pushFront(8); // [836] -> 7
	ASSERT_EQ(8, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(6, buf[2]);
	ASSERT_EQ(7, prev);
}

TEST(HelloTest, CircleBuffer_popBack) {
	CircleBuffer<int, 3, 0> buf;
	ASSERT_TRUE(buf.empty());

	buf.pushBack(1);
	buf.pushBack(2);
	buf.pushBack(3);
	buf.pushBack(4);
	ASSERT_FALSE(buf.empty());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(2, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(4, buf[2]);

	int prev = buf.popBack();
	ASSERT_FALSE(buf.empty());
	ASSERT_EQ(2, buf.size());
	ASSERT_EQ(2, buf[0]);
	ASSERT_EQ(3, buf[1]);
	ASSERT_EQ(4, prev);

	prev = buf.popBack();
	ASSERT_FALSE(buf.empty());
	ASSERT_EQ(1, buf.size());
	ASSERT_EQ(2, buf[0]);
	ASSERT_EQ(3, prev);

	prev = buf.popBack();
	ASSERT_TRUE(buf.empty());
	ASSERT_EQ(0, buf.size());
	ASSERT_EQ(2, prev);

	prev = buf.popBack();
	ASSERT_TRUE(buf.empty());
	ASSERT_EQ(0, buf.size());
	ASSERT_EQ(0, prev);
}

TEST(HelloTest, CircleBuffer_popFront) {
	CircleBuffer<int, 3, 0> buf;
	ASSERT_TRUE(buf.empty());

	buf.pushBack(1);
	buf.pushBack(2);
	buf.pushBack(3);
	buf.pushBack(4);
	buf.pushBack(5);
	ASSERT_FALSE(buf.empty());
	ASSERT_EQ(3, buf.size());
	ASSERT_EQ(3, buf[0]);
	ASSERT_EQ(4, buf[1]);
	ASSERT_EQ(5, buf[2]);

	ASSERT_EQ(3, buf.popFront());
	ASSERT_EQ(4, buf[0]);
	ASSERT_EQ(5, buf[1]);
	ASSERT_EQ(4, buf.popFront());
	ASSERT_EQ(5, buf.popFront());
	ASSERT_EQ(0, buf.popFront());
	ASSERT_EQ(0, buf.popFront());
}
