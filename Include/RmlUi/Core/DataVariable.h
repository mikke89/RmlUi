/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUI_CORE_DATAVARIABLE_H
#define RMLUI_CORE_DATAVARIABLE_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "Variant.h"
#include "DataTypes.h"
#include <iterator>

namespace Rml {

enum class DataVariableType { Scalar, Array, Struct, Function, MemberFunction };


/*
*   A 'DataVariable' wraps a user handle (pointer) and a VariableDefinition.
*
*   Together they can be used to get and set variables between the user side and data model side.
*/

class RMLUICORE_API DataVariable {
public:
	DataVariable() {}
	DataVariable(VariableDefinition* definition, void* ptr) : definition(definition), ptr(ptr) {}

	explicit operator bool() const { return definition; }

	bool Get(Variant& variant);
	bool Set(const Variant& variant);
	int Size();
	DataVariable Child(const DataAddressEntry& address);
	DataVariableType Type();

private:
	VariableDefinition* definition = nullptr;
	void* ptr = nullptr;
};


/*
*   A 'VariableDefinition' specifies how a user handle (pointer) is translated to and from a value in the data model.
* 
*   Generally, Scalar types can set and get values, while Array and Struct types can retrieve children based on data addresses.
*/

class RMLUICORE_API VariableDefinition {
public:
	virtual ~VariableDefinition() = default;
	DataVariableType Type() const { return type; }

	virtual bool Get(void* ptr, Variant& variant);
	virtual bool Set(void* ptr, const Variant& variant);

	virtual int Size(void* ptr);
	virtual DataVariable Child(void* ptr, const DataAddressEntry& address);

protected:
	VariableDefinition(DataVariableType type) : type(type) {}

private:
	DataVariableType type;
};


RMLUICORE_API DataVariable MakeLiteralIntVariable(int value);


template<typename T>
class ScalarDefinition final : public VariableDefinition {
public:
	ScalarDefinition() : VariableDefinition(DataVariableType::Scalar) {}

	bool Get(void* ptr, Variant& variant) override
	{
		variant = *static_cast<T*>(ptr);
		return true;
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		return variant.GetInto<T>(*static_cast<T*>(ptr));
	}
};

class FuncDefinition final : public VariableDefinition {
public:

	FuncDefinition(DataGetFunc get, DataSetFunc set) : VariableDefinition(DataVariableType::Function), get(std::move(get)), set(std::move(set)) {}

	bool Get(void* /*ptr*/, Variant& variant) override
	{
		if (!get)
			return false;
		get(variant);
		return true;
	}
	bool Set(void* /*ptr*/, const Variant& variant) override
	{
		if (!set)
			return false;
		set(variant);
		return true;
	}
private:
	DataGetFunc get;
	DataSetFunc set;
};

template<typename T>
class ScalarFuncDefinition final : public VariableDefinition {
public:

	ScalarFuncDefinition(DataTypeGetFunc<T> get, DataTypeSetFunc<T> set) : VariableDefinition(DataVariableType::Function), get(get), set(set) {}

	bool Get(void* ptr, Variant& variant) override
	{
		if (!get)
			return false;
		get(static_cast<const T*>(ptr), variant);
		return true;
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		if (!set)
			return false;
		set(static_cast<T*>(ptr), variant);
		return true;
	}
private:
	DataTypeGetFunc<T> get;
	DataTypeSetFunc<T> set;
};

template<typename Container>
class ArrayDefinition final : public VariableDefinition {
public:
	ArrayDefinition(VariableDefinition* underlying_definition) : VariableDefinition(DataVariableType::Array), underlying_definition(underlying_definition) {}

