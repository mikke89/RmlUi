#pragma once

#include "../Config/Config.h"
#include "Debug.h"
#include "Header.h"
#include <type_traits>

#ifdef RMLUI_HAS_RTTI
	#include <typeinfo>
#endif

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
		static_assert(std::is_base_of_v<Releasable, T>, "Rml::Releaser can only operate with classes derived from ::Rml::Releasable.");
		Release(static_cast<Releasable*>(target));
	}
};

enum class FamilyId : int {};

class RMLUICORE_API FamilyBase {
protected:
	static int GetNewId();
};

template <typename T>
class Family : FamilyBase {
public:
	// Get a unique ID for a given type.
	// Note: An ID for a given type may not match across DLL-boundaries.
	static FamilyId Id()
	{
		static int id = GetNewId();
		return static_cast<FamilyId>(id);
	}
};

using ClassId = void*;

} // namespace Rml

#define RMLUI_RTTI_Declare(NAME)                     \
	using RttiClassType = NAME;                      \
	using RttiParentClassType = NAME;                \
	static Rml::ClassId GetStaticClassIdentifier();  \
	virtual Rml::ClassId GetClassIdentifier() const; \
	virtual bool IsClass(Rml::ClassId type_identifier) const;

#define RMLUI_RTTI_DeclareWithParent(NAME, PARENT)                                       \
	using RttiClassType = NAME;                                                          \
	using RttiParentClassType = PARENT;                                                  \
	static_assert(std::is_same_v<typename PARENT::RttiClassType, PARENT>,                \
		"Parent does not implement RMLUI_RTTI_Declare or RMLUI_RTTI_DeclareWithParent"); \
	static Rml::ClassId GetStaticClassIdentifier();                                      \
	Rml::ClassId GetClassIdentifier() const override;                                    \
	bool IsClass(Rml::ClassId type_identifier) const override;

#define RMLUI_RTTI_Define(NAME)                                                                                    \
	Rml::ClassId NAME::GetStaticClassIdentifier()                                                                  \
	{                                                                                                              \
		static int dummy;                                                                                          \
		return &dummy;                                                                                             \
	}                                                                                                              \
	Rml::ClassId NAME::GetClassIdentifier() const                                                                  \
	{                                                                                                              \
		return GetStaticClassIdentifier();                                                                         \
	}                                                                                                              \
	bool NAME::IsClass(Rml::ClassId type_identifier) const                                                         \
	{                                                                                                              \
		if constexpr (!std::is_same_v<NAME, NAME::RttiParentClassType>)                                            \
		{                                                                                                          \
			return type_identifier == GetStaticClassIdentifier() || RttiParentClassType::IsClass(type_identifier); \
		}                                                                                                          \
		else                                                                                                       \
		{                                                                                                          \
			return type_identifier == GetStaticClassIdentifier();                                                  \
		}                                                                                                          \
	}

template <class Derived, class Base>
Derived rmlui_dynamic_cast(Base base_instance)
{
	static_assert(std::is_pointer_v<Derived> && std::is_pointer_v<Base>, "rmlui_dynamic_cast can only cast pointer types");
	using T_Derived = std::remove_cv_t<std::remove_pointer_t<Derived>>;
	static_assert(std::is_same_v<typename T_Derived::RttiClassType, T_Derived>,
		"Derived type does not implement RMLUI_RTTI_Declare or RMLUI_RTTI_DeclareWithParent");

	if (base_instance && base_instance->IsClass(T_Derived::GetStaticClassIdentifier()))
		return static_cast<Derived>(base_instance);
	else
		return nullptr;
}

template <class Derived, class Base>
Derived rmlui_static_cast(Base base_instance)
{
	static_assert(std::is_pointer_v<Derived> && std::is_pointer_v<Base>, "rmlui_static_cast can only cast pointer types");
#ifdef RMLUI_HAS_RTTI
	RMLUI_ASSERT(dynamic_cast<Derived>(base_instance));
#endif
	return static_cast<Derived>(base_instance);
}

#ifdef RMLUI_HAS_RTTI

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

#else

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

#endif
