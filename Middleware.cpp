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
		middlewarePtr->Loop();
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

			middlewarePtr->Loop();
		}
	}
	while(timeoutMicros > 0 && timeout < timeoutMicros);
}

int Middleware::ProcessCommandRegisteredMiddlewares(const std::string& command)
{
	int result = COMMAND_IGNORE;

	for(Middleware* middlewarePtr : middlewarePtrs)
	{
		result |= middlewarePtr->ProcessCommand(command);
	}

	return result;
}

Middleware::Middleware(
	const Middleware::AbstractType abstractType,
	int priority
) :
	m_abstractType(abstractType),
	m_nestedPriority(priority)
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

void Middleware::Loop()
{
	return;
}

int Middleware::ProcessCommand(const std::string& command)
{
  return COMMAND_IGNORE;
}
