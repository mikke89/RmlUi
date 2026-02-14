#pragma once

#include "DataTypes.h"
#include "DataVariable.h"
#include "Header.h"
#include "Traits.h"
#include "Types.h"
#include "Utilities.h"

namespace Rml {

template <class T>
constexpr bool is_builtin_data_scalar_v = std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<std::remove_const_t<T>, String>;

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
		if constexpr (!PointerTraits<T>::is_pointer_v && is_builtin_data_scalar_v<T>)
		{
			// Get definition for scalar types that can be assigned to and from Rml::Variant.
			// We automatically register these when needed, so users don't have to register trivial types manually.
			static_assert(!std::is_const_v<T>, "Data binding variables cannot point to constant variables.");
			FamilyId id = Family<T>::Id();

			auto result = type_register.emplace(id, nullptr);
			bool inserted = result.second;
			UniquePtr<VariableDefinition>& definition = result.first->second;

			if (inserted)
				definition = Rml::MakeUnique<ScalarDefinition<T>>();

			return definition.get();
		}
		else if constexpr (!PointerTraits<T>::is_pointer_v && !is_builtin_data_scalar_v<T>)
		{
			// Get definition for types that are not a built-in scalar.
			// These must already have been registered by the user.
			FamilyId id = Family<T>::Id();
			auto it = type_register.find(id);
			if (it == type_register.end())
			{
				RMLUI_LOG_TYPE_ERROR(T,
					"Desired data type T not registered with the type register, please use the 'Register...()' functions before binding values, "
					"adding "
					"members, or registering arrays of non-scalar types.");
				return nullptr;
			}

			return it->second.get();
		}
		else if constexpr (PointerTraits<T>::is_pointer_v)
		{
			// Get definition for pointer types, or create one as needed.
			// This will create a wrapper definition that forwards the call to the definition of the underlying type.
			static_assert(PointerTraits<T>::is_pointer_v, "Invalid pointer type provided.");

			using UnderlyingType = typename PointerTraits<T>::element_type;
			static_assert(!PointerTraits<UnderlyingType>::is_pointer_v,
				"Recursive pointer types (pointer to pointer) to data variables are disallowed.");
			static_assert(!std::is_const_v<UnderlyingType>, "Pointer to a const data variable is not supported.");

			// Get the underlying definition.
			VariableDefinition* underlying_definition = GetDefinition<UnderlyingType>();
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
		else
		{
			static_assert(Utilities::DependentFalse<T>);
			return nullptr;
		}
	}

	inline TransformFuncRegister* GetTransformFuncRegister() { return &transform_register; }

private:
	UnorderedMap<FamilyId, UniquePtr<VariableDefinition>> type_register;
	TransformFuncRegister transform_register;
};

} // namespace Rml
