#pragma once

#include <array>
#include <sstream>

template<typename T, size_t capacity, T defaultValue>
class CircleBuffer {

	std::array<T, capacity> items;
	size_t count;
	size_t head;	// First element position
	size_t tail;	// Next position after last element

public:
	CircleBuffer() : items({ defaultValue }) {
		count = 0;
		head = 0;
		tail = 0;
	}

	size_t size() const {
		return count;
	}

	int operator[](size_t index) const {
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

private:
	size_t nextIndex(size_t index) {
		return (index + 1) % capacity;
	}
	size_t prevIndex(size_t index) {
		return (index + capacity - 1) % capacity;
	}
};
