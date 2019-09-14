/******************************************************************************
*	Event class implmentation
*
*	\file Event.h
*	\author Jensen Miller
*
*	Copyright (c) 2019 LooUQ Incorporated.
*
*	License: The GNU License
*
*	This file is part of CoreZero.
*
*   CoreZero is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   CoreZero is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with CoreZero.  If not, see <https://www.gnu.org/licenses/>.
*
******************************************************************************/
#ifndef COREZERO_EVENT_H
#define COREZERO_EVENT_H

#include "CoreZero.Delegate.hpp"

namespace CoreZero
{
	template <typename T>
	class Event;
}



namespace CoreZero
{
	/*****************************************************************************
	 *	\brief An Event exposes an interface for signals to be handled.
	 *
	 *	A static function or an object may subscribe to an event
	 *		to receive control upon a given signal. This allows
	 *		the chaining of events and tasks to carry out the
	 *		proper and full handling of a given signal.
	 */
	template <typename RETTy_, typename... ARGS>
	class Event<Delegate<RETTy_(ARGS...)>> final
	{
		using EventHandler = Delegate<RETTy_(ARGS...)>;
		using eventHandlerType = RETTy_(*)(ARGS...);

	public:
		/// Construct an event using a blank Delegate.
		constexpr Event() : p_eventHandler(new Delegate<RETTy_(ARGS...)>([](ARGS...)->RETTy_ {return RETTy_(); })) {}		

		/// Destructor deletes the referenced handler.
		~Event() { delete p_eventHandler; }

	public:
		/// Function operator that uses callback
		RETTy_ operator()(ARGS... arguments_) {			
			return (*p_eventHandler)(arguments_...);
		}

		/// Subscribe operator to attach event handler.
		void operator +=(const EventHandler evHandler) {
			delete p_eventHandler;
			p_eventHandler = new Delegate<RETTy_(ARGS...)>(evHandler);
		}
		
		/// Subscribe operator to attach an object's member function as an
		///		event handler.
		template <class T>
		void operator +=(Delegate<RETTy_(T::*)(ARGS...)> * evHandler)
		{
			delete p_eventHandler;
			p_eventHandler = evHandler;
		}
		
	private:
		/// A pointer to the event handler.
		EventHandler * p_eventHandler;
	};	
}


#endif // !COREZERO_EVENT_H
