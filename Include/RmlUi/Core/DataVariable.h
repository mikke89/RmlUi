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

#ifndef RMLUICOREDATAVARIABLE_H
#define RMLUICOREDATAVARIABLE_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "Variant.h"
#include <functional>

namespace Rml {
namespace Core {

class Variable;
class DataTypeRegister;
class DataModelHandle;
class Event;

template<typename T>
struct is_valid_scalar {
	static constexpr bool value = std::is_arithmetic<T>::value 
		|| std::is_same<typename std::remove_cv<T>::type, String>::value;
};

enum class VariableType { Scalar, Array, Struct, Function, MemberFunction };

enum class DataFunctionHandle : int {};

using DataGetFunc = std::function<void(Variant&)>;
using DataSetFunc = std::function<void(const Variant&)>;
using DataTransformFunc = std::function<bool(Variant&, const VariantList&)>;
using DataEventFunc = std::function<void(DataModelHandle, Event&, const VariantList&)>;

template<typename T> using MemberGetFunc = void(T::*)(Variant&);
template<typename T> using MemberSetFunc = void(T::*)(const Variant&);


struct AddressEntry {
	AddressEntry(String name) : name(name), index(-1) { }
	AddressEntry(int index) : index(index) { }
	String name;
	int index;
};
using DataAddress = std::vector<AddressEntry>;


class RMLUICORE_API VariableDefinition {
public:
	virtual ~VariableDefinition() = default;
	VariableType Type() const { return type; }

	virtual bool Get(void* ptr, Variant& variant);
	virtual bool Set(void* ptr, const Variant& variant);
	virtual int Size(void* ptr);
	virtual Variable GetChild(void* ptr, const AddressEntry& address);

protected:
	VariableDefinition(VariableType type) : type(type) {}

private:
	VariableType type;
};



class Variable {
public:
	Variable() {}
	Variable(VariableDefinition* definition, void* ptr) : definition(definition), ptr(ptr) {}

	explicit operator bool() const { return definition && (ptr || Type() == VariableType::Function); }

	inline bool Get(Variant& variant) {
		return definition->Get(ptr, variant);
	}
	inline bool Set(const Variant& variant) {
		return definition->Set(ptr, variant);
	}
	inline int Size() {
		return definition->Size(ptr);
	}
	inline Variable GetChild(const AddressEntry& address) {
		return definition->GetChild(ptr, address);
	}
	inline VariableType Type() const {
		return definition->Type();
	}

private:
	VariableDefinition* definition = nullptr;
	void* ptr = nullptr;
};



template<typename T>
class ScalarDefinition final : public VariableDefinition {
public:
	ScalarDefinition() : VariableDefinition(VariableType::Scalar) {}

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

	FuncDefinition(DataGetFunc get, DataSetFunc set) : VariableDefinition(VariableType::Function), get(std::move(get)), set(std::move(set)) {}

	bool Get(void* ptr, Variant& variant) override
	{
		if (!get)
			return false;
		get(variant);
		return true;
	}
	bool Set(void* ptr, const Variant& variant) override
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


template<typename Container>
class ArrayDefinition final : public VariableDefinition {
public:
	ArrayDefinition(VariableDefinition* underlying_definition) : VariableDefinition(VariableType::Array), underlying_definition(underlying_definition) {}

	int Size(void* ptr) override {
		return int(static_cast<Container*>(ptr)->size());
	}

protected:
	Variable GetChild(void* void_ptr, const AddressEntry& address) override
	{
		Container* ptr = static_cast<Container*>(void_ptr);
		const int index = address.index;

		const int container_size = int(ptr->size());
		if (index < 0 || index >= container_size)
		{
			if (address.name == "size")
				return Variable(DataTypeRegister::GetArraySizeDefinition(), reinterpret_cast<void*>(static_cast<intptr_t>(container_size)));

			Log::Message(Log::LT_WARNING, "Data array index out of bounds.");
			return Variable();
		}

		void* next_ptr = &((*ptr)[index]);
		return Variable(underlying_definition, next_ptr);
	}

private:
	VariableDefinition* underlying_definition;
};


class StructMember {
public:
	StructMember(VariableDefinition* definition) : definition(definition) {}
	virtual ~StructMember() = default;

	VariableDefinition* GetVariable() const { return definition; }

