#pragma once

#include "Header.h"
#include "Types.h"
#include <type_traits>

namespace Rml {

class VariableDefinition;
class DataTypeRegister;
class TransformFuncRegister;
class DataModelHandle;
class DataVariable;

using DataGetFunc = Function<void(Variant&)>;
using DataSetFunc = Function<void(const Variant&)>;
using DataTransformFunc = Function<Variant(const VariantList&)>;
using DataEventFunc = Function<void(DataModelHandle, Event&, const VariantList&)>;

template <typename T>
using MemberGetFunc = void (T::*)(Variant&);
template <typename T>
using MemberSetFunc = void (T::*)(const Variant&);

template <typename T>
using DataTypeGetFunc = void (*)(const T&, Variant&);
template <typename T>
using DataTypeSetFunc = void (*)(T&, const Variant&);

template <typename Object, typename ReturnType>
using MemberGetterFunc = ReturnType (Object::*)();
template <typename Object, typename AssignType>
using MemberSetterFunc = void (Object::*)(AssignType);

using DirtyVariables = SmallUnorderedSet<String>;

struct DataAddressEntry {
	DataAddressEntry(String name) : name(std::move(name)), index(-1) {}
	DataAddressEntry(int index) : index(index) {}
	String name;
	int index;
};
using DataAddress = Vector<DataAddressEntry>;

template <class T>
struct PointerTraits {
	using is_pointer = std::false_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return ptr; }
};
template <class T>
struct PointerTraits<T*> {
	using is_pointer = std::true_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return static_cast<void*>(*static_cast<T**>(ptr)); }
};
template <class T>
struct PointerTraits<UniquePtr<T>> {
	using is_pointer = std::true_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return static_cast<void*>(static_cast<UniquePtr<T>*>(ptr)->get()); }
};
template <class T>
struct PointerTraits<SharedPtr<T>> {
	using is_pointer = std::true_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return static_cast<void*>(static_cast<SharedPtr<T>*>(ptr)->get()); }
};

struct VoidMemberFunc {};
template <typename T>
using IsVoidMemberFunc = std::is_same<T, VoidMemberFunc>;

#define RMLUI_LOG_TYPE_ERROR(T, msg) RMLUI_ERRORMSG((String(msg) + String("\nT: ") + String(rmlui_type_name<T>())).c_str())
#define RMLUI_LOG_TYPE_ERROR_ASSERT(T, val, msg) RMLUI_ASSERTMSG((val), (String(msg) + String("\nT: ") + String(rmlui_type_name<T>())).c_str())

namespace Detail {
	class DataVariableAccessor;
	class DataModelConstructorAccessor;
} // namespace Detail

} // namespace Rml
