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

#ifndef RMLUI_CORE_DATADEFINITIONS_H
#define RMLUI_CORE_DATADEFINITIONS_H

#include "Header.h"
#include "Types.h"
#include <type_traits>

namespace Rml {

class VariableDefinition;
class DataTypeRegister;
class TransformFuncRegister;
class DataModelHandle;
class DataVariable;

using DataGetFunc = Function<void(Variant&)>;
using DataSetFunc = Function<void(const Variant&)>;
using DataTransformFunc = Function<Variant(const VariantList&)>;
using DataEventFunc = Function<void(DataModelHandle, Event&, const VariantList&)>;

template <typename T>
using MemberGetFunc = void (T::*)(Variant&);
template <typename T>
using MemberSetFunc = void (T::*)(const Variant&);

template <typename T>
using DataTypeGetFunc = void (*)(const T&, Variant&);
template <typename T>
using DataTypeSetFunc = void (*)(T&, const Variant&);

template <typename Object, typename ReturnType>
using MemberGetterFunc = ReturnType (Object::*)();
template <typename Object, typename AssignType>
using MemberSetterFunc = void (Object::*)(AssignType);

using DirtyVariables = SmallUnorderedSet<String>;

struct DataAddressEntry {
	DataAddressEntry(String name) : name(std::move(name)), index(-1) {}
	DataAddressEntry(int index) : index(index) {}
	String name;
	int index;
};
using DataAddress = Vector<DataAddressEntry>;

template <class T>
struct PointerTraits {
	using is_pointer = std::false_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return ptr; }
};
template <class T>
struct PointerTraits<T*> {
	using is_pointer = std::true_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return static_cast<void*>(*static_cast<T**>(ptr)); }
};
template <class T>
struct PointerTraits<UniquePtr<T>> {
	using is_pointer = std::true_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return static_cast<void*>(static_cast<UniquePtr<T>*>(ptr)->get()); }
};
template <class T>
struct PointerTraits<SharedPtr<T>> {
	using is_pointer = std::true_type;
	using element_type = T;
	static void* Dereference(void* ptr) { return static_cast<void*>(static_cast<SharedPtr<T>*>(ptr)->get()); }
};

struct VoidMemberFunc {};
template <typename T>
using IsVoidMemberFunc = std::is_same<T, VoidMemberFunc>;

#define RMLUI_LOG_TYPE_ERROR(T, msg) RMLUI_ERRORMSG((String(msg) + String("\nT: ") + String(rmlui_type_name<T>())).c_str())
#define RMLUI_LOG_TYPE_ERROR_ASSERT(T, val, msg) RMLUI_ASSERTMSG((val), (String(msg) + String("\nT: ") + String(rmlui_type_name<T>())).c_str())

} // namespace Rml
#endif
