#include "ElementDataModels.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/DataModelHandle.h"
#include "../../Include/RmlUi/Core/DataVariable.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "CommonSource.h"
#include "DataModelsSource.h"
#include <algorithm>

namespace Rml {
namespace Debugger {

static void ReadDataVariableRecursive(String& out_rml, const int indent_level, const DataVariable variable)
{
	constexpr int max_num_child_elements = 50;

	String indent_child_element;
	for (int i = 0; i <= indent_level; i++)
		indent_child_element += "&nbsp;&nbsp;";

	switch (variable.Type())
	{
	case DataVariableType::Scalar:
	{
		Variant variant;
		if (!variable.Get(variant))
			out_rml += "<em>(unreadable)</em>";
		else if (variant.GetType() == Variant::NONE)
			out_rml += "<em>(none)</em>";
		else if (variant.GetType() == Variant::BOOL)
			out_rml += variant.Get<bool>() ? "true" : "false";
		else
			out_rml += StringUtilities::EncodeRml(variant.Get<String>());
		out_rml += "<br/>";
	}
	break;
	case DataVariableType::Array:
	{
		const int size = variable.Size();
		out_rml += "Array (" + ToString(variable.Size()) + ")<br/>";
		for (int i = 0; i < size; i++)
		{
			if (i >= max_num_child_elements)
			{
				out_rml += indent_child_element + "<span class='name'>[...]</span><br/>";
				break;
			}

			out_rml += indent_child_element + "<span class='name'>[" + ToString(i) + "]</span>: ";
			DataVariable child_variable = variable.Child(DataAddressEntry(i));
			ReadDataVariableRecursive(out_rml, indent_level + 1, child_variable);
		}
	}
	break;
	case DataVariableType::Struct:
	{
		const StringList members = Detail::DataVariableAccessor::GetDefinition(variable)->ReflectMemberNames();
		out_rml += "Struct (" + ToString(members.size()) + ")<br/>";
		for (const String& member_name : members)
		{
			if (std::distance(&members.front(), &member_name) >= max_num_child_elements)
			{
				out_rml += indent_child_element + "<span class='name'>[...]</span><br/>";
				break;
			}

			out_rml += indent_child_element + "<span class='name'>." + member_name + "</span>: ";
			DataVariable child_variable = variable.Child(DataAddressEntry(member_name));
			ReadDataVariableRecursive(out_rml, indent_level + 1, child_variable);
		}
	}
	break;
	}
}

ElementDataModels::ElementDataModels(const String& tag) : ElementDebugDocument(tag) {}

ElementDataModels::~ElementDataModels()
{
	RemoveEventListener(EventId::Click, this);
}

bool ElementDataModels::Initialise(Context* target_context)
{
	SetInnerRML(data_models_rml);
	SetId("rmlui-debug-data-models");

	AddEventListener(EventId::Click, this);

	SharedPtr<StyleSheetContainer> style_sheet = Factory::InstanceStyleSheetString(String(common_rcss) + String(data_models_rcss));
	if (!style_sheet)
		return false;

	SetStyleSheetContainer(std::move(style_sheet));

	SetDebugContext(target_context);

	return true;
}

void ElementDataModels::Reset()
{
	SetDebugContext(nullptr);
}

void ElementDataModels::SetDebugContext(Context* new_debug_context)
{
	debug_context = new_debug_context;
}

void ElementDataModels::OnUpdate()
{
	if (!IsVisible() || !debug_context)
		return;

	const double t = GetSystemInterface()->GetElapsedTime();
	const float dt = (float)(t - previous_update_time);

	constexpr float update_interval = 0.3f;

	if (dt > update_interval)
	{
		previous_update_time = t;
		UpdateContent();
	}
}

void ElementDataModels::ProcessEvent(Event& event)
{
	if (!IsVisible())
		return;

	Element* target_element = event.GetTargetElement();
	if (target_element->GetOwnerDocument() != this)
		return;

	if (event == EventId::Click)
	{
		const String& id = event.GetTargetElement()->GetId();

		if (id == "close_button")
			Hide();

		event.StopPropagation();
	}
}

void ElementDataModels::UpdateContent()
{
	RMLUI_ASSERT(debug_context);
	Element* models_content_element = GetElementById("content");

	SmallOrderedMap<String, String> new_model_rml_map;

	const UnorderedMap<String, DataModelConstructor> data_models = debug_context->GetDataModels();
	for (const auto& name_model_pair : data_models)
	{
		const String& model_name = name_model_pair.first;
		const DataModelConstructor& model_constructor = name_model_pair.second;

		String& model_rml = new_model_rml_map[model_name];
		model_rml += "<div class='model'>";
		model_rml += "<h2>" + model_name + "</h2>";
		model_rml += "<div class='model-content'>";

		const UnorderedMap<String, DataVariable>& variables = Detail::DataModelConstructorAccessor::GetAllVariables(model_constructor);
		for (const auto& name_variable_pair : variables)
		{
			const String& name = name_variable_pair.first;
			const DataVariable& variable = name_variable_pair.second;
			model_rml += "<span class='name'>" + name + "</span>: ";
			ReadDataVariableRecursive(model_rml, 0, variable);
		}

		if (variables.empty())
			model_rml += "<em>No data variables in data model.</em><br/>";

		model_rml += "</div>";
		model_rml += "</div>";
	}

	if (new_model_rml_map != model_rml_map)
	{
		model_rml_map = std::move(new_model_rml_map);

		String models_rml;
		for (const auto& name_rml_pair : model_rml_map)
			models_rml += name_rml_pair.second;

		if (data_models.empty())
			models_rml = "<em>No data models in context.</em>";

		models_content_element->SetInnerRML(models_rml);
	}
}

} // namespace Debugger
} // namespace Rml
