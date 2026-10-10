/*
LiteQueue.hpp - A lightweight queue template class in c++
Author: Jacob A Psimos
Date: 5/9/22
Updated: 4/11/25
*/

#ifndef __LITEQUEUE_H
#define __LITEQUEUE_H

template <class ElementType, size_t Capacity>
class LiteQueue
{
public:
	explicit LiteQueue();
	~LiteQueue();
	size_t capacity() const;
	size_t count() const;
	size_t overruns() const;
	bool full() const;
	bool empty() const;
	bool push(const ElementType& item, const bool ring = false);
	bool prepush(ElementType** outItem, const bool ring = false);
	bool unpush();
	bool pop();
	ElementType& front();
	void clear();

private:
	const size_t m_capacity;
	volatile size_t m_count;
	volatile size_t m_overruns;
	volatile size_t m_head;
	volatile size_t m_tail;
	volatile bool m_unpushable;
	alignas(16) ElementType m_array[Capacity];
};

#include "LiteQueue.cpp"

#endif
