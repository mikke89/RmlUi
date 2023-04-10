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

#ifndef RMLUI_CORE_DATASTRUCTHANDLE_H
#define RMLUI_CORE_DATASTRUCTHANDLE_H

#include "DataTypeRegister.h"
#include "DataTypes.h"
#include "DataVariable.h"
#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

template <typename Object>
class StructHandle {
public:
	StructHandle(DataTypeRegister* type_register, StructDefinition* struct_definition) :
		type_register(type_register), struct_definition(struct_definition)
	{}

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
	bool RegisterMember(const String& name, MemberType Object::*member_object_ptr)
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
	bool RegisterMember(const String& name, ReturnType (Object::*member_get_func_ptr)())
	{
		return RegisterMemberGetter(name, member_get_func_ptr);
	}

	/// Register member getter and setter functions. The getter and setter functions must return and assign scalar value types.
	/// @note Underlying type must be registered before it is used as a member.
	/// @note Getter and setter functions can return by reference, raw pointer, or by value.
	/// @example
	///		struct Invader {
	///			MyColorType GetColor();
	///			void SetColor(MyColorType color);
	///			/* ... */
	/// 	};
	///		struct_handle.RegisterMember("color", &Invader::GetColor, &Invader::SetColor);
	template <typename ReturnType, typename AssignType>
	bool RegisterMember(const String& name, ReturnType (Object::*member_get_func_ptr)(), void (Object::*member_set_func_ptr)(AssignType))
	{
		using BasicReturnType = typename std::remove_reference<ReturnType>::type;
		using BasicAssignType = typename std::remove_const<typename std::remove_reference<AssignType>::type>::type;
		using UnderlyingType = typename std::conditional<IsVoidMemberFunc<ReturnType>::value, BasicAssignType, BasicReturnType>::type;

		static_assert(IsVoidMemberFunc<ReturnType>::value || IsVoidMemberFunc<AssignType>::value ||
				std::is_same<BasicReturnType, BasicAssignType>::value,
			"Provided getter and setter functions must get and set the same type.");

		return CreateMemberScalarGetSetFuncDefinition<UnderlyingType>(name, member_get_func_ptr, member_set_func_ptr);
	}

	explicit operator bool() const { return type_register && struct_definition; }

private:
	// Member getter with reference return type.
	template <typename ReturnType>
	bool RegisterMemberGetter(const String& name, ReturnType& (Object::*member_get_func_ptr)())
	{
		return CreateMemberGetFuncDefinition<ReturnType>(name, member_get_func_ptr);
	}

	// Member getter with pointer return type.
	template <typename ReturnType>
	bool RegisterMemberGetter(const String& name, ReturnType* (Object::*member_get_func_ptr)())
	{
		return CreateMemberGetFuncDefinition<ReturnType>(name, member_get_func_ptr);
	}

	// Member getter with value return type, only valid for scalar return types.
	template <typename ReturnType>
	bool RegisterMemberGetter(const String& name, ReturnType (Object::*member_get_func_ptr)())
	{
		using SetType = VoidMemberFunc Object::*;
		return CreateMemberScalarGetSetFuncDefinition<ReturnType>(name, member_get_func_ptr, SetType{});
	}

	template <typename MemberType>
	bool CreateMemberObjectDefinition(const String& name, MemberType Object::*member_ptr);

	template <typename BasicReturnType, typename MemberType>
	bool CreateMemberGetFuncDefinition(const String& name, MemberType Object::*member_get_func_ptr);

	template <typename UnderlyingType, typename MemberGetType, typename MemberSetType>
	bool CreateMemberScalarGetSetFuncDefinition(const String& name, MemberGetType Object::*member_get_func_ptr,
		MemberSetType Object::*member_set_func_ptr);

	DataTypeRegister* type_register;
	StructDefinition* struct_definition;
};

template <typename Object>
template <typename MemberType>
bool StructHandle<Object>::CreateMemberObjectDefinition(const String& name, MemberType Object::*member_ptr)
{
	using MemberObjectPtr = MemberType Object::*;

	// If the member function signature doesn't match the getter function signature, it will end up calling this function. Emit a compile error in
	// that case.
	static_assert(!std::is_member_function_pointer<MemberObjectPtr>::value,
		"Illegal data member getter function signature. Make sure it takes no arguments and is not const qualified.");
	static_assert(!std::is_const<MemberType>::value, "Data member objects cannot be const qualified.");

	VariableDefinition* underlying_definition = type_register->GetDefinition<MemberType>();
	if (!underlying_definition)
		return false;
	struct_definition->AddMember(name, Rml::MakeUnique<MemberObjectDefinition<Object, MemberType>>(underlying_definition, member_ptr));
	return true;
}

template <typename Object>
template <typename BasicReturnType, typename MemberType>
bool StructHandle<Object>::CreateMemberGetFuncDefinition(const String& name, MemberType Object::*member_get_func_ptr)
{
	static_assert(!std::is_const<BasicReturnType>::value, "Returned type from data member function cannot be const qualified.");

	VariableDefinition* underlying_definition = type_register->GetDefinition<BasicReturnType>();
	if (!underlying_definition)
		return false;

	struct_definition->AddMember(name,
		Rml::MakeUnique<MemberGetFuncDefinition<Object, MemberType, BasicReturnType>>(underlying_definition, member_get_func_ptr));
	return true;
}

template <typename Object>
template <typename UnderlyingType, typename MemberGetType, typename MemberSetType>
bool StructHandle<Object>::CreateMemberScalarGetSetFuncDefinition(const String& name, MemberGetType Object::*member_get_func_ptr,
	MemberSetType Object::*member_set_func_ptr)
{
	static_assert(std::is_default_constructible<UnderlyingType>::value,
		"Struct member getter/setter functions must return/assign a type that is default constructible.");
	static_assert(!std::is_const<UnderlyingType>::value, "Const qualified type illegal in data member getter functions.");

	if (!IsVoidMemberFunc<MemberGetType>::value)
	{
		RMLUI_ASSERTMSG(member_get_func_ptr, "Expected member getter function, but none provided.");
	}
	if (!IsVoidMemberFunc<MemberSetType>::value)
	{
		RMLUI_ASSERTMSG(member_get_func_ptr, "Expected member setter function, but none provided.");
	}

	VariableDefinition* underlying_definition = type_register->GetDefinition<UnderlyingType>();
	if (!underlying_definition)
		return false;

	if (underlying_definition->Type() != DataVariableType::Scalar)
	{
		RMLUI_LOG_TYPE_ERROR(UnderlyingType,
			"Only scalar data variables are allowed here. Data member functions require scalar types when returning by value, or using getter/setter "
			"function pairs.");
		return false;
	}

	struct_definition->AddMember(name,
		Rml::MakeUnique<MemberScalarGetSetFuncDefinition<Object, MemberGetType, MemberSetType, UnderlyingType>>(underlying_definition,
			member_get_func_ptr, member_set_func_ptr));
	return true;
}

} // namespace Rml
#endif
