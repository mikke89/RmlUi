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

#ifndef RMLUI_CORE_DATAVARIABLE_H
#define RMLUI_CORE_DATAVARIABLE_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "Variant.h"
#include "DataTypes.h"
#include <iterator>

namespace Rml {

enum class DataVariableType { Scalar, Array, Struct, Function, MemberFunction };


class RMLUICORE_API DataVariable {
public:
	DataVariable() {}
	DataVariable(VariableDefinition* definition, void* ptr) : definition(definition), ptr(ptr) {}

	explicit operator bool() const { return definition; }

	bool Get(Variant& variant);
	bool Set(const Variant& variant);
	int Size();
	DataVariable Child(const DataAddressEntry& address);
	DataVariableType Type();

private:
	VariableDefinition* definition = nullptr;
	void* ptr = nullptr;
};



class RMLUICORE_API VariableDefinition {
public:
	virtual ~VariableDefinition() = default;
	DataVariableType Type() const { return type; }

	virtual bool Get(void* ptr, Variant& variant);
	virtual bool Set(void* ptr, const Variant& variant);

	virtual int Size(void* ptr);
	virtual DataVariable Child(void* ptr, const DataAddressEntry& address);

protected:
	VariableDefinition(DataVariableType type) : type(type) {}

private:
	DataVariableType type;
};


RMLUICORE_API DataVariable MakeLiteralIntVariable(int value);


template<typename T>
class ScalarDefinition final : public VariableDefinition {
public:
	ScalarDefinition() : VariableDefinition(DataVariableType::Scalar) {}

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

	FuncDefinition(DataGetFunc get, DataSetFunc set) : VariableDefinition(DataVariableType::Function), get(std::move(get)), set(std::move(set)) {}

	bool Get(void* /*ptr*/, Variant& variant) override
	{
		if (!get)
			return false;
		get(variant);
		return true;
	}
	bool Set(void* /*ptr*/, const Variant& variant) override
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
	ArrayDefinition(VariableDefinition* underlying_definition) : VariableDefinition(DataVariableType::Array), underlying_definition(underlying_definition) {}

	int Size(void* ptr) override {
		return int(static_cast<Container*>(ptr)->size());
	}

protected:
	DataVariable Child(void* void_ptr, const DataAddressEntry& address) override
	{
		Container* ptr = static_cast<Container*>(void_ptr);
		const int index = address.index;

		const int container_size = int(ptr->size());
		if (index < 0 || index >= container_size)
		{
			if (address.name == "size")
				return MakeLiteralIntVariable(container_size);

			Log::Message(Log::LT_WARNING, "Data array index out of bounds.");
			return DataVariable();
		}

		auto it = ptr->begin();
		std::advance(it, index);

		void* next_ptr = &(*it);
		return DataVariable(underlying_definition, next_ptr);
	}

private:
	VariableDefinition* underlying_definition;
};


class StructMember {
public:
	StructMember(VariableDefinition* definition) : definition(definition) {}
	virtual ~StructMember() = default;

	VariableDefinition* GetDefinition() const { return definition; }

	virtual void* GetPointer(void* base_ptr) = 0;

private:
	VariableDefinition* definition;
};

template <typename Object, typename MemberType>
class StructMemberObject final : public StructMember {
public:
	StructMemberObject(VariableDefinition* definition, MemberType Object::* member_ptr) : StructMember(definition), member_ptr(member_ptr) {}

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
	StructDefinition() : VariableDefinition(DataVariableType::Struct)
	{}

	DataVariable Child(void* ptr, const DataAddressEntry& address) override
	{
		const String& name = address.name;
		if (name.empty())
		{
			Log::Message(Log::LT_WARNING, "Expected a struct member name but none given.");
			return DataVariable();
		}

		auto it = members.find(name);
		if (it == members.end())
		{
			Log::Message(Log::LT_WARNING, "Member %s not found in data struct.", name.c_str());
			return DataVariable();
		}

		void* next_ptr = it->second->GetPointer(ptr);
		VariableDefinition* next_definition = it->second->GetDefinition();

		return DataVariable(next_definition, next_ptr);
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
	MemberFuncDefinition(MemberGetFunc<T> get, MemberSetFunc<T> set) : VariableDefinition(DataVariableType::MemberFunction), get(get), set(set) {}

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

} // namespace Rml
#endif
