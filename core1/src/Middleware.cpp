/*
 * Middleware.h
 * Author Jacob A Psimos
 *
 */

#include <functional>
#include <algorithm>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include "xparameters.h"
#include "Middleware.h"
#include "Helpers.h"

using namespace Helpers;

static std::vector<Middleware*> middlewarePtrs;

void Middleware::LoopRegisteredMiddlewares()
{
	for(Middleware* middlewarePtr : middlewarePtrs)
	{
		if(!middlewarePtr->m_nestedLock)
		{
			middlewarePtr->Loop();
		}
	}
}

void Middleware::Yield(unsigned int timeoutMicros)
{
	Time::Stopwatch timeout;

	do
	{
		for(Middleware* middlewarePtr : middlewarePtrs)
		{
			if(timeoutMicros > 0 && timeout > timeoutMicros)
			{
				break;
			}

			if(!middlewarePtr->m_nestedLock)
			{
				middlewarePtr->Loop();
			}
		}
	}
	while(timeoutMicros > 0 && timeout < timeoutMicros);
}

int Middleware::ProcessCommandRegisteredMiddlewares(const std::string& command)
{
	int result = COMMAND_IGNORE;

	for(Middleware* middlewarePtr : middlewarePtrs)
	{
		if(!middlewarePtr->m_nestedLock)
		{
			result |= middlewarePtr->ProcessCommand(command);
		}
	}

	return result;
}


Middleware::Middleware(
	const Middleware::AbstractType abstractType,
	int priority
) :
	m_abstractType(abstractType),
	m_nestedPriority(priority),
	m_nestedLock(false),
	m_bestLoopTime(0U),
	m_worstLoopTime(0U),
	m_loopPerformanceTimer()
{
}

Middleware::AbstractType Middleware::GetAbstractType() const
{
	return m_abstractType;
}

void Middleware::Register()
{
	if(middlewarePtrs.end() == std::find(middlewarePtrs.begin(), middlewarePtrs.end(), this))
	{
		middlewarePtrs.push_back(this);
		std::sort(
			middlewarePtrs.begin()
			,middlewarePtrs.end()
			,[](const Middleware* a, const Middleware* b)
			{
				if(a && b)
				{
					return static_cast<bool>(a->m_nestedPriority < b->m_nestedPriority);
				}
				return false;
			}
		);
	}
}

void Middleware::Unregister()
{
	std::vector<Middleware*>::const_iterator thisMiddlewareIter
		= std::find(middlewarePtrs.begin(), middlewarePtrs.end(), this);

	if(middlewarePtrs.end() != thisMiddlewareIter)
	{
		middlewarePtrs.erase(thisMiddlewareIter);
	}
}

void Middleware::SetNestedLock(const bool lock)
{
	m_nestedLock = lock;
}

bool Middleware::GetNestedLock() const
{
	const bool nestedLock = m_nestedLock;
	return nestedLock;
}

int Middleware::MeasureLoopPerformance(unsigned int* millis)
{
	int result = MW_PERF_NOT_INTERESTING;

	if(m_loopPerformanceTimer < m_bestLoopTime)
	{
		m_bestLoopTime = m_loopPerformanceTimer;
		result |= MW_PERF_NEW_BEST;
	}

	if(m_loopPerformanceTimer > m_worstLoopTime)
	{
		m_worstLoopTime = m_loopPerformanceTimer;
		result |= MW_PERF_NEW_WORST;
	}

	if(NULL != millis)
	{
		*millis = static_cast<unsigned int>(m_loopPerformanceTimer.GetMillis());
	}

	m_loopPerformanceTimer.Reset();

	return result;
}

void Middleware::Loop()
{
	return;
}

int Middleware::ProcessCommand(const std::string& command)
{
  return COMMAND_IGNORE;
}
