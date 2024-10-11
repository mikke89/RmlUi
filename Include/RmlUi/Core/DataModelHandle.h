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

#ifndef RMLUI_CORE_DATAMODELHANDLE_H
#define RMLUI_CORE_DATAMODELHANDLE_H

#include "DataStructHandle.h"
#include "DataTypeRegister.h"
#include "DataTypes.h"
#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class DataModel;

class RMLUICORE_API DataModelHandle {
public:
	DataModelHandle(DataModel* model = nullptr);

	bool IsVariableDirty(const String& variable_name);
	void DirtyVariable(const String& variable_name);
	void DirtyAllVariables();

	explicit operator bool() { return model; }

private:
	DataModel* model;
};

class RMLUICORE_API DataModelConstructor {
public:
	template <typename T>
	using DataEventMemberFunc = void (T::*)(DataModelHandle, Event&, const VariantList&);

	DataModelConstructor();
	explicit DataModelConstructor(DataModel* model);

	// Return a handle to the data model being constructed, which can later be used to synchronize variables and update the model.
	DataModelHandle GetModelHandle() const;

	// Bind a data variable.
	// @note For non-builtin types, make sure they first have been registered with the appropriate 'Register...()' functions.
	template <typename T>
	bool Bind(const String& name, T* ptr)
	{
		RMLUI_ASSERTMSG(ptr, "Invalid pointer to data variable");
		return BindVariable(name, DataVariable(type_register->GetDefinition<T>(), static_cast<void*>(ptr)));
	}

	// Bind a get/set function pair.
	bool BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func = {});

	// Bind an event callback.
	bool BindEventCallback(const String& name, DataEventFunc event_func);

	// Convenience wrapper around BindEventCallback for member functions.
	template <typename T>
	bool BindEventCallback(const String& name, DataEventMemberFunc<T> member_func, T* object_pointer)
	{
		return BindEventCallback(name, [member_func, object_pointer](DataModelHandle handle, Event& event, const VariantList& arguments) {
			(object_pointer->*member_func)(handle, event, arguments);
		});
	}

	// Bind a user-declared DataVariable.
	// For advanced use cases, for example for binding variables to a custom 'VariableDefinition'.
	bool BindCustomDataVariable(const String& name, DataVariable data_variable) { return BindVariable(name, data_variable); }

	// Register a scalar type with associated get and set functions.
	// @note This registers a type which can later be used as a normal data variable, while 'BindFunc' registers a named data variable with a specific
	// getter and setter.
	// @note The type applies to every data model associated with the current Context.
	template <typename T>
	bool RegisterScalar(DataTypeGetFunc<T> get_func, DataTypeSetFunc<T> set_func = {});

	// Register a struct type.
	// @note The type applies to every data model associated with the current Context.
	// @return A handle which can be used to register struct members.
	template <typename T>
	StructHandle<T> RegisterStruct();

	// Register a user-declared VariableDefinition to describe a custom type behaviour.
	template <typename T>
	bool RegisterCustomDataVariableDefinition(UniquePtr<VariableDefinition> definition);

	// Register an array type.
	// @note The type applies to every data model associated with the current Context.
	// @note If 'Container::value_type' represents a non-scalar type, that type must already have been registered with the appropriate 'Register...()'
	// functions.
	// @note Container requires the following functions to be implemented: size() and begin(). This is satisfied by several containers including
	// std::vector and std::array.
	template <typename Container>
	bool RegisterArray();

	// Register a transform function.
	// A transform function modifies a variant with optional arguments. It can be called in data expressions using the pipe '|' operator.
	// @note The transform function applies to every data model associated with the current Context.
	void RegisterTransformFunc(const String& name, DataTransformFunc transform_func)
	{
		type_register->GetTransformFuncRegister()->Register(name, std::move(transform_func));
	}

	// Returns the type register.
	// The type register contains VariableDefinitions of all the data types registered to this data model's owning context.
	DataTypeRegister* GetDataTypeRegister() const { return type_register; }

	explicit operator bool() { return model && type_register; }

private:
	bool BindVariable(const String& name, DataVariable data_variable);

	DataModel* model;
	DataTypeRegister* type_register;
};

template <typename T>
inline bool DataModelConstructor::RegisterScalar(DataTypeGetFunc<T> get_func, DataTypeSetFunc<T> set_func)
{
	// Though enum is builtin scalar type, we allow custom getter/setter on it.
	static_assert(!is_builtin_data_scalar<T>::value || std::is_enum<T>::value,
		"Cannot register scalar data type function. Arithmetic types and String are handled internally and does not need to be registered.");
	const FamilyId id = Family<T>::Id();

	auto scalar_func_definition = Rml::MakeUnique<ScalarFuncDefinition<T>>(get_func, set_func);

	const bool inserted = type_register->RegisterDefinition(id, std::move(scalar_func_definition));
	if (!inserted)
	{
		RMLUI_LOG_TYPE_ERROR(T, "Scalar function type already registered.");
		return false;
	}

	return true;
}

template <typename T>
inline bool DataModelConstructor::RegisterCustomDataVariableDefinition(UniquePtr<VariableDefinition> definition)
{
	const FamilyId id = Family<T>::Id();

	const bool inserted = type_register->RegisterDefinition(id, std::move(definition));
	if (!inserted)
	{
		RMLUI_LOG_TYPE_ERROR(T, "Custom data type already registered.");
		return false;
	}

	return true;
}

template <typename T>
inline StructHandle<T> DataModelConstructor::RegisterStruct()
{
	static_assert(std::is_class<T>::value, "Type must be a struct or class type.");
	const FamilyId id = Family<T>::Id();

	auto struct_definition = Rml::MakeUnique<StructDefinition>();
	StructDefinition* struct_variable_raw = struct_definition.get();

	const bool inserted = type_register->RegisterDefinition(id, std::move(struct_definition));
	if (!inserted)
	{
		RMLUI_LOG_TYPE_ERROR(T, "Struct type already declared");
		return StructHandle<T>(nullptr, nullptr);
	}

	return StructHandle<T>(type_register, struct_variable_raw);
}

template <typename Container>
inline bool DataModelConstructor::RegisterArray()
{
	using value_type = typename Container::value_type;
	VariableDefinition* value_variable = type_register->GetDefinition<value_type>();
	RMLUI_LOG_TYPE_ERROR_ASSERT(value_type, value_variable, "Underlying value type of array has not been registered.");
	if (!value_variable)
		return false;

	const FamilyId container_id = Family<Container>::Id();
	auto array_definition = Rml::MakeUnique<ArrayDefinition<Container>>(value_variable);

	const bool inserted = type_register->RegisterDefinition(container_id, std::move(array_definition));
	if (!inserted)
	{
		RMLUI_LOG_TYPE_ERROR(Container, "Array type already declared.");
		return false;
	}

	return true;
}

} // namespace Rml

#endif