	virtual void* GetPointer(void* base_ptr) = 0;

private:
	VariableDefinition* definition;
};

template <typename Object, typename MemberType>
class StructMemberDefault final : public StructMember {
public:
	StructMemberDefault(VariableDefinition* definition, MemberType Object::* member_ptr) : StructMember(definition), member_ptr(member_ptr) {}

	void* GetPointer(void* base_ptr) override {
		return &(static_cast<Object*>(base_ptr)->*member_ptr);
	}

private:
	MemberType Object::* member_ptr;
};

class StructMemberFunc final : public StructMember {
public:
	StructMemberFunc(VariableDefinition* definition) : StructMember(definition) {}
	void* GetPointer(void* base_ptr) override {
		return base_ptr;
	}
};


class StructDefinition final : public VariableDefinition {
public:
	StructDefinition() : VariableDefinition(VariableType::Struct)
	{}

	Variable GetChild(void* ptr, const AddressEntry& address) override
	{
		const String& name = address.name;
		if (name.empty())
		{
			Log::Message(Log::LT_WARNING, "Expected a struct member name but none given.");
			return Variable();
		}

		auto it = members.find(name);
		if (it == members.end())
		{
			Log::Message(Log::LT_WARNING, "Member %s not found in data struct.", name.c_str());
			return Variable();
		}

		void* next_ptr = it->second->GetPointer(ptr);
		VariableDefinition* next_variable = it->second->GetVariable();

		return Variable(next_variable, next_ptr);
	}

	void AddMember(const String& name, UniquePtr<StructMember> member)
	{
		RMLUI_ASSERT(member);
		bool inserted = members.emplace(name, std::move(member)).second;
		RMLUI_ASSERTMSG(inserted, "Member name already exists.");
		(void)inserted;
	}

private:
	SmallUnorderedMap<String, UniquePtr<StructMember>> members;
};


template<typename T>
class MemberFuncDefinition final : public VariableDefinition {
public:
	MemberFuncDefinition(MemberGetFunc<T> get, MemberSetFunc<T> set) : VariableDefinition(VariableType::MemberFunction), get(get), set(set) {}

	bool Get(void* ptr, Variant& variant) override
	{
		if (!get)
			return false;
		(static_cast<T*>(ptr)->*get)(variant);
		return true;
	}
	bool Set(void* ptr, const Variant& variant) override
	{
		if (!set)
			return false;
		(static_cast<T*>(ptr)->*set)(variant);
		return true;
	}
private:
	MemberGetFunc<T> get;
	MemberSetFunc<T> set;
};


class DataTypeRegister;

template<typename Object>
class StructHandle {
public:
	StructHandle(DataTypeRegister* type_register, StructDefinition* struct_definition) : type_register(type_register), struct_definition(struct_definition) {}
	
	template <typename MemberType>
	StructHandle<Object>& AddMember(const String& name, MemberType Object::* member_ptr);

	StructHandle<Object>& AddMemberFunc(const String& name, MemberGetFunc<Object> get_func, MemberSetFunc<Object> set_func = nullptr);

	explicit operator bool() const {
		return type_register && struct_definition;
	}

private:
	DataTypeRegister* type_register;
	StructDefinition* struct_definition;
};





class TransformFuncRegister {
public:
	void Register(const String& name, DataTransformFunc transform_func)
	{
		RMLUI_ASSERT(transform_func);
		bool inserted = transform_functions.emplace(name, std::move(transform_func)).second;
		if (!inserted)
		{
			Log::Message(Log::LT_ERROR, "Transform function '%s' already exists.", name.c_str());
			RMLUI_ERROR;
		}
	}

	bool Call(const String& name, Variant& inout_result, const VariantList& arguments) const
	{
		auto it = transform_functions.find(name);
		if (it == transform_functions.end())
			return false;

		const DataTransformFunc& transform_func = it->second;
		RMLUI_ASSERT(transform_func);

		return transform_func(inout_result, arguments);
	}

private:
	UnorderedMap<String, DataTransformFunc> transform_functions;
};



class DataTypeRegister : NonCopyMoveable {
public:
	DataTypeRegister();
	~DataTypeRegister();

