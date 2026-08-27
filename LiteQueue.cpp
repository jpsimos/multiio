/*
LiteQueue.cpp - A lightweight queue template class in c++
Author: Jacob A Psimos
Date: 5/9/22
Updated: 4/3/26
*/

#ifndef __LITEQUEUE_CPP
#define __LITEQUEUE_CPP

#include <atomic>
#include <stdexcept>
#include "LiteQueue.hpp"
#include "Helpers.h"

using namespace Helpers;

template <class ElementType, size_t Capacity>
LiteQueue<ElementType, Capacity>::LiteQueue()
	: m_capacity(Capacity)
	, m_head(0)
	, m_tail(0)
	, m_array()
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
    size_t h = m_head.load(std::memory_order_acquire);
    size_t t = m_tail.load(std::memory_order_acquire);
    return (t - h) & (m_capacity - 1);
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::full() const
{
	size_t t = m_tail.load(std::memory_order_acquire);
    return ((t + 1) & (m_capacity - 1)) == m_head.load(std::memory_order_acquire);
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::empty() const
{
	return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
}

template <class ElementType, size_t Capacity>
ElementType& LiteQueue<ElementType, Capacity>::front()
{
    size_t h = m_head.load(std::memory_order_relaxed);
    m_head.store((h + 1) & (m_capacity - 1), std::memory_order_release);
    return m_array[h];
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::pop()
{
    if(!empty())
    {
    	size_t h = m_head.load(std::memory_order_relaxed);
    	m_head.store((h + 1) & (m_capacity - 1), std::memory_order_release);
    	return true;
    }
    return false;
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::push(const ElementType& item)
{
    size_t t = m_tail.load(std::memory_order_relaxed);
    if (((t + 1) & (m_capacity - 1)) != m_head.load(std::memory_order_acquire))
    {
        m_array[t] = std::move(item);
        m_tail.store((t + 1) & (m_capacity - 1), std::memory_order_release);
        return true;
    }
    return false;
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::prepush(ElementType** outItem)
{
	if(outItem != nullptr)
	{
		size_t t = m_tail.load(std::memory_order_relaxed);
		if (((t + 1) & (m_capacity - 1)) != m_head.load(std::memory_order_acquire))
		{
			*outItem = std::addressof(m_array[t]);
			m_tail.store((t + 1) & (m_capacity - 1), std::memory_order_release);
			return true;
		}
		else
		{
			*outItem = nullptr;
		}
	}
	return false;
}

template <class ElementType, size_t Capacity>
bool LiteQueue<ElementType, Capacity>::unpush()
{
	if(!empty())
	{
		size_t t = m_tail.load(std::memory_order_relaxed);
		m_tail.store((t - 1) & (m_capacity - 1), std::memory_order_release);
		return true;
	}
	return false;
}

template <class ElementType, size_t Capacity>
void LiteQueue<ElementType, Capacity>::clear()
{
	m_head.store(m_tail.load(std::memory_order_acquire), std::memory_order_acquire);
}


#endif
