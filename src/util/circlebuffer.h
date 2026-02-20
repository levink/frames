#pragma once
#include <array>
#include <sstream>

/*
	Fixed-size buffer which allows shifting elements
	Pops element or default from left when pushing back
	Pops element or default from right when pushing front

	Example:

	CircleBuffer<int, 3, 0> buf; // buf == [0, 0, 0]
	buf.pushBack(1);			 // buf == [1, 0, 0]
	buf.pushBack(2);			 // buf == [1, 2, 0]
	buf.pushBack(3);			 // buf == [1, 2, 3]

	int left = buf.pushBack(4);
	// left == 1
	// buf == [2, 3, 4]

	int right = buf.pushFront(1);
	// right == 4
	// buf == [1, 2, 3]

	Good luck. (c) Levin K.
*/

template<typename T, size_t capacity, T defaultValue>
class CircleBuffer {

	std::array<T, capacity> items;
	size_t count;
	size_t head;	// First element position
	size_t tail;	// Next position after last element

public:
	CircleBuffer() : 
		items({ defaultValue }), 
		count(0), 
		head(0), 
		tail(0) { }

	size_t size() const {
		return count;
	}

	const T& operator[](size_t index) const {
		return items[(head + index) % capacity];
	}

	bool full() const {
		return count && (tail == head);
	}

	bool empty() const {
		return !count && (tail == head);
	}

	T pushBack(T value) {
		T prev = defaultValue;

		if (full()) {
			prev = items[head];
			head = nextIndex(head);
		}
		else {
			count++;
		}

		items[tail] = value;
		tail = nextIndex(tail);

		return prev;
	}

	T pushFront(T value) {
		T prev = defaultValue;

		if (full()) {
			tail = prevIndex(tail);
			prev = items[tail];
		}
		else {
			count++;
		}

		head = prevIndex(head);
		items[head] = value;

		return prev;
	}

	T popBack() {
		if (empty()) {
			return defaultValue;
		}

		count--;
		tail = prevIndex(tail);

		auto result = items[tail];
		items[tail] = defaultValue;
		return result;
	}

	T popFront() {
		if (empty()) {
			return defaultValue;
		}

		auto result = items[head];
		items[head] = defaultValue;

		count--;
		head = nextIndex(head);

		return result;
	}

	const T& front() {
		return items[head];
	}

	const T& back() {
		return items[prevIndex(tail)];
	}

	void clear() {
		items.fill(defaultValue);
		count = 0;
		head = 0;
		tail = 0;
	}

	struct iterator {
		const CircleBuffer& buffer;
		size_t index = 0;
		iterator& operator++() {
			index++;
			return *this;
		}
		const T& operator*() const { 
			return buffer[index];
		}
		friend bool operator!=(const iterator& left, const iterator& right) {
			return left.index != right.index;
		}
	};

	iterator begin() const {
		return iterator{ *this, 0 };
	}

	iterator end() const {
		return iterator{ *this, count };
	}

private:
	size_t nextIndex(size_t index) const {
		return (index + 1) % capacity;
	}
	size_t prevIndex(size_t index) const {
		return (index + capacity - 1) % capacity;
	}
};
