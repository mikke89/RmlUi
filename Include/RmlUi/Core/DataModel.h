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
#include "Variant.h"
#include "StringUtilities.h"
#include "DataView.h"
#include "DataController.h"

namespace Rml {
namespace Core {

class DataBinding;
class DataMember;


class DataModel {
public:
	bool GetValue(const String& name, Variant& out_value) const;
	bool SetValue(const String& name, const Variant& value) const;

	template<typename T>
	bool GetValue(const String& name, T& out_value) const {
		Variant variant;
		return GetValue(name, variant) && variant.GetInto<T>(out_value);
	}

	using Bindings = UnorderedMap<String, UniquePtr<DataBinding>>;
	Bindings bindings;

	DataControllers controllers;
	DataViews views;

	using DataMembers = SmallUnorderedMap<String, UniquePtr<DataMember>>;
	using DataTypes = UnorderedMap<String, DataMembers>;

	DataTypes data_types;
};



class DataBinding {
public:
	DataBinding(void* ptr) : ptr(ptr) {}
	virtual ~DataBinding() = default;

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
class DataBindingDefault : public DataBinding {
public:
	DataBindingDefault(void* ptr) : DataBinding(ptr) {}

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

class DataBindingMember : public DataBinding {
public:
	DataBindingMember(void* object, DataMember* member) : DataBinding(object), member(member) {}

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


class DataTypeHandle {
public:
	DataTypeHandle(DataModel::DataMembers* members) : members(members) {}

	template <typename Object, typename MemberType>
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
		model->bindings.emplace(name, std::make_unique<DataBindingDefault<T>>( object ));
		return *this;
	}

	DataModelHandle& BindTypeValue(String name, String type_name, void* object)
	{
		RMLUI_ASSERT(model);
		// Todo: We can make this type safe, removing the need for type_name.
		//   Make this a templated function, create another templated "family" class which assigns
		//   a unique id for each new type encountered, look up the type name there. Or use the ID as
		//   the look-up key.

		auto it = model->data_types.find(type_name);
		if (it != model->data_types.end())
		{
			auto& members = it->second;
			for (auto& pair : members)
			{
				const String full_name = name + '.' + pair.first;
				DataMember* member = pair.second.get();
				bool inserted = model->bindings.emplace(full_name, std::make_unique<DataBindingMember>(object, member)).second;
				RMLUI_ASSERT(inserted);
			}
		}

		return *this;
	}

	DataTypeHandle RegisterType(String name)
	{
		RMLUI_ASSERT(model);
		auto result = model->data_types.emplace(name, DataModel::DataMembers() );
		return DataTypeHandle(&result.first->second);
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
