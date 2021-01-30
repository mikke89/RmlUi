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

#define RMLUI_LOG_TYPE_ERROR(T, msg) RMLUI_ERRORMSG((String(msg) + String("\nT: ") + String(rmlui_type_name<T>())).c_str())
#define RMLUI_LOG_TYPE_ERROR_ASSERT(T, val, msg) RMLUI_ASSERTMSG(val, (String(msg) + String("\nT: ") + String(rmlui_type_name<T>())).c_str())

template<typename T>
struct is_builtin_data_scalar {
	static constexpr bool value = std::is_arithmetic<T>::value
		|| std::is_same<typename std::remove_const<T>::type, String>::value;
};


template<typename Object>
class StructHandle {
public:
	StructHandle(DataTypeRegister* type_register, StructDefinition* struct_definition) : type_register(type_register), struct_definition(struct_definition) {}

	/// Register a member object.
	/// @note Underlying type must be registered before it is used as a member.
	/// @note Getter functions can return by reference, raw pointer, or by value. If returned by value,
	///       the returned type must be a scalar data type, otherwise any data type can be used.
	/// @example
	///		struct Invader {
	///			int health;
	/// 	};
	///		struct_handle.RegisterMember("health", &Invader::health);
	template <typename MemberType>
	bool RegisterMember(const String& name, MemberType Object::* member_object_ptr)
	{
		return CreateMemberObjectDefinition(name, member_object_ptr);
	}

	/// Register a member getter function.
	/// @note Underlying type must be registered before it is used as a member.
	/// @note Getter functions can return by reference, raw pointer, or by value. If returned by value,
	///       the returned type must be a scalar data type, otherwise any data type can be used.
	/// @example
	///		struct Invader {
	///			std::vector<Weapon>& GetWeapons();
	///			/* ... */
	/// 	};
	///		struct_handle.RegisterMember("weapons", &Invader::GetWeapons);
	template <typename ReturnType>
	bool RegisterMember(const String& name, ReturnType(Object::* member_get_func_ptr)())
	{
		return RegisterMemberGetter(name, member_get_func_ptr);
	}


	/// Register member getter and setter functions. The getter and setter functions must return and assign scalar value types.
	/// @note Underlying type must be registered before it is used as a member.
	/// @note Getter and setter functions can return by reference, raw pointer, or by value. Only scalar data types allowed.
	/// @example
	///		struct Invader {
	///			MyColorType GetColor() const;
	///			void SetColor(MyColorType color);
	///			/* ... */
	/// 	};
	///		struct_handle.RegisterMember("color", &Invader::GetColor, &Invader::SetColor);
	template <typename ReturnType, typename AssignType>
	bool RegisterMember(const String& name, ReturnType(Object::* member_get_func_ptr)(), void(Object::* member_set_func_ptr)(AssignType))
	{
		using BasicReturnType = typename std::remove_reference<ReturnType>::type;
		using BasicAssignType = typename std::remove_const<typename std::remove_reference<AssignType>::type>::type;
		using UnderlyingType = typename std::conditional<std::is_null_pointer<BasicReturnType>::value, BasicAssignType, BasicReturnType>::type;

		static_assert(std::is_null_pointer<ReturnType>::value || std::is_null_pointer<AssignType>::value || std::is_same<BasicReturnType, BasicAssignType>::value, "Provided getter and setter functions must get and set the same type.");

		return CreateMemberScalarGetSetFuncDefinition<UnderlyingType>(name, member_get_func_ptr, member_set_func_ptr);
	}

	explicit operator bool() const {
		return type_register && struct_definition;
	}

private:

	template <typename ReturnType>
	bool RegisterMemberGetter(const String& name, ReturnType& (Object::* member_get_func_ptr)()) {
		return CreateMemberGetFuncDefinition<ReturnType>(name, member_get_func_ptr);
	}

	template <typename ReturnType>
	bool RegisterMemberGetter(const String& name, ReturnType* (Object::* member_get_func_ptr)()) {
		return CreateMemberGetFuncDefinition<ReturnType>(name, member_get_func_ptr);
	}

	template <typename ReturnType>
	bool RegisterMemberGetter(const String& name, ReturnType(Object::* member_get_func_ptr)()) {
		using BasicReturnType = typename std::remove_reference<ReturnType>::type;
		using SetType = std::nullptr_t Object::*;
		return CreateMemberScalarGetSetFuncDefinition<BasicReturnType>(name, member_get_func_ptr, SetType{});
	}

	template<typename MemberType>
	bool CreateMemberObjectDefinition(const String& name, MemberType Object::* member_ptr);

	template<typename BasicReturnType, typename MemberType>
	bool CreateMemberGetFuncDefinition(const String& name, MemberType Object::* member_get_func_ptr);

