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

#ifndef RMLUI_CORE_DATATYPEREGISTER_H
#define RMLUI_CORE_DATATYPEREGISTER_H

#include "DataTypes.h"
#include "DataVariable.h"
#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

template <typename T>
struct is_builtin_data_scalar {
	static constexpr bool value =
		std::is_arithmetic<T>::value || std::is_enum<T>::value || std::is_same<typename std::remove_const<T>::type, String>::value;
};

class RMLUICORE_API TransformFuncRegister {
public:
	void Register(const String& name, DataTransformFunc transform_func);
	bool Call(const String& name, const VariantList& arguments, Variant& out_result) const;

private:
	UnorderedMap<String, DataTransformFunc> transform_functions;
};

class RMLUICORE_API DataTypeRegister final : NonCopyMoveable {
public:
	DataTypeRegister();
	~DataTypeRegister();

	inline bool RegisterDefinition(FamilyId id, UniquePtr<VariableDefinition> definition)
	{
		const bool inserted = type_register.emplace(id, std::move(definition)).second;
		return inserted;
	}

	template <typename T>
	VariableDefinition* GetDefinition()
	{
		return GetDefinitionDetail<T>();
	}

	inline TransformFuncRegister* GetTransformFuncRegister() { return &transform_register; }

private:
	// Get definition for scalar types that can be assigned to and from Rml::Variant.
	// We automatically register these when needed, so users don't have to register trivial types manually.
	template <typename T, typename std::enable_if<!PointerTraits<T>::is_pointer::value && is_builtin_data_scalar<T>::value, int>::type = 0>
	VariableDefinition* GetDefinitionDetail()
	{
		static_assert(!std::is_const<T>::value, "Data binding variables cannot point to constant variables.");
		FamilyId id = Family<T>::Id();

		auto result = type_register.emplace(id, nullptr);
		bool inserted = result.second;
		UniquePtr<VariableDefinition>& definition = result.first->second;

		if (inserted)
			definition = Rml::MakeUnique<ScalarDefinition<T>>();

		return definition.get();
	}

	// Get definition for types that are not a built-in scalar.
	// These must already have been registered by the user.
	template <typename T, typename std::enable_if<!PointerTraits<T>::is_pointer::value && !is_builtin_data_scalar<T>::value, int>::type = 0>
	VariableDefinition* GetDefinitionDetail()
	{
		FamilyId id = Family<T>::Id();
		auto it = type_register.find(id);
		if (it == type_register.end())
		{
			RMLUI_LOG_TYPE_ERROR(T,
				"Desired data type T not registered with the type register, please use the 'Register...()' functions before binding values, adding "
				"members, or registering arrays of non-scalar types.");
			return nullptr;
		}

		return it->second.get();
	}

	// Get definition for pointer types, or create one as needed.
	// This will create a wrapper definition that forwards the call to the definition of the underlying type.
	template <typename T, typename std::enable_if<PointerTraits<T>::is_pointer::value, int>::type = 0>
	VariableDefinition* GetDefinitionDetail()
	{
		static_assert(PointerTraits<T>::is_pointer::value, "Invalid pointer type provided.");

		using UnderlyingType = typename PointerTraits<T>::element_type;
		static_assert(!PointerTraits<UnderlyingType>::is_pointer::value,
			"Recursive pointer types (pointer to pointer) to data variables are disallowed.");
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
			definition = Rml::MakeUnique<PointerDefinition<T>>(underlying_definition);

		return definition.get();
	}

	UnorderedMap<FamilyId, UniquePtr<VariableDefinition>> type_register;
	TransformFuncRegister transform_register;
};

} // namespace Rml
#endif
