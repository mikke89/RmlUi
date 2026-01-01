#include "../../Include/RmlUi/Core/DataModelHandle.h"
#include "DataModel.h"

namespace Rml {

DataModelHandle::DataModelHandle(DataModel* model) : model(model) {}

bool DataModelHandle::IsVariableDirty(const String& variable_name)
{
	return model->IsVariableDirty(variable_name);
}

void DataModelHandle::DirtyVariable(const String& variable_name)
{
	model->DirtyVariable(variable_name);
}

void DataModelHandle::DirtyAllVariables()
{
	model->DirtyAllVariables();
}

DataModelConstructor::DataModelConstructor() : model(nullptr), type_register(nullptr) {}

DataModelConstructor::DataModelConstructor(DataModel* model) : model(model), type_register(model->GetDataTypeRegister())
{
	RMLUI_ASSERT(model);
}

DataModelHandle DataModelConstructor::GetModelHandle() const
{
	return DataModelHandle(model);
}

bool DataModelConstructor::BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func)
{
	return model->BindFunc(name, std::move(get_func), std::move(set_func));
}

bool DataModelConstructor::BindEventCallback(const String& name, DataEventFunc event_func)
{
	return model->BindEventCallback(name, std::move(event_func));
}

bool DataModelConstructor::BindVariable(const String& name, DataVariable data_variable)
{
	return model->BindVariable(name, data_variable);
}

const UnorderedMap<String, DataVariable>& Detail::DataModelConstructorAccessor::GetAllVariables(const DataModelConstructor& data_model_constructor)
{
	RMLUI_ASSERT(data_model_constructor.model);
	return data_model_constructor.model->GetAllVariables();
}

} // namespace Rml
