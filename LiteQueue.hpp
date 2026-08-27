/*
LiteQueue.hpp - A lightweight queue template class in c++
Author: Jacob A Psimos
Date: 5/9/22
Updated: 4/3/26
*/

#ifndef __LITEQUEUE_H
#define __LITEQUEUE_H

#include <atomic>

template <class ElementType, size_t Capacity>
class LiteQueue
{
public:
	explicit LiteQueue();
	~LiteQueue() = default;
	size_t capacity() const;
	size_t count() const;
	bool full() const;
	bool empty() const;
	ElementType& front();
	bool pop();
	bool push(const ElementType& item);
	bool prepush(ElementType** outItem);
	bool unpush();
	void clear();

private:
	const size_t m_capacity;
	std::atomic<size_t> m_head;
	std::atomic<size_t> m_tail;
	alignas(32) ElementType m_array[Capacity];
};

#include "LiteQueue.cpp"

#endif
