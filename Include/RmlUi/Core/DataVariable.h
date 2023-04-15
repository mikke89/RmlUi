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

#ifndef RMLUI_CORE_DATAVARIABLE_H
#define RMLUI_CORE_DATAVARIABLE_H

#include "DataTypes.h"
#include "Header.h"
#include "Traits.h"
#include "Types.h"
#include "Variant.h"
#include <iterator>

namespace Rml {

enum class DataVariableType { Scalar, Array, Struct };

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

class RMLUICORE_API VariableDefinition : public NonCopyMoveable {
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

// Literal data variable constructor
RMLUICORE_API DataVariable MakeLiteralIntVariable(int value);

template <typename T>
class ScalarDefinition final : public VariableDefinition {
public:
	ScalarDefinition() : VariableDefinition(DataVariableType::Scalar) {}

	bool Get(void* ptr, Variant& variant) override
	{
		variant = *static_cast<const T*>(ptr);
		return true;
	}
	bool Set(void* ptr, const Variant& variant) override { return variant.GetInto<T>(*static_cast<T*>(ptr)); }
};

class RMLUICORE_API FuncDefinition final : public VariableDefinition {
public:
	FuncDefinition(DataGetFunc get, DataSetFunc set);

	bool Get(void* ptr, Variant& variant) override;
	bool Set(void* ptr, const Variant& variant) override;

private:
	DataGetFunc get;
	DataSetFunc set;
};

template <typename T>
class ScalarFuncDefinition final : public VariableDefinition {
public:
	ScalarFuncDefinition(DataTypeGetFunc<T> get, DataTypeSetFunc<T> set) : VariableDefinition(DataVariableType::Scalar), get(get), set(set) {}

	bool Get(void* ptr, Variant& variant) override
	{
		if (!get)
			return false;
		get(*static_cast<const T*>(ptr), variant);
		return true;
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		if (!set)
			return false;
		set(*static_cast<T*>(ptr), variant);
		return true;
	}

private:
	DataTypeGetFunc<T> get;
	DataTypeSetFunc<T> set;
};

class RMLUICORE_API StructDefinition final : public VariableDefinition {
public:
	StructDefinition();

	DataVariable Child(void* ptr, const DataAddressEntry& address) override;

	void AddMember(const String& name, UniquePtr<VariableDefinition> member);

private:
	SmallUnorderedMap<String, UniquePtr<VariableDefinition>> members;
};

template <typename Container>
class ArrayDefinition final : public VariableDefinition {
public:
	ArrayDefinition(VariableDefinition* underlying_definition) :
		VariableDefinition(DataVariableType::Array), underlying_definition(underlying_definition)
	{}

	int Size(void* ptr) override { return int(static_cast<Container*>(ptr)->size()); }

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

class RMLUICORE_API BasePointerDefinition : public VariableDefinition {
public:
	BasePointerDefinition(VariableDefinition* underlying_definition);

	bool Get(void* ptr, Variant& variant) override;
	bool Set(void* ptr, const Variant& variant) override;
	int Size(void* ptr) override;
	DataVariable Child(void* ptr, const DataAddressEntry& address) override;

protected:
	virtual void* DereferencePointer(void* ptr) = 0;

private:
	VariableDefinition* underlying_definition;
};

template <typename T>
class PointerDefinition final : public BasePointerDefinition {
public:
	PointerDefinition(VariableDefinition* underlying_definition) : BasePointerDefinition(underlying_definition) {}

protected:
	void* DereferencePointer(void* ptr) override { return PointerTraits<T>::Dereference(ptr); }
};

template <typename Object, typename MemberType>
class MemberObjectDefinition final : public BasePointerDefinition {
public:
	MemberObjectDefinition(VariableDefinition* underlying_definition, MemberType Object::*member_ptr) :
		BasePointerDefinition(underlying_definition), member_ptr(member_ptr)
	{}

protected:
	void* DereferencePointer(void* base_ptr) override { return &(static_cast<Object*>(base_ptr)->*member_ptr); }

private:
	MemberType Object::*member_ptr;
};

template <typename Object, typename MemberType, typename BasicReturnType>
class MemberGetFuncDefinition final : public BasePointerDefinition {
public:
	MemberGetFuncDefinition(VariableDefinition* underlying_definition, MemberType Object::*member_get_func_ptr) :
		BasePointerDefinition(underlying_definition), member_get_func_ptr(member_get_func_ptr)
	{}

protected:
	void* DereferencePointer(void* base_ptr) override
	{
		return static_cast<void*>(Extract((static_cast<Object*>(base_ptr)->*member_get_func_ptr)()));
	}

private:
	BasicReturnType* Extract(BasicReturnType* value) { return value; }
	BasicReturnType* Extract(BasicReturnType& value) { return &value; }

	MemberType Object::*member_get_func_ptr;
};

template <typename Object, typename MemberGetType, typename MemberSetType, typename UnderlyingType>
class MemberScalarGetSetFuncDefinition final : public VariableDefinition {
public:
	MemberScalarGetSetFuncDefinition(VariableDefinition* underlying_definition, MemberGetType Object::*member_get_func_ptr,
		MemberSetType Object::*member_set_func_ptr) :
		VariableDefinition(underlying_definition->Type()),
		underlying_definition(underlying_definition), member_get_func_ptr(member_get_func_ptr), member_set_func_ptr(member_set_func_ptr)
	{}

	bool Get(void* ptr, Variant& variant) override { return GetDetail(ptr, variant); }
	bool Set(void* ptr, const Variant& variant) override { return SetDetail(ptr, variant); }

private:
	template <typename T = MemberGetType, typename std::enable_if<IsVoidMemberFunc<T>::value, int>::type = 0>
	bool GetDetail(void* /*ptr*/, Variant& /*variant*/)
	{
		return false;
	}

	template <typename T = MemberGetType, typename std::enable_if<!IsVoidMemberFunc<T>::value, int>::type = 0>
	bool GetDetail(void* ptr, Variant& variant)
	{
		RMLUI_ASSERT(member_get_func_ptr);

		auto&& value = (static_cast<Object*>(ptr)->*member_get_func_ptr)();
		bool result = underlying_definition->Get(static_cast<void*>(&value), variant);
		return result;
	}

	template <typename T = MemberSetType, typename std::enable_if<IsVoidMemberFunc<T>::value, int>::type = 0>
	bool SetDetail(void* /*ptr*/, const Variant& /*variant*/)
	{
		return false;
	}

	template <typename T = MemberSetType, typename std::enable_if<!IsVoidMemberFunc<T>::value, int>::type = 0>
	bool SetDetail(void* ptr, const Variant& variant)
	{
		RMLUI_ASSERT(member_set_func_ptr);

		UnderlyingType result;
		if (!underlying_definition->Set(static_cast<void*>(&result), variant))
			return false;

		(static_cast<Object*>(ptr)->*member_set_func_ptr)(result);

		return true;
	}

	VariableDefinition* underlying_definition;
	MemberGetType Object::*member_get_func_ptr;
	MemberSetType Object::*member_set_func_ptr;
};

} // namespace Rml
#endif
