/******************************************************************************
*	Delegate class implementation
*
*	\file Delegate.h
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
#ifndef COREZERO_DELEGATE_H_
#define COREZERO_DELEGATE_H_


namespace CoreZero {
	template <typename T> class Delegate;
}



namespace CoreZero
{
	/*****************************************************************************
	 *	\brief A Delegate is a proxy for functions.
	 *
	 *	A delegate can provide means of linking functions
	 *		and callbacks at runtime. This way, the defining
	 *		point of a delegate need not know of the course
	 *		of action, but only the interface of the behavior.
	 */
	template <typename RETTy_, typename... ARGS>
	class Delegate<RETTy_(ARGS...)>
	{
	public:
		using delegateType = RETTy_(*)(ARGS...);

		//
		//	Constructors
		//
	protected:
		/// Construct a blank delegate.
		constexpr Delegate() : p_delegateFunction(nullptr) {}

	public:
		/// Construct from function reference
		constexpr Delegate(delegateType delegateFunction) : p_delegateFunction(delegateFunction) {}

		/// Construct from other delegate; copy function reference
		constexpr Delegate(const Delegate& other) : p_delegateFunction(other.p_delegateFunction) {}

		/// Construct from other delegate; copy function reference
		constexpr Delegate(Delegate&& other) : p_delegateFunction(other.p_delegateFunction) {}

		virtual ~Delegate() {}

		/// Copy Assignment
		Delegate& operator=(const Delegate& other) { p_delegateFunction = other.p_delegateFunction; return *this; }

		/// Function Operator
		virtual RETTy_ operator()(ARGS... arguments_) {
			return p_delegateFunction(arguments_...);
		}

	private:
		/// Pointer to the represented function.
		delegateType p_delegateFunction;
	};



	/*****************************************************************************
	 *	\brief A Delegate is a proxy for functions.
	 *
	 *	A delegate can provide means of linking functions
	 *		and callbacks at runtime. This way, the defining
	 *		point of a delegate need not know of the course
	 *		of action, but only the interface of the behavior.
	 *
	 *	This Delegate implementation specifically targets the
	 *		handling of member functions, adapting the use
	 *		of them to the same use-cases as static functions.
	 */
	template<class OBJECTTy_, typename RETTy_, typename... ARGS>
	class Delegate <RETTy_(OBJECTTy_::*)(ARGS...)>
		: public Delegate<RETTy_(ARGS...)>
	{
		/// The signature of a member function.
		using memberDelegateType = RETTy_(OBJECTTy_::*)(ARGS...);

		/// The signarure of a static function.
		using freeDelegateType = RETTy_(*)(ARGS...);

	public:
		/// Construct Delegate from member function and a pointer to the class object
		constexpr Delegate(OBJECTTy_* obj_, RETTy_(OBJECTTy_::*memberFunction)(ARGS...))
			: p_classObject(obj_)
			, p_memberFunction(memberFunction) {}

		virtual ~Delegate() {}

		/// Copy assignment; copy the pointers to the class object and the member
		///		function.
		Delegate& operator=(const Delegate& other)
		{
			p_classObject = other.p_classObject;
			p_memberFunction = other.p_memberFunction;
			return *this;
		}

		/// Override the base method for calling a function, and call the member
		///		function instead.
		virtual RETTy_ operator()(ARGS... arguments_) override
		{
			return (p_classObject->*p_memberFunction)(arguments_...);
		}

	private:
		/// A pointer to the class instance.
		OBJECTTy_* p_classObject;

		/// A pointer to the class's member function;
		memberDelegateType p_memberFunction;
	};


	template <class OBJECTTy_, typename RETTy_, typename... ARGS>
	class Delegate<RETTy_(OBJECTTy_::*)(ARGS...)const>
		: public Delegate<RETTy_(ARGS...)>
	{
	public:
		constexpr Delegate(const OBJECTTy_& objectInstance, RETTy_(OBJECTTy_::* memberFunction)(ARGS...)const)
			: m_pObjectInstance(objectInstance)
			, m_pMemberFunction(memberFunction)
		{}

		template <typename LAMBDA_T>
		constexpr Delegate(const LAMBDA_T& lambdaInstance)
			: m_pObjectInstance(lambdaInstance)
			, m_pMemberFunction(&LAMBDA_T::operator())
		{}

		virtual ~Delegate() {}

		virtual RETTy_ operator()(ARGS... arguments_) override
		{
			return (m_pObjectInstance.*m_pMemberFunction)(arguments_...);
		}

	private:
		const OBJECTTy_& m_pObjectInstance;
		RETTy_(OBJECTTy_::* m_pMemberFunction)(ARGS...) const;
	};



	/*****************************************************************************
	 *	\brief Create a class member function delegate from a reference to the
	 *		class instance, and a reference to the class's member function.
	 *
	 *	\param[in] obj_ The pointer to the class instance.
	 *	\param[in] memberFunction Pointer to the class's member function.
	 *	\returns[dynamic_alloc] A delegate representing a specified class's
	 *		function.
	 */
	template<class OBJ, typename RETTy_, typename... ARGS>
	inline Delegate<RETTy_(OBJ::*)(ARGS...)>*
		Create_MemberDelegate(OBJ* const obj_, RETTy_(OBJ::* memberFunction)(ARGS...))
	{
		return new Delegate<RETTy_(OBJ::*)(ARGS...)>{ obj_, memberFunction };
	}


	template <class OBJECTTy_, typename RETTy_, typename... ARGS>
	inline Delegate<RETTy_(OBJECTTy_::*)(ARGS...)const>*
		Create_MemberDelegate(const OBJECTTy_& objectInstance, RETTy_(OBJECTTy_::* memberFunction)(ARGS...)const)
	{
		return new Delegate<RETTy_(OBJECTTy_::*)(ARGS...)const>{ objectInstance, memberFunction };
	}
}

#endif // !COREZERO_DELEGATE_H

