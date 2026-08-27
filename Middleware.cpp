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

static std::vector<Middleware*> sm_middlewarePtrs;

void Middleware::LoopRegisteredMiddlewares()
{
	for(Middleware* middlewarePtr : sm_middlewarePtrs)
	{
		if(!middlewarePtr->m_excludeFromLoop)
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
		for(Middleware* middlewarePtr : sm_middlewarePtrs)
		{
			if(timeoutMicros > 0 && timeout > timeoutMicros)
			{
				break;
			}

			if(!middlewarePtr->m_excludeFromLoop)
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

	for(Middleware* middlewarePtr : sm_middlewarePtrs)
	{
		if(!middlewarePtr->m_excludeFromLoop)
		{
			middlewarePtr->m_lastCommandResult = middlewarePtr->ProcessCommand(command);
			result |= middlewarePtr->m_lastCommandResult;
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
	m_excludeFromLoop(false),
	m_lastCommandResult(COMMAND_IGNORE)
{
}

Middleware::AbstractType Middleware::GetAbstractType() const
{
	return m_abstractType;
}

void Middleware::Register()
{
	if(sm_middlewarePtrs.end() == std::find(sm_middlewarePtrs.begin(), sm_middlewarePtrs.end(), this))
	{
		sm_middlewarePtrs.push_back(this);
		std::sort(
			sm_middlewarePtrs.begin()
			,sm_middlewarePtrs.end()
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
		= std::find(sm_middlewarePtrs.begin(), sm_middlewarePtrs.end(), this);

	if(sm_middlewarePtrs.end() != thisMiddlewareIter)
	{
		sm_middlewarePtrs.erase(thisMiddlewareIter);
	}
}

void Middleware::Loop()
{
	return;
}

int Middleware::ProcessCommand(const std::string& command)
{
  return COMMAND_IGNORE;
}
