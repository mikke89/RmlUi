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

#ifndef RMLUI_CORE_DATAMODELHANDLE_H
#define RMLUI_CORE_DATAMODELHANDLE_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "DataTypes.h"
#include "DataTypeRegister.h"

namespace Rml {

class DataModel;


class RMLUICORE_API DataModelHandle {
public:
	DataModelHandle(DataModel* model = nullptr);

	bool IsVariableDirty(const String& variable_name);
	void DirtyVariable(const String& variable_name);

	explicit operator bool() { return model; }

private:
	DataModel* model;
};


class RMLUICORE_API DataModelConstructor {
public:
	template<typename T>
	using DataEventMemberFunc = void(T::*)(DataModelHandle, Event&, const VariantList&);

	DataModelConstructor();
	DataModelConstructor(DataModel* model, DataTypeRegister* type_register);

	// Return a handle to the data model being constructed, which can later be used to synchronize variables and update the model.
	DataModelHandle GetModelHandle() const;

	// Bind a data variable.
	// @note For non-scalar types make sure they first have been registered with the appropriate 'Register...()' functions.
	template<typename T>
	bool Bind(const String& name, T* ptr) {
		RMLUI_ASSERTMSG(ptr, "Invalid pointer to data variable");
		return BindVariable(name, DataVariable(type_register->GetOrAddScalar<T>(), ptr));
	}

	// Bind a get/set function pair.
	bool BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func = {});

	// Bind an event callback.
	bool BindEventCallback(const String& name, DataEventFunc event_func);

	// Convenience wrapper around BindEventCallback for member functions.
	template<typename T>
	bool BindEventCallback(const String& name, DataEventMemberFunc<T> member_func, T* object_pointer) {
		return BindEventCallback(name, [member_func, object_pointer](DataModelHandle handle, Event& event, const VariantList& arguments) {
			(object_pointer->*member_func)(handle, event, arguments);
		});
	}

	// Bind a user-declared DataVariable.
	// For advanced use cases, for example for binding variables to a custom 'VariableDefinition'.
	bool BindCustomDataVariable(const String& name, DataVariable data_variable) {
		return BindVariable(name, data_variable);
	}

	// Register a struct type.
	// @note The type applies to every data model associated with the current Context.
	// @return A handle which can be used to register struct members.
	template<typename T>
	StructHandle<T> RegisterStruct() {
		return type_register->RegisterStruct<T>();
	}

	// Register an array type.
	// @note The type applies to every data model associated with the current Context.
	// @note If 'Container::value_type' represents a non-scalar type, that type must already have been registered with the appropriate 'Register...()' functions.
	// @note Container requires the following functions to be implemented: size() and begin(). This is satisfied by several containers such as std::vector and std::array.
	template<typename Container>
	bool RegisterArray() {
		return type_register->RegisterArray<Container>();
	}

	// Register a pointer type.
	// @note The type applies to every data model associated with the current Context.
	// @note If 'PointerType' represents a non-scalar type, that type must already have been registered with the appropriate 'Register...()' functions.
	template<typename PointerType>
	bool RegisterPointer() {
		return type_register->RegisterPointer<PointerType>();
	}

	// Register an array type.
	// @note The type applies to every data model associated with the current Context.
	// @note If 'PointerType::element_type' represents a non-scalar type, that type must already have been registered with the appropriate 'Register...()' functions.
	// @note PointerType requires the following functions to be implemented: PointerType::element_type* get(). This is satisfied by several "pointer containers" such as std::shared_ptr.
	template<typename PointerType>
	bool RegisterComplexPointer() {
		return type_register->RegisterComplexPointer<PointerType>();
	}

	// Register a transform function.
	// A transform function modifies a variant with optional arguments. It can be called in data expressions using the pipe '|' operator.
	// @note The transform function applies to every data model associated with the current Context.
	void RegisterTransformFunc(const String& name, DataTransformFunc transform_func) {
		type_register->GetTransformFuncRegister()->Register(name, std::move(transform_func));
	}

	explicit operator bool() { return model && type_register; }

private:
	bool BindVariable(const String& name, DataVariable data_variable);

	DataModel* model;
	DataTypeRegister* type_register;
};

} // namespace Rml

#endif
