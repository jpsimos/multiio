/*
LiteQueue.cpp - A lightweight queue template class in c++
Author: Jacob A Psimos
Date: 5/9/22
Updated: 4/11/25
*/

#ifndef __LITEQUEUE_CPP
#define __LITEQUEUE_CPP

#include <stdexcept>
#include "LiteQueue.hpp"
#include "Helpers.h"

using namespace Helpers;

template <class ElementType, size_t Capacity>
LiteQueue<ElementType, Capacity>::LiteQueue(const bool ring)
	: m_capacity(Capacity)
	, m_ring(ring)
	, m_count(0)
	, m_overruns(0)
	, m_head(0)
	, m_tail(0)
	, m_unpushable(false)
	, m_array()
{
}

template <class ElementType, size_t Capacity>
LiteQueue<ElementType, Capacity>::~LiteQueue()
{
}

template <class ElementType, size_t Capacity>
size_t LiteQueue<ElementType, Capacity>::capacity() const
{
	return m_capacity;
}

template <class ElementType, size_t Capacity>
size_t LiteQueue<ElementType, Capacity>::count() const
{
	return m_count;
}

template <class ElementType, size_t Capacity>
size_t LiteQueue<ElementType, Capacity>::overruns() const
{
	return m_overruns;
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::full() const
{
	return static_cast<bool>(m_count == m_capacity);
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::empty() const
{
	return static_cast<bool>(0 == m_count);
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::push(const ElementType& item)
{
	if (m_ring || m_count < m_capacity)
	{
		m_array[m_tail % m_capacity] = item;
		m_tail = m_tail + 1;
		m_unpushable = false;

		if(m_count < m_capacity)
		{
			m_count = m_count + 1;
		}
		else
		{
			m_overruns = m_overruns + 1;
		}

		return true;
	}
	return false;
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::prepush(ElementType** outItem)
{
	if(nullptr != outItem && (m_ring || m_count < m_capacity))
	{
		*outItem = &m_array[m_tail % m_capacity];
		m_tail = m_tail + 1;
		m_unpushable = true;

		if(m_count < m_capacity)
		{
			m_count = m_count + 1;
		}
		else
		{
			m_overruns = m_overruns + 1;
		}

		return true;
	}

	return false;
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::unpush()
{
	if(m_unpushable || m_count > 0)
	{
		m_unpushable = false;

		if(m_count > 0)
		{
			m_tail = m_tail - 1;
			m_count = m_count - 1;
		}
	}

	return !m_unpushable;
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::pop()
{
	bool popped = false;

	if (m_count > 0)
	{
		m_head = (m_head + 1) % m_capacity;
		m_count = m_count - 1;
		popped = true;
	}

	if(0 == m_count)
	{
		m_unpushable = false;
	}

	return popped;
}

template <class ElementType, size_t Capacity>
ElementType& LiteQueue<ElementType, Capacity>::front()
{
	if(0 == m_count)
	{
		throw std::runtime_error("LiteQueue::front() Queue is empty.");
	}

	return m_array[m_head];
}

template <class ElementType, size_t Capacity>
void LiteQueue<ElementType, Capacity>::clear()
{
	m_count = 0;
	m_head = 0;
	m_tail = 0;
	m_overruns = 0;
	m_unpushable = false;
}

#endif
