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

#ifndef RMLUICOREDATATYPEREGISTER_H
#define RMLUICOREDATATYPEREGISTER_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "Variant.h"
#include "DataTypes.h"
#include "DataVariable.h"


namespace Rml {
namespace Core {


template<typename T>
struct is_valid_data_scalar {
	static constexpr bool value = std::is_arithmetic<T>::value
		|| std::is_same<typename std::remove_cv<T>::type, String>::value;
};


template<typename Object>
class StructHandle {
public:
	StructHandle(DataTypeRegister* type_register, StructDefinition* struct_definition) : type_register(type_register), struct_definition(struct_definition) {}
	
	template <typename MemberType>
	StructHandle<Object>& RegisterMember(const String& name, MemberType Object::* member_ptr);

	StructHandle<Object>& RegisterMemberFunc(const String& name, MemberGetFunc<Object> get_func, MemberSetFunc<Object> set_func = nullptr);

	explicit operator bool() const {
		return type_register && struct_definition;
	}

private:
	DataTypeRegister* type_register;
	StructDefinition* struct_definition;
};


class RMLUICORE_API TransformFuncRegister {
public:
	void Register(const String& name, DataTransformFunc transform_func);

	bool Call(const String& name, Variant& inout_result, const VariantList& arguments) const;

private:
	UnorderedMap<String, DataTransformFunc> transform_functions;
};



class RMLUICORE_API DataTypeRegister : NonCopyMoveable {
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

	template<typename T, typename std::enable_if<is_valid_data_scalar<T>::value, int>::type = 0>
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

	template<typename T, typename std::enable_if<!is_valid_data_scalar<T>::value, int>::type = 0>
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

private:
	UnorderedMap<FamilyId, UniquePtr<VariableDefinition>> type_register;

	TransformFuncRegister transform_register;

};

template<typename Object>
template<typename MemberType>
inline StructHandle<Object>& StructHandle<Object>::RegisterMember(const String& name, MemberType Object::* member_ptr) {
	VariableDefinition* member_type = type_register->GetOrAddScalar<MemberType>();
	struct_definition->AddMember(name, std::make_unique<StructMemberObject<Object, MemberType>>(member_type, member_ptr));
	return *this;
}
template<typename Object>
inline StructHandle<Object>& StructHandle<Object>::RegisterMemberFunc(const String& name, MemberGetFunc<Object> get_func, MemberSetFunc<Object> set_func) {
	VariableDefinition* definition = type_register->RegisterMemberFunc<Object>(get_func, set_func);
	struct_definition->AddMember(name, std::make_unique<StructMemberFunc>(definition));
	return *this;
}

}
}

#endif