	template<typename UnderlyingType, typename MemberGetType, typename MemberSetType>
	bool CreateMemberScalarGetSetFuncDefinition(const String& name, MemberGetType Object::* member_get_func_ptr, MemberSetType Object::* member_set_func_ptr);

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
		static_assert(!is_builtin_data_scalar<T>::value, "Cannot register scalar data type function. Arithmetic types and String are handled internally and does not need to be registered.");
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
	template<typename T, typename std::enable_if<!PointerTraits<T>::is_pointer::value && is_builtin_data_scalar<T>::value, int>::type = 0>
	VariableDefinition* GetDefinitionDetail()
	{
		static_assert(!std::is_const<T>::value, "Data binding variables cannot point to constant variables.");
		FamilyId id = Family<T>::Id();

		auto result = type_register.emplace(id, nullptr);
		bool inserted = result.second;
		UniquePtr<VariableDefinition>& definition = result.first->second;

		if (inserted)
			definition = MakeUnique<ScalarDefinition<T>>();

		return definition.get();
	}

	// Get definition for types that are not a built-in scalar.
	// These must already have been registered by the user.
	template<typename T, typename std::enable_if<!PointerTraits<T>::is_pointer::value && !is_builtin_data_scalar<T>::value, int>::type = 0>
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
		static_assert(PointerTraits<T>::is_pointer::value, "Invalid pointer type provided.");

		using UnderlyingType = typename PointerTraits<T>::element_type;
		static_assert(!PointerTraits<UnderlyingType>::is_pointer::value, "Recursive pointer types (pointer to pointer) to data variables are disallowed.");
		static_assert(!std::is_const<UnderlyingType>::value, "Pointer to a const data variable is not supported.");

		// Get the underlying definition.
		VariableDefinition* underlying_definition = GetDefinitionDetail<UnderlyingType>();
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
bool StructHandle<Object>::CreateMemberObjectDefinition(const String& name, MemberType Object::* member_ptr)
{
	using MemberObjectPtr = MemberType Object::*;
	// If the member function signature doesn't match the getter function signature, it will end up calling this function. Emit a compile error in that case.
	static_assert(!std::is_member_function_pointer<MemberObjectPtr>::value, "Illegal data member getter function signature. Make sure it takes no arguments and is not const qualified.");
	static_assert(!std::is_const<MemberType>::value, "Data member objects cannot be const qualified.");

	VariableDefinition* underlying_definition = type_register->GetDefinition<MemberType>();
	if (!underlying_definition)
		return false;
	struct_definition->AddMember(
		name,
		MakeUnique<MemberObjectDefinition<Object, MemberType>>(underlying_definition, member_ptr)
	);
	return true;
}

template<typename Object>
template<typename BasicReturnType, typename MemberType>
bool StructHandle<Object>::CreateMemberGetFuncDefinition(const String& name, MemberType Object::* member_get_func_ptr)
{
	static_assert(!std::is_const<BasicReturnType>::value, "Returned type from data member function cannot be const qualified.");

	VariableDefinition* underlying_definition = type_register->GetDefinition<BasicReturnType>();
	if (!underlying_definition)
		return false;

	struct_definition->AddMember(
		name,
		MakeUnique<MemberGetFuncDefinition<Object, MemberType, BasicReturnType>>(underlying_definition, member_get_func_ptr)
	);
	return true;
}

template<typename Object>
template<typename UnderlyingType, typename MemberGetType, typename MemberSetType>
bool StructHandle<Object>::CreateMemberScalarGetSetFuncDefinition(const String& name, MemberGetType Object::* member_get_func_ptr, MemberSetType Object::* member_set_func_ptr)
{
	static_assert(std::is_default_constructible<UnderlyingType>::value, "Struct member getter/setter functions must return/assign a type that is default constructible.");
	static_assert(!std::is_const<UnderlyingType>::value, "Const qualified type illegal in data member getter functions.");

	VariableDefinition* underlying_definition = type_register->GetDefinition<UnderlyingType>();
	if (!underlying_definition)
		return false;

	if (underlying_definition->Type() != DataVariableType::Scalar)
	{
		RMLUI_LOG_TYPE_ERROR(UnderlyingType, "Returning or assigning a non-scalar variable by value in a data struct member function is illegal. Only scalar data variables are allowed here: A getter function returning by value, or a getter/setter function pair.");
		return false;
	}

	struct_definition->AddMember(
		name,
		MakeUnique<MemberScalarGetSetFuncDefinition<Object, MemberGetType, MemberSetType, UnderlyingType>>(underlying_definition, member_get_func_ptr, member_set_func_ptr)
	);
	return true;
}

} // namespace Rml
#endif