	int Size(void* ptr) override {
		return int(static_cast<Container*>(ptr)->size());
	}

protected:
	DataVariable Child(void* void_ptr, const DataAddressEntry& address) override
	{
		Container* ptr = static_cast<Container*>(void_ptr);
		const int index = address.index;

		const int container_size = int(ptr->size());
		if (index < 0 || index >= container_size)
		{
			if (address.name == "size")
				return MakeLiteralIntVariable(container_size);

			Log::Message(Log::LT_WARNING, "Data array index out of bounds.");
			return DataVariable();
		}

		auto it = ptr->begin();
		std::advance(it, index);

		void* next_ptr = &(*it);
		return DataVariable(underlying_definition, next_ptr);
	}

private:
	VariableDefinition* underlying_definition;
};


template<typename T>
class PointerDefinition final : public VariableDefinition {
public:
	PointerDefinition(VariableDefinition* underlying_definition) : VariableDefinition(underlying_definition->Type()), underlying_definition(underlying_definition) {}

	bool Get(void* ptr, Variant& variant) override
	{
		return underlying_definition->Get(DereferencePointer(ptr), variant);
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		return SetDetail<T>(ptr, variant);
	}
	int Size(void* ptr) override
	{
		return underlying_definition->Size(DereferencePointer(ptr));
	}
	DataVariable Child(void* ptr, const DataAddressEntry& address) override
	{
		// TODO: Return the constness of T?
		return underlying_definition->Child(DereferencePointer(ptr), address);
	}

private:
	template<typename U, typename std::enable_if<!std::is_const<typename PointerTraits<U>::element_type>::value, int>::type = 0>
	bool SetDetail(void* ptr, const Variant& variant)
	{
		return underlying_definition->Set(DereferencePointer(ptr), variant);
	}
	template<typename U, typename std::enable_if<std::is_const<typename PointerTraits<U>::element_type>::value, int>::type = 0>
	bool SetDetail(void* /*ptr*/, const Variant& /*variant*/)
	{
		return false;
	}

	static void* DereferencePointer(void* ptr)
	{
		return PointerTraits<T>::Dereference(ptr);
	}

	VariableDefinition* underlying_definition;
};


class StructDefinition final : public VariableDefinition {
public:
	StructDefinition() : VariableDefinition(DataVariableType::Struct)
	{}

	DataVariable Child(void* ptr, const DataAddressEntry& address) override
	{
		const String& name = address.name;
		if (name.empty())
		{
			Log::Message(Log::LT_WARNING, "Expected a struct member name but none given.");
			return DataVariable();
		}

		auto it = members.find(name);
		if (it == members.end())
		{
			Log::Message(Log::LT_WARNING, "Member %s not found in data struct.", name.c_str());
			return DataVariable();
		}

		VariableDefinition* next_definition = it->second.get();

		return DataVariable(next_definition, ptr);
	}

	void AddMember(const String& name, UniquePtr<VariableDefinition> member)
	{
		RMLUI_ASSERT(member);
		bool inserted = members.emplace(name, std::move(member)).second;
		RMLUI_ASSERTMSG(inserted, "Member name already exists.");
		(void)inserted;
	}

private:
	SmallUnorderedMap<String, UniquePtr<VariableDefinition>> members;
};


template <typename Object, typename MemberType>
class MemberAccessor {
protected:
	MemberAccessor(MemberType Object::* member_ptr) : member_ptr(member_ptr) {}

	void* ResolvePointer(void* base_ptr) const {
		return &(static_cast<Object*>(base_ptr)->*member_ptr);
	}

private:
	MemberType Object::* member_ptr;
};

template <typename Object, typename MemberType>
class MemberObjectPointerDefinition final : public VariableDefinition, public MemberAccessor<Object, MemberType> {
public:
	MemberObjectPointerDefinition(VariableDefinition* underlying_definition, MemberType Object::* member_ptr)
		: VariableDefinition(underlying_definition->Type()), MemberAccessor<Object, MemberType>(member_ptr), underlying_definition(underlying_definition) {}

