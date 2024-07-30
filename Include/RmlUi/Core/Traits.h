/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_TRAITS_H
#define RMLUI_CORE_TRAITS_H

#include "../Config/Config.h"
#include "Debug.h"
#include "Header.h"
#include <type_traits>

namespace Rml {

class RMLUICORE_API NonCopyMoveable {
public:
	NonCopyMoveable() {}
	~NonCopyMoveable() {}
	NonCopyMoveable(const NonCopyMoveable&) = delete;
	NonCopyMoveable& operator=(const NonCopyMoveable&) = delete;
	NonCopyMoveable(NonCopyMoveable&&) = delete;
	NonCopyMoveable& operator=(NonCopyMoveable&&) = delete;
};

class ReleaserBase;

class RMLUICORE_API Releasable : public NonCopyMoveable {
protected:
	virtual ~Releasable() = default;
	virtual void Release() = 0;
	friend class Rml::ReleaserBase;
};

class RMLUICORE_API ReleaserBase {
protected:
	void Release(Releasable* target) const { target->Release(); }
};

template <typename T>
class RMLUICORE_API Releaser : public ReleaserBase {
public:
	void operator()(T* target) const
	{
		static_assert(std::is_base_of<Releasable, T>::value, "Rml::Releaser can only operate with classes derived from ::Rml::Releasable.");
		Release(static_cast<Releasable*>(target));
	}
};

enum class FamilyId : int {};

class RMLUICORE_API FamilyBase {
protected:
	static int GetNewId()
	{
		static int id = 0;
		return id++;
	}
};

template <typename T>
class Family : FamilyBase {
public:
	// Get a unique ID for a given type.
	// Note: IDs will be unique across DLL-boundaries even for the same type.
	static FamilyId Id()
	{
		static int id = GetNewId();
		return static_cast<FamilyId>(id);
	}
};

} // namespace Rml

#ifdef RMLUI_CUSTOM_RTTI

	#define RMLUI_RTTI_Define(_NAME_)                             \
		using RttiClassType = _NAME_;                             \
		static void* GetStaticClassIdentifier()                   \
		{                                                         \
			static int dummy;                                     \
			return &dummy;                                        \
		}                                                         \
		virtual bool IsClass(void* type_identifier) const         \
		{                                                         \
			return type_identifier == GetStaticClassIdentifier(); \
		}

	#define RMLUI_RTTI_DefineWithParent(_NAME_, _PARENT_)                                               \
		using RttiClassType = _NAME_;                                                                   \
		static void* GetStaticClassIdentifier()                                                         \
		{                                                                                               \
			static int dummy;                                                                           \
			return &dummy;                                                                              \
		}                                                                                               \
		bool IsClass(void* type_identifier) const override                                              \
		{                                                                                               \
			static_assert(std::is_same<typename _PARENT_::RttiClassType, _PARENT_>::value,              \
				"Parent does not implement RMLUI_RTTI_Define or RMLUI_RTTI_DefineWithParent");          \
			return type_identifier == GetStaticClassIdentifier() || _PARENT_::IsClass(type_identifier); \
		}

template <class Derived, class Base>
Derived rmlui_dynamic_cast(Base base_instance)
{
	static_assert(std::is_pointer<Derived>::value && std::is_pointer<Base>::value, "rmlui_dynamic_cast can only cast pointer types");
	using T_Derived = typename std::remove_cv<typename std::remove_pointer<Derived>::type>::type;

	static_assert(std::is_same<typename T_Derived::RttiClassType, T_Derived>::value, "Derived type does not implement RMLUI_RTTI_DefineWithParent");

	if (base_instance->IsClass(T_Derived::GetStaticClassIdentifier()))
		return static_cast<Derived>(base_instance);
	else
		return nullptr;
}

template <class Derived, class Base>
Derived rmlui_static_cast(Base base_instance)
{
	static_assert(std::is_pointer<Derived>::value && std::is_pointer<Base>::value, "rmlui_static_cast can only cast pointer types");
	return static_cast<Derived>(base_instance);
}

template <class T>
const char* rmlui_type_name(const T& /*var*/)
{
	return "(type name unavailable)";
}

template <class T>
const char* rmlui_type_name()
{
	return "(type name unavailable)";
}

#else

	#include <typeinfo>

	#define RMLUI_RTTI_Define(_NAME_)
	#define RMLUI_RTTI_DefineWithParent(_NAME_, _PARENT_)

template <class Derived, class Base>
Derived rmlui_dynamic_cast(Base base_instance)
{
	static_assert(std::is_pointer<Derived>::value && std::is_pointer<Base>::value, "rmlui_dynamic_cast can only cast pointer types");
	return dynamic_cast<Derived>(base_instance);
}

template <class Derived, class Base>
Derived rmlui_static_cast(Base base_instance)
{
	static_assert(std::is_pointer<Derived>::value && std::is_pointer<Base>::value, "rmlui_static_cast can only cast pointer types");
	RMLUI_ASSERT(dynamic_cast<Derived>(base_instance));
	return static_cast<Derived>(base_instance);
}

template <class T>
const char* rmlui_type_name(const T& var)
{
	return typeid(var).name();
}

template <class T>
const char* rmlui_type_name()
{
	return typeid(T).name();
}

#endif // RMLUI_CUSTOM_RTTI

#endif // RMLUI_CORE_TRAITS_H
