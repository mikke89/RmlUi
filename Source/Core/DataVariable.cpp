#include "../../Include/RmlUi/Core/DataVariable.h"

namespace Rml {

bool DataVariable::Get(Variant& variant) const
{
	return definition->Get(ptr, variant);
}

bool DataVariable::Set(const Variant& variant)
{
	return definition->Set(ptr, variant);
}

int DataVariable::Size() const
{
	return definition->Size(ptr);
}

DataVariable DataVariable::Child(const DataAddressEntry& address) const
{
	return definition->Child(ptr, address);
}

DataVariableType DataVariable::Type() const
{
	return definition->Type();
}

bool VariableDefinition::Get(void* /*ptr*/, Variant& /*variant*/)
{
	Log::Message(Log::LT_WARNING, "Values can only be retrieved from scalar data types.");
	return false;
}
bool VariableDefinition::Set(void* /*ptr*/, const Variant& /*variant*/)
{
	Log::Message(Log::LT_WARNING, "Values can only be assigned to scalar data types.");
	return false;
}
int VariableDefinition::Size(void* /*ptr*/)
{
	Log::Message(Log::LT_WARNING, "Tried to get the size from a non-array data type.");
	return 0;
}
DataVariable VariableDefinition::Child(void* /*ptr*/, const DataAddressEntry& /*address*/)
{
	Log::Message(Log::LT_WARNING, "Tried to get the child of a scalar type.");
	return DataVariable();
}

StringList VariableDefinition::ReflectMemberNames()
{
	Log::Message(Log::LT_WARNING, "Tried to get the member names of a non-struct type.");
	return StringList();
}

class LiteralIntDefinition final : public VariableDefinition {
public:
	LiteralIntDefinition() : VariableDefinition(DataVariableType::Scalar) {}

	bool Get(void* ptr, Variant& variant) override
	{
		variant = static_cast<int>(reinterpret_cast<intptr_t>(ptr));
		return true;
	}
};

DataVariable MakeLiteralIntVariable(int value)
{
	static LiteralIntDefinition literal_int_definition;
	return DataVariable(&literal_int_definition, reinterpret_cast<void*>(static_cast<intptr_t>(value)));
}

StructDefinition::StructDefinition() : VariableDefinition(DataVariableType::Struct) {}

DataVariable StructDefinition::Child(void* ptr, const DataAddressEntry& address)
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

	VariableDefinition* next_definition = it->second.get();

	return DataVariable(next_definition, ptr);
}

StringList StructDefinition::ReflectMemberNames()
{
	StringList names;
	names.reserve(members.size());
	for (const auto& entry : members)
		names.push_back(entry.first);
	return names;
}

void StructDefinition::AddMember(const String& name, UniquePtr<VariableDefinition> member)
{
	RMLUI_ASSERT(member);
	bool inserted = members.emplace(name, std::move(member)).second;
	RMLUI_ASSERTMSG(inserted, "Member name already exists.");
	(void)inserted;
}

FuncDefinition::FuncDefinition(DataGetFunc get, DataSetFunc set) :
	VariableDefinition(DataVariableType::Scalar), get(std::move(get)), set(std::move(set))
{}

bool FuncDefinition::Get(void* /*ptr*/, Variant& variant)
{
	if (!get)
		return false;
	get(variant);
	return true;
}

bool FuncDefinition::Set(void* /*ptr*/, const Variant& variant)
{
	if (!set)
		return false;
	set(variant);
	return true;
}

BasePointerDefinition::BasePointerDefinition(VariableDefinition* underlying_definition) :
	VariableDefinition(underlying_definition->Type()), underlying_definition(underlying_definition)
{}

bool BasePointerDefinition::Get(void* ptr, Variant& variant)
{
	if (!ptr)
		return false;
	return underlying_definition->Get(DereferencePointer(ptr), variant);
}

bool BasePointerDefinition::Set(void* ptr, const Variant& variant)
{
	if (!ptr)
		return false;
	return underlying_definition->Set(DereferencePointer(ptr), variant);
}

int BasePointerDefinition::Size(void* ptr)
{
	if (!ptr)
		return 0;
	return underlying_definition->Size(DereferencePointer(ptr));
}

DataVariable BasePointerDefinition::Child(void* ptr, const DataAddressEntry& address)
{
	if (!ptr)
		return DataVariable();
	return underlying_definition->Child(DereferencePointer(ptr), address);
}

} // namespace Rml
