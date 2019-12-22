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
#include "Variant.h"
#include "StringUtilities.h"
#include "DataView.h"
#include "DataController.h"

namespace Rml {
namespace Core {

class DataVariable;
class DataMember;
class DataContainer;
class Element;


class DataAddress {

	struct Entry {
		enum Type { Array, Struct, Variable };
		int index;
		String name;
	};

	std::vector<Entry> address;

};



class DataModel {
public:
	bool GetValue(const String& name, Variant& out_value) const;
	bool SetValue(const String& name, const Variant& value) const;

	template<typename T>
	bool GetValue(const String& name, T& out_value) const {
		Variant variant;
		return GetValue(name, variant) && variant.GetInto<T>(out_value);
	}

	String ResolveVariableName(const String& raw_name, Element* parent) const;


	using Variables = UnorderedMap<String, UniquePtr<DataVariable>>;
	Variables variables;

	DataControllers controllers;
	DataViews views;

	using DataMembers = SmallUnorderedMap<String, UniquePtr<DataMember>>;
	using DataTypes = UnorderedMap<int, DataMembers>;
	DataTypes data_types;

	using DataContainers = UnorderedMap<String, UniquePtr<DataContainer>>;
	DataContainers containers;

	using ScopedAliases = UnorderedMap< Element*, SmallUnorderedMap<String, String> >;
	mutable ScopedAliases aliases;
};


class DataVariable {
public:
	DataVariable(void* ptr) : ptr(ptr) {}
	virtual ~DataVariable() = default;

	inline bool Get(Variant& out_value) {
		return Get(ptr, out_value);
	}
	inline bool Set(const Variant& in_value) {
		return Set(ptr, in_value);
	}

protected:
	virtual bool Get(const void* object, Variant& out_value) = 0;
	virtual bool Set(void* object, const Variant& in_value) = 0;

private:
	void* ptr;
};

class DataMember {
public:
	virtual ~DataMember() = default;
	virtual bool Get(const void* object, Variant& out_value) = 0;
	virtual bool Set(void* object, const Variant& in_value) = 0;
};



template<typename T>
class DataBindingDefault : public DataVariable {
public:
	DataBindingDefault(void* ptr) : DataVariable(ptr) {}

private:
	bool Get(const void* object, Variant& out_value) override {
		out_value = *static_cast<const T*>(object);
		return true;
	}
	bool Set(void* object, const Variant& in_value) override {
		T& target = *static_cast<T*>(object);
		return in_value.GetInto<T>(target);
	}
};




class DataBindingContext {
public:
	struct Item {
		String name;
		int index = -1;
	};
	DataBindingContext(std::vector<Item>&& in_items) : items(std::move(in_items)), it(items.begin()) {}

	operator bool() const { return it != items.end(); }

	const Item& Next() {
		RMLUI_ASSERT(it != items.end());
		return *(it++);
	}
private:
	std::vector<Item> items;
	std::vector<Item>::iterator it;
};


class DataContainer {
public:
	DataContainer(void* ptr) : ptr(ptr) {}
	virtual ~DataContainer() = default;

	inline bool Get(Variant& out_value, DataBindingContext& context) {
		return Get(ptr, out_value, context);
	}
	inline bool Set(const Variant& in_value, DataBindingContext& context) {
		return Set(ptr, in_value, context);
	}
	inline int Size() {
		return Size(ptr);
	}

protected:
	virtual bool Get(const void* container_ptr, Variant& out_value, DataBindingContext& context) = 0;
	virtual bool Set(void* container_ptr, const Variant& in_value, DataBindingContext& context) = 0;
	virtual int Size(void* container_ptr) = 0;

private:
	void* ptr;
};


template<typename T>
class DataContainerDefault : public DataContainer {
public:
	DataContainerDefault(void* ptr) : DataContainer(ptr) {}

private:
	bool Get(const void* container_ptr, Variant& out_value, DataBindingContext& context) override {
		if (!context)
		{
			RMLUI_ERROR;
			return false;
		}
		auto& item = context.Next();
		auto& container = *static_cast<const T*>(container_ptr);
		if (item.index < 0 || item.index >= (int)container.size())
		{
			Log::Message(Log::LT_WARNING, "Data container index out of bounds.");
			return false;
		}
		out_value = container[item.index];
		return true;
	}
	bool Set(void* container_ptr, const Variant& in_value, DataBindingContext& context) override {
		if (!context)
		{
			RMLUI_ERROR;
			return false;
		}
		auto& item = context.Next();
		auto& container = *static_cast<T*>(container_ptr);
		if (item.index < (int)container.size() || item.index >= (int)container.size())
		{
			Log::Message(Log::LT_WARNING, "Data container index out of bounds.");
			return false;
		}
		container[item.index] = in_value.Get< typename T::value_type >();
		return true;
	}
	int Size(void* container_ptr) override {
		return (int)static_cast<T*>(container_ptr)->size();
	}
};




class DataBindingMember : public DataVariable {
public:
	DataBindingMember(void* object, DataMember* member) : DataVariable(object), member(member) {}

private:
	bool Get(const void* object, Variant& out_value) override {
		return member->Get(object, out_value);
	}
	bool Set(void* object, const Variant& in_value) override {
		return member->Set(object, in_value);
	}

