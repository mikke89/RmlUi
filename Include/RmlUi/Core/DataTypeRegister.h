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

#ifndef RMLUI_CORE_DATATYPEREGISTER_H
#define RMLUI_CORE_DATATYPEREGISTER_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "Variant.h"
#include "DataTypes.h"
#include "DataVariable.h"


namespace Rml {

#define RMLUI_LOG_TYPE_ERROR(T, msg) RMLUI_ERRORMSG((String(msg) + String(" T: ") + String(rmlui_type_name<T>())).c_str())
#define RMLUI_LOG_TYPE_ERROR_ASSERT(T, val, msg) RMLUI_ASSERTMSG(val, (String(msg) + String(" T: ") + String(rmlui_type_name<T>())).c_str())

template<typename T>
struct is_valid_data_scalar {
	static constexpr bool value = std::is_arithmetic<T>::value
		|| std::is_same<typename std::remove_cv<T>::type, String>::value;
};


template<typename Object>
class StructHandle {
public:
	StructHandle(DataTypeRegister* type_register, StructDefinition* struct_definition) : type_register(type_register), struct_definition(struct_definition) {}

	// Register a member object. Any data type can be used.
	template <typename MemberType>
	StructHandle<Object>& RegisterMember(const String& name, MemberType Object::* member_ptr);

	// Register a member using a getter function. Must return value by reference or by raw pointer. Any data type can be used.
    template<typename ReturnType>
    StructHandle<Object>& RegisterMember(const String& name, ReturnType(Object::* member_get_func_ptr)());

	// Register a member using a getter function and optionally a setter function. Only scalar data types can be used.
	// Can get and set by value or by reference.
	/// @example
	///     RegisterMemberScalar("color", &Invader::GetColor, &Invader::SetColor);
	///       where functions can be declared as 
	///     MyColorType Invader::GetColor();
	///     void Invader::SetColor(MyColorType color);
	template<typename ReturnType = std::nullptr_t, typename AssignType = std::nullptr_t>
	StructHandle<Object>& RegisterMemberScalar(const String& name, ReturnType(Object::* member_get_func_ptr)(), void(Object::* member_set_func_ptr)(AssignType) = nullptr);

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




class RMLUICORE_API DataTypeRegister final : NonCopyMoveable {
public:
	DataTypeRegister();
	~DataTypeRegister();

	template<typename T>
	StructHandle<T> RegisterStruct()
	{
		static_assert(std::is_class<T>::value, "Type must be a struct or class type.");
		FamilyId id = Family<T>::Id();

		auto struct_definition = MakeUnique<StructDefinition>();
		StructDefinition* struct_variable_raw = struct_definition.get();

		bool inserted = type_register.emplace(id, std::move(struct_definition)).second;
		if (!inserted)
		{
			RMLUI_LOG_TYPE_ERROR(T, "Struct type already declared");
			return StructHandle<T>(nullptr, nullptr);
		}
		
		return StructHandle<T>(this, struct_variable_raw);
	}

	template<typename Container>
	bool RegisterArray()
	{
		using value_type = typename Container::value_type;
		VariableDefinition* value_variable = GetDefinitionDetail<value_type>();
		RMLUI_LOG_TYPE_ERROR_ASSERT(value_type, value_variable, "Underlying value type of array has not been registered.");
		if (!value_variable)
			return false;

		FamilyId container_id = Family<Container>::Id();

		auto array_definition = MakeUnique<ArrayDefinition<Container>>(value_variable);

		bool inserted = type_register.emplace(container_id, std::move(array_definition)).second;
		if (!inserted)
		{
			RMLUI_LOG_TYPE_ERROR(Container, "Array type already declared.");
			return false;
		}

		return true;
	}

	template<typename T>
	bool RegisterScalar(DataTypeGetFunc<T> get_func, DataTypeSetFunc<T> set_func)
	{
		static_assert(!is_valid_data_scalar<T>::value, "Cannot register scalar data type function. Arithmetic types and String are handled internally and does not need to be registered.");
		FamilyId id = Family<T>::Id();

		auto scalar_func_definition = MakeUnique<ScalarFuncDefinition<T>>(get_func, set_func);

		bool inserted = type_register.emplace(id, std::move(scalar_func_definition)).second;
		if (!inserted)
		{
			RMLUI_LOG_TYPE_ERROR(T, "Scalar function type already registered.");
			return false;
		}

		return true;
	}

	template<typename T>
	VariableDefinition* GetDefinition()
	{
		return GetDefinitionDetail<T>();
	}

	TransformFuncRegister* GetTransformFuncRegister() {
		return &transform_register;
	}

private:

	// Get definition for scalar types that can be assigned to and from Rml::Variant.
	// We automatically register these when needed, so users don't have to register trivial types manually.
	template<typename T, typename std::enable_if<!PointerTraits<T>::is_pointer::value&& is_valid_data_scalar<T>::value, int>::type = 0>
	VariableDefinition* GetDefinitionDetail()
	{
		FamilyId id = Family<T>::Id();

		auto result = type_register.emplace(id, nullptr);
		bool inserted = result.second;
		UniquePtr<VariableDefinition>& definition = result.first->second;

		if (inserted)
			definition = MakeUnique<ScalarDefinition<T>>();

		return definition.get();
	}

