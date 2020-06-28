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

#ifndef RMLUICOREDATAMODEL_H
#define RMLUICOREDATAMODEL_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "DataTypes.h"
#include "DataTypeRegister.h"

namespace Rml {
namespace Core {

class DataViews;
class DataControllers;
class Element;


class RMLUICORE_API DataModel : NonCopyMoveable {
public:
	DataModel(const TransformFuncRegister* transform_register = nullptr);
	~DataModel();

	void AddView(DataViewPtr view);
	void AddController(DataControllerPtr controller);

	bool BindVariable(const String& name, DataVariable variable);
	bool BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func);

	bool BindEventCallback(const String& name, DataEventFunc event_func);

	bool InsertAlias(Element* element, const String& alias_name, DataAddress replace_with_address);
	bool EraseAliases(Element* element);

	DataAddress ResolveAddress(const String& address_str, Element* element) const;
	const DataEventFunc* GetEventCallback(const String& name);

	DataVariable GetVariable(const DataAddress& address) const;
	bool GetVariableInto(const DataAddress& address, Variant& out_value) const;

	void DirtyVariable(const String& variable_name);
	bool IsVariableDirty(const String& variable_name) const;

	bool CallTransform(const String& name, Variant& inout_result, const VariantList& arguments) const;

	// Elements declaring 'data-model' need to be attached.
	void AttachModelRootElement(Element* element);
	ElementList GetAttachedModelRootElements() const;

	void OnElementRemove(Element* element);

	bool Update();

private:
	UniquePtr<DataViews> views;
	UniquePtr<DataControllers> controllers;

	UnorderedMap<String, DataVariable> variables;
	DirtyVariables dirty_variables;

	UnorderedMap<String, UniquePtr<FuncDefinition>> function_variable_definitions;
	UnorderedMap<String, DataEventFunc> event_callbacks;

	using ScopedAliases = UnorderedMap<Element*, SmallUnorderedMap<String, DataAddress>>;
	ScopedAliases aliases;

	const TransformFuncRegister* transform_register;

	SmallUnorderedSet<Element*> attached_elements;
};



class RMLUICORE_API DataModelHandle {
public:
	DataModelHandle(DataModel* model = nullptr) : model(model)
	{}

	void Update() {
		model->Update();
	}

	bool IsVariableDirty(const String& variable_name) {
		return model->IsVariableDirty(variable_name);
	}
	void DirtyVariable(const String& variable_name) {
		model->DirtyVariable(variable_name);
	}

	explicit operator bool() { return model; }

private:
	DataModel* model;
};


class RMLUICORE_API DataModelConstructor {
public:
	template<typename T>
	using DataEventMemberFunc = void(T::*)(DataModelHandle, Event&, const VariantList&);

	DataModelConstructor() : model(nullptr), type_register(nullptr) {}
	DataModelConstructor(DataModel* model, DataTypeRegister* type_register) : model(model), type_register(type_register) {
		RMLUI_ASSERT(model && type_register);
	}

	// Return a handle to the data model being constructed, which can later be used to synchronize variables and update the model.
	DataModelHandle GetModelHandle() const {
		return DataModelHandle(model);
	}

	// Bind a data variable.
	// @note For non-scalar types make sure they first have been registered with the appropriate 'Register...()' functions.
	template<typename T> bool Bind(const String& name, T* ptr) {
		RMLUI_ASSERTMSG(ptr, "Invalid pointer to data variable");
		return model->BindVariable(name, DataVariable(type_register->GetOrAddScalar<T>(), ptr));
	}

	// Bind a get/set function pair.
	bool BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func = {}) {
		return model->BindFunc(name, std::move(get_func), std::move(set_func));
	}

	// Bind an event callback.
	bool BindEventCallback(const String& name, DataEventFunc event_func) {
		return model->BindEventCallback(name, std::move(event_func));
	}
	// Convenience wrapper around BindEventCallback for member functions.
	template<typename T>
	bool BindEventCallback(const String& name, DataEventMemberFunc<T> member_func, T* object_pointer) {
		return BindEventCallback(name, std::bind(member_func, object_pointer, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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

	// Register a transform function.
	// A transform function modifies a variant with optional arguments. It can be called in data expressions using the pipe '|' operator.
	// @note The transform function applies to every data model associated with the current Context.
	void RegisterTransformFunc(const String& name, DataTransformFunc transform_func) {
		type_register->GetTransformFuncRegister()->Register(name, std::move(transform_func));
	}

	explicit operator bool() { return model && type_register; }

private:
	DataModel* model;
	DataTypeRegister* type_register;
};

}
}

#endif