	DataMember* member;
};


template <typename Object, typename MemberType>
class DataMemberDefault : public DataMember {
public:
	DataMemberDefault(MemberType Object::* member_ptr) : member_ptr(member_ptr) {}

	bool Get(const void* object, Variant& out_value) override {
		out_value = static_cast<const Object*>(object)->*member_ptr;
		return true;
	}
	bool Set(void* object, const Variant& in_value) override {
		MemberType& target = static_cast<Object*>(object)->*member_ptr;
		return in_value.GetInto<MemberType>(target);
	}

private:
	MemberType Object::* member_ptr;
};



template <typename Object>
class DataTypeHandle {
public:
	DataTypeHandle(DataModel::DataMembers* members) : members(members) {}

	template <typename MemberType>
	DataTypeHandle& RegisterMember(String name, MemberType Object::* member_ptr)
	{
		RMLUI_ASSERT(members);
		members->emplace(name, std::make_unique<DataMemberDefault<Object, MemberType>>(member_ptr));
		return *this;
	}

private:
	DataModel::DataMembers* members;
};


class DataModelHandle {
public:
	DataModelHandle() : model(nullptr) {}
	DataModelHandle(DataModel* model) : model(model) {}

	template <typename T>
	DataModelHandle& BindValue(String name, T* object)
	{
		RMLUI_ASSERT(model);
		model->variables.emplace(name, std::make_unique<DataBindingDefault<T>>( object ));
		return *this;
	}

	template <typename Container>
	DataModelHandle& BindContainer(String name, Container* object)
	{
		RMLUI_ASSERT(model);
		//using T = Container::value_type;

		model->containers.emplace(name, std::make_unique<DataContainerDefault<Container>>(object));
		return *this;
	}

	template <typename T>
	DataModelHandle& BindTypeValue(String name, T* object)
	{
		RMLUI_ASSERT(model);

		int id = Family< typename std::remove_pointer< T >::type >::Id();

		auto it = model->data_types.find(id);
		if (it != model->data_types.end())
		{
			auto& members = it->second;
			for (auto& pair : members)
			{
				const String full_name = name + '.' + pair.first;
				DataMember* member = pair.second.get();
				bool inserted = model->variables.emplace(full_name, std::make_unique<DataBindingMember>(object, member)).second;
				RMLUI_ASSERT(inserted);
			}
		}

		return *this;
	}

	template <typename T>
	DataTypeHandle<T> RegisterType()
	{
		RMLUI_ASSERT(model);
		int id = Family< T >::Id();
		auto result = model->data_types.emplace(id, DataModel::DataMembers() );
		if (!result.second) {
			RMLUI_ERRORMSG("Type already registered.");
			return DataTypeHandle<T>(nullptr);
		}
		return DataTypeHandle<T>(&result.first->second);
	}

	void UpdateControllers() {
		RMLUI_ASSERT(model);
		model->controllers.Update(*model);
	}

	void UpdateViews() {
		RMLUI_ASSERT(model);
		model->views.Update(*model);
	}

	operator bool() { return model != nullptr; }

private:
	DataModel* model;
};

}
}

#endif