	bool Get(void* ptr, Variant& variant) override
	{
		return underlying_definition->Get(ResolvePointer(ptr), variant);
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		return underlying_definition->Set(ResolvePointer(ptr), variant);
	}
	int Size(void* ptr) override
	{
		return underlying_definition->Size(ResolvePointer(ptr));
	}
	DataVariable Child(void* ptr, const DataAddressEntry& address) override
	{
		return underlying_definition->Child(ResolvePointer(ptr), address);
	}

private:
	using MemberAccessor<Object, MemberType>::ResolvePointer;

	VariableDefinition* underlying_definition;
};


template <typename Object, typename ReturnType, typename BasicReturnType>
class MemberFunctionPointerDefinition final : public VariableDefinition {
public:
	using MemberFunc = ReturnType(Object::*)();

	MemberFunctionPointerDefinition(VariableDefinition* underlying_definition, MemberFunc member_ptr)
		: VariableDefinition(underlying_definition->Type()), underlying_definition(underlying_definition), member_ptr(member_ptr) 
	{
		// static assert return type is reference or pointer
	}

	bool Get(void* ptr, Variant& variant) override
	{
		return underlying_definition->Get(GetPointer(ptr), variant);
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		return underlying_definition->Set(GetPointer(ptr), variant);
	}
	int Size(void* ptr) override
	{
		return underlying_definition->Size(GetPointer(ptr));
	}
	DataVariable Child(void* ptr, const DataAddressEntry& address) override
	{
		return underlying_definition->Child(GetPointer(ptr), address);
	}

private:
	void* GetPointer(void* base_ptr) {
		return (void*)Extract((static_cast<Object*>(base_ptr)->*member_ptr)());
	}

	// Pointer return types
	BasicReturnType* Extract(BasicReturnType* value) {
		return value;
	}
	// Reference return types
	BasicReturnType* Extract(BasicReturnType& value) {
		return &value;
	}

	VariableDefinition* underlying_definition;
	MemberFunc member_ptr;
};



template <typename Object, typename ReturnType, typename AssignType, typename UnderlyingType>
class MemberScalarFunctionPointerDefinition final : public VariableDefinition {
public:
	using MemberFunc = ReturnType(Object::*)();

	MemberScalarFunctionPointerDefinition(VariableDefinition* underlying_definition, MemberGetterFunc<Object, ReturnType> get_func, MemberSetterFunc<Object, AssignType> set_func)
		: VariableDefinition(underlying_definition->Type()), underlying_definition(underlying_definition), get_func(get_func), set_func(set_func)
	{}

	bool Get(void* ptr, Variant& variant) override
	{
		return GetDetail(ptr, variant);
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		return SetDetail(ptr, variant);
	}

private:

	template<typename R = ReturnType, typename std::enable_if<std::is_null_pointer<R>::value, int>::type = 0>
	bool GetDetail(void* /*ptr*/, Variant& /*variant*/)
	{
		return false;
	}

	template<typename R = ReturnType, typename std::enable_if<!std::is_null_pointer<R>::value, int>::type = 0>
	bool GetDetail(void* ptr, Variant& variant)
	{
		if (!get_func)
			return false;

		// TODO: Does this work for pointers?
		auto&& value = (static_cast<Object*>(ptr)->*get_func)();
		bool result = underlying_definition->Get((void*)&value, variant);
		return result;
	}

	template<typename A = AssignType, typename std::enable_if<std::is_null_pointer<A>::value, int>::type = 0>
	bool SetDetail(void* /*ptr*/, const Variant& /*variant*/)
	{
		return false;
	}

	template<typename A = AssignType, typename std::enable_if<!std::is_null_pointer<A>::value, int>::type = 0>
	bool SetDetail(void* ptr, const Variant& variant)
	{
		if (!set_func)
			return false;

		UnderlyingType result;
		if (!underlying_definition->Set((void*)&result, variant))
			return false;

		(static_cast<Object*>(ptr)->*set_func)(result);

		return true;
	}

	VariableDefinition* underlying_definition;
	MemberGetterFunc<Object, ReturnType> get_func;
	MemberSetterFunc<Object, AssignType> set_func;
};


} // namespace Rml
#endif