	template<typename T>
	StructHandle<T> RegisterStruct()
	{
		static_assert(std::is_class<T>::value, "Type must be a struct or class type.");
		FamilyId id = Family<T>::Id();

		auto struct_variable = std::make_unique<StructDefinition>();
		StructDefinition* struct_variable_raw = struct_variable.get();

		bool inserted = type_register.emplace(id, std::move(struct_variable)).second;
		if (!inserted)
		{
			RMLUI_ERRORMSG("Type already declared");
			return StructHandle<T>(nullptr, nullptr);
		}
		
		return StructHandle<T>(this, struct_variable_raw);
	}

	template<typename Container>
	bool RegisterArray()
	{
		using value_type = typename Container::value_type;
		VariableDefinition* value_variable = GetOrAddScalar<value_type>();
		RMLUI_ASSERTMSG(value_variable, "Underlying value type of array has not been registered.");
		if (!value_variable)
			return false;

		FamilyId container_id = Family<Container>::Id();

		auto array_variable = std::make_unique<ArrayDefinition<Container>>(value_variable);
		ArrayDefinition<Container>* array_variable_raw = array_variable.get();

		bool inserted = type_register.emplace(container_id, std::move(array_variable)).second;
		if (!inserted)
		{
			RMLUI_ERRORMSG("Array type already declared.");
			return false;
		}

		return true;
	}


	VariableDefinition* RegisterFunc(DataGetFunc get_func, DataSetFunc set_func)
	{
		static DataFunctionHandle current_handle = DataFunctionHandle(0);
		current_handle = DataFunctionHandle(int(current_handle) + 1);

		auto result = functions.emplace(current_handle, nullptr);
		auto& it = result.first;
		bool inserted = result.second;
		RMLUI_ASSERT(inserted);
		(void)inserted;

		it->second = std::make_unique<FuncDefinition>(std::move(get_func), std::move(set_func));

		return it->second.get();
	}

	template<typename T>
	VariableDefinition* RegisterMemberFunc(MemberGetFunc<T> get_func, MemberSetFunc<T> set_func)
	{
		FamilyId id = Family<MemberGetFunc<T>>::Id();

		auto result = type_register.emplace(id, nullptr);
		auto& it = result.first;
		bool inserted = result.second;

		if (inserted)
			it->second = std::make_unique<MemberFuncDefinition<T>>(get_func, set_func);

		return it->second.get();
	}

	template<typename T, typename std::enable_if<is_valid_scalar<T>::value, int>::type = 0>
	VariableDefinition* GetOrAddScalar()
	{
		FamilyId id = Family<T>::Id();

		auto result = type_register.emplace(id, nullptr);
		bool inserted = result.second;
		UniquePtr<VariableDefinition>& definition = result.first->second;

		if (inserted)
			definition = std::make_unique<ScalarDefinition<T>>();

		return definition.get();
	}

	template<typename T, typename std::enable_if<!is_valid_scalar<T>::value, int>::type = 0>
	VariableDefinition* GetOrAddScalar()
	{
		return Get<T>();
	}

	template<typename T>
	VariableDefinition* Get()
	{
		FamilyId id = Family<T>::Id();
		auto it = type_register.find(id);
		if (it == type_register.end())
		{
			RMLUI_ERRORMSG("Desired data type T not registered with the type register, please use the 'Register...()' functions before binding values, adding members, or registering arrays of non-scalar types.")
			return nullptr;
		}

		return it->second.get();
	}

	TransformFuncRegister* GetTransformFuncRegister() {
		return &transform_register;
	}

	static VariableDefinition* GetArraySizeDefinition();

private:
	UnorderedMap<DataFunctionHandle, UniquePtr<FuncDefinition>> functions;

	UnorderedMap<FamilyId, UniquePtr<VariableDefinition>> type_register;

	TransformFuncRegister transform_register;

};



template<typename Object>
template<typename MemberType>
inline StructHandle<Object>& StructHandle<Object>::AddMember(const String& name, MemberType Object::* member_ptr) {
	VariableDefinition* member_type = type_register->GetOrAddScalar<MemberType>();
	struct_definition->AddMember(name, std::make_unique<StructMemberDefault<Object, MemberType>>(member_type, member_ptr));
	return *this;
}
template<typename Object>
inline StructHandle<Object>& StructHandle<Object>::AddMemberFunc(const String& name, MemberGetFunc<Object> get_func, MemberSetFunc<Object> set_func) {
	VariableDefinition* definition = type_register->RegisterMemberFunc<Object>(get_func, set_func);
	struct_definition->AddMember(name, std::make_unique<StructMemberFunc>(definition));
	return *this;
}

}
}

#endif