	// Get definition for non-scalar types.
	// These must already have been registered by the user.
	template<typename T, typename std::enable_if<!PointerTraits<T>::is_pointer::value && !is_valid_data_scalar<T>::value, int>::type = 0>
	VariableDefinition* GetDefinitionDetail()
	{
		FamilyId id = Family<T>::Id();
		auto it = type_register.find(id);
		if (it == type_register.end())
		{
			RMLUI_LOG_TYPE_ERROR(T, "Desired data type T not registered with the type register, please use the 'Register...()' functions before binding values, adding members, or registering arrays of non-scalar types.");
			return nullptr;
		}

		return it->second.get();
	}

	// Get definition for pointer types, or create one as needed.
	// This will create a wrapper definition that forwards the call to the definition for the underlying type.
	template<typename T, typename std::enable_if<PointerTraits<T>::is_pointer::value, int>::type = 0>
	VariableDefinition* GetDefinitionDetail()
	{
		static_assert(PointerTraits<T>::is_pointer::value, "Not a valid pointer type.");
		static_assert(!PointerTraits<typename PointerTraits<T>::element_type>::is_pointer::value, "Recursive pointer type (pointer to pointer) detected.");

		// Get the underlying definition.
		VariableDefinition* underlying_definition = GetDefinitionDetail<typename std::remove_cv<typename PointerTraits<T>::element_type>::type>();
		if (!underlying_definition)
		{
			RMLUI_LOG_TYPE_ERROR(T, "Underlying type of pointer not registered.");
			return nullptr;
		}

		// Get or create the pointer wrapper definition.
		FamilyId id = Family<T>::Id();

		auto result = type_register.emplace(id, nullptr);
		bool inserted = result.second;
		UniquePtr<VariableDefinition>& definition = result.first->second;

		if (inserted)
			definition = MakeUnique<PointerDefinition<T>>(underlying_definition);

		return definition.get();
	}


	UnorderedMap<FamilyId, UniquePtr<VariableDefinition>> type_register;

	TransformFuncRegister transform_register;
};

template<typename Object>
template<typename MemberType>
inline StructHandle<Object>& StructHandle<Object>::RegisterMember(const String& name, MemberType Object::* member_ptr)
{
	if (VariableDefinition* member_definition = type_register->GetDefinition<MemberType>())
	{
		struct_definition->AddMember(
			name,
			MakeUnique<MemberObjectPointerDefinition<Object, MemberType>>(member_definition, member_ptr)
		);
	}
	return *this;
}

template<typename Object>
template<typename ReturnType>
inline StructHandle<Object>& StructHandle<Object>::RegisterMember(const String& name, ReturnType(Object::*member_get_func_ptr)())
{
	using BasicReturnType = typename std::remove_pointer<typename std::remove_reference<ReturnType>::type>::type;
	static_assert(!std::is_same<ReturnType, BasicReturnType>::value, "Struct member getter function must return value by reference or raw pointer.");

	VariableDefinition* underlying_definition = type_register->GetDefinition<BasicReturnType>();
	struct_definition->AddMember(
		name,
		MakeUnique<MemberFunctionPointerDefinition<Object, ReturnType, BasicReturnType>>(underlying_definition, member_get_func_ptr)
	);
	return *this;
}

template<typename Object>
template<typename ReturnType, typename AssignType>
inline StructHandle<Object>& StructHandle<Object>::RegisterMemberScalar(const String& name, ReturnType(Object::* member_get_func_ptr)(), void(Object::* member_set_func_ptr)(AssignType))
{
	using BasicReturnType = typename std::remove_cv<typename std::remove_reference<ReturnType>::type>::type;
	using BasicAssignType = typename std::remove_cv<typename std::remove_reference<AssignType>::type>::type;
	using UnderlyingType = typename std::conditional<std::is_null_pointer<BasicReturnType>::value, BasicAssignType, BasicReturnType>::type;

	static_assert(std::is_null_pointer<ReturnType>::value || std::is_null_pointer<AssignType>::value || std::is_same<BasicReturnType, BasicAssignType>::value, "Provided getter and setter functions must get and set the same type.");
	static_assert(std::is_copy_assignable<UnderlyingType>::value, "Struct member getter and setter functions must return/assign a type that is copy assignable.");
	static_assert(std::is_default_constructible<UnderlyingType>::value, "Struct member getter and setter functions must return/assign a type that is default constructible.");

	VariableDefinition* underlying_definition = type_register->GetDefinition<UnderlyingType>();
	if (!underlying_definition)
		return *this;

	DataVariableType type = underlying_definition->Type();
	if (!(type == DataVariableType::Scalar || type == DataVariableType::Function))
	{
		RMLUI_LOG_TYPE_ERROR(UnderlyingType, "A getter/setter member function must return and assign a scalar data variable type.");
		return *this;
	}

	struct_definition->AddMember(
		name,
		MakeUnique<MemberScalarFunctionPointerDefinition<Object, ReturnType, AssignType, UnderlyingType>>(underlying_definition, member_get_func_ptr, member_set_func_ptr)
	);
	return *this;
}

} // namespace Rml
#endif
