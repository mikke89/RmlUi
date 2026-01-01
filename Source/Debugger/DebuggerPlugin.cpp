#include "DebuggerPlugin.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementInstancer.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "DebuggerSystemInterface.h"
#include "ElementContextHook.h"
#include "ElementDataModels.h"
#include "ElementDebugDocument.h"
#include "ElementInfo.h"
#include "ElementLog.h"
#include "FontSource.h"
#include "Geometry.h"
#include "MenuSource.h"
#include <stack>

namespace Rml {
namespace Debugger {

DebuggerPlugin* DebuggerPlugin::instance = nullptr;

DebuggerPlugin::DebuggerPlugin()
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;
	host_context = nullptr;
	debug_context = nullptr;
	log_interface = nullptr;

	menu_element = nullptr;
	info_element = nullptr;
	log_element = nullptr;
	data_explorer_element = nullptr;
	hook_element = nullptr;

	render_outlines = false;

	application_interface = nullptr;
}

DebuggerPlugin::~DebuggerPlugin()
{
	instance = nullptr;
}

bool DebuggerPlugin::Initialise(Context* context)
{
	host_context = context;
	Geometry::SetContext(context);

	if (!LoadFont())
	{
		Log::Message(Log::LT_ERROR, "Failed to initialise debugger, unable to load font.");
		return false;
	}

	if (!LoadMenuElement() || !LoadInfoElement() || !LoadLogElement() || !LoadDataExplorerElement())
	{
		Log::Message(Log::LT_ERROR, "Failed to initialise debugger, error while load debugger elements.");
		return false;
	}

	hook_element_instancer = MakeUnique<ElementInstancerGeneric<ElementContextHook>>();
	Factory::RegisterElementInstancer("debug-hook", hook_element_instancer.get());

	return true;
}

bool DebuggerPlugin::SetContext(Context* context)
{
	if (debug_context && hook_element)
	{
		debug_context->UnloadDocument(hook_element);
		hook_element = nullptr;
	}

	if (context)
	{
		ElementDocument* element = context->CreateDocument("debug-hook");
		if (!element)
			return false;

		RMLUI_ASSERT(!hook_element);
		hook_element = rmlui_dynamic_cast<ElementContextHook*>(element);
		if (!hook_element)
		{
			context->UnloadDocument(element);
			return false;
		}

		hook_element->Initialise(this);
	}

	if (info_element)
	{
		SetupInfoListeners(context);
		info_element->Reset();
	}

	if (data_explorer_element)
	{
		data_explorer_element->SetDebugContext(context);
	}

	debug_context = context;
	return true;
}

void DebuggerPlugin::SetVisible(bool visibility)
{
	if (visibility)
		menu_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));
	else
		menu_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
}

bool DebuggerPlugin::IsVisible()
{
	return menu_element->IsVisible();
}

void DebuggerPlugin::Render()
{
	// Render the outlines of the debug context's elements.
	if (render_outlines && debug_context)
	{
		for (int i = 0; i < debug_context->GetNumDocuments(); ++i)
		{
			ElementDocument* document = debug_context->GetDocument(i);
			if (document->GetId().find("rmlui-debug-") == 0)
				continue;

			Stack<Element*> element_stack;
			element_stack.push(document);

			while (!element_stack.empty())
			{
				Element* element = element_stack.top();
				element_stack.pop();
				if (element->IsVisible())
				{
					ElementUtilities::ApplyTransform(*element);
					for (int j = 0; j < element->GetNumBoxes(); ++j)
					{
						const RenderBox box = element->GetRenderBox(BoxArea::Border, j);
						Geometry::RenderOutline(element->GetAbsoluteOffset(BoxArea::Border) + box.GetBorderOffset(), box.GetFillSize(),
							Colourb(255, 0, 0, 128), 1);
					}

					for (int j = 0; j < element->GetNumChildren(); ++j)
						element_stack.push(element->GetChild(j));
				}
			}
		}
	}

	// Render the info element's boxes.
	if (info_element && info_element->IsVisible())
	{
		info_element->RenderHoverElement();
		info_element->RenderSourceElement();
	}
}

void DebuggerPlugin::OnShutdown()
{
	// Release the elements before we leak track, this ensures the debugger hook has been cleared
	// and that we don't try send the messages to the debug log window
	ReleaseElements();

	hook_element_instancer.reset();

	delete this;
}

void DebuggerPlugin::OnContextDestroy(Context* context)
{
	if (context == debug_context)
	{
		// The context we're debugging is being destroyed, so we need to remove our debug hook elements.
		SetContext(nullptr);
	}

	if (context == host_context)
	{
		// Our host is being destroyed, so we need to shut down the debugger.

		ReleaseElements();

		Geometry::SetContext(nullptr);
		host_context = nullptr;
	}
}

void DebuggerPlugin::OnElementDestroy(Element* element)
{
	// Detect external destruction of the debugger documents. This can happen for example if the user calls
	// `Context::UnloadAllDocuments()` on the host context.
	if (element == menu_element || element == info_element || element == log_element || element == data_explorer_element)
	{
		ReleaseElements();
		Log::Message(Log::LT_ERROR,
			"A document owned by the Debugger plugin was destroyed externally. This is not allowed. Consider shutting down the debugger instead.");
	}

	if (info_element)
		info_element->OnElementDestroy(element);
}

void DebuggerPlugin::ProcessEvent(Event& event)
{
	struct ButtonIdToDocumentMapping {
		String id;
		ElementDocument* document;
	};
	const ButtonIdToDocumentMapping button_mappings[] = {
		{"event-log-button", log_element},
		{"debug-info-button", info_element},
		{"data-models-button", data_explorer_element},
	};

	if (event == EventId::Click)
	{
		for (const ButtonIdToDocumentMapping& button_mapping : button_mappings)
		{
			if (event.GetTargetElement()->GetId() == button_mapping.id)
			{
				if (button_mapping.document->IsVisible())
					button_mapping.document->Hide();
				else
					button_mapping.document->Show();
			}
		}

		if (event.GetTargetElement()->GetId() == "outlines-button")
		{
			render_outlines = !render_outlines;
			event.GetTargetElement()->SetClass("open", render_outlines);
		}
	}
	else if (event == EventId::Hide || event == EventId::Show)
	{
		for (const ButtonIdToDocumentMapping& button_mapping : button_mappings)
		{
			if (event.GetTargetElement() == button_mapping.document)
			{
				Element* button = menu_element->GetElementById(button_mapping.id);
				const bool set_open = (event == EventId::Show);
				button->SetClass("open", set_open);
			}
		}
	}
}

DebuggerPlugin* DebuggerPlugin::GetInstance()
{
	return instance;
}

bool DebuggerPlugin::LoadFont()
{
	const String font_family_name = "rmlui-debugger-font";

	return (LoadFontFace({courier_prime_code, sizeof(courier_prime_code)}, font_family_name, Style::FontStyle::Normal, Style::FontWeight::Normal) &&
		LoadFontFace({courier_prime_code_italic, sizeof(courier_prime_code_italic)}, font_family_name, Style::FontStyle::Italic,
			Style::FontWeight::Normal));
}

bool DebuggerPlugin::LoadMenuElement()
{
	debug_document_instancer = MakeUnique<ElementInstancerGeneric<ElementDebugDocument>>();
	Factory::RegisterElementInstancer("debug-document", debug_document_instancer.get());
	menu_element = host_context->CreateDocument("debug-document");
	if (!menu_element)
		return false;

	menu_element->SetId("rmlui-debug-menu");
	menu_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
	menu_element->SetInnerRML(menu_rml);

	SharedPtr<StyleSheetContainer> style_sheet = Factory::InstanceStyleSheetString(menu_rcss);
	if (!style_sheet)
	{
		host_context->UnloadDocument(menu_element);
		menu_element = nullptr;
		return false;
	}

	menu_element->SetStyleSheetContainer(std::move(style_sheet));

	menu_element->GetElementById("version-number")->SetInnerRML(Rml::GetVersion());

	for (auto* id : {"event-log-button", "debug-info-button", "outlines-button", "data-models-button"})
	{
		Element* button = menu_element->GetElementById(id);
		button->AddEventListener(EventId::Click, this);
	}

	return true;
}

bool DebuggerPlugin::LoadInfoElement()
{
	info_element_instancer = MakeUnique<ElementInstancerGeneric<ElementInfo>>();
	Factory::RegisterElementInstancer("debug-info", info_element_instancer.get());
	info_element = rmlui_dynamic_cast<ElementInfo*>(host_context->CreateDocument("debug-info"));
	if (!info_element)
		return false;

	info_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));

	if (!info_element->Initialise())
	{
		host_context->UnloadDocument(info_element);
		info_element = nullptr;
		return false;
	}

	info_element->AddEventListener(EventId::Hide, this);
	info_element->AddEventListener(EventId::Show, this);

	return true;
}

bool DebuggerPlugin::LoadLogElement()
{
	log_element_instancer = MakeUnique<ElementInstancerGeneric<ElementLog>>();
	Factory::RegisterElementInstancer("debug-log", log_element_instancer.get());
	log_element = rmlui_dynamic_cast<ElementLog*>(host_context->CreateDocument("debug-log"));
	if (!log_element)
		return false;

	log_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));

	if (!log_element->Initialise())
	{
		host_context->UnloadDocument(log_element);
		log_element = nullptr;
		return false;
	}

	log_element->AddEventListener(EventId::Hide, this);
	log_element->AddEventListener(EventId::Show, this);

	// Make the system interface; this will trap the log messages for us.
	application_interface = Rml::GetSystemInterface();
	log_interface = MakeUnique<DebuggerSystemInterface>(application_interface, log_element);
	Rml::SetSystemInterface(log_interface.get());

	return true;
}

bool DebuggerPlugin::LoadDataExplorerElement()
{
	data_explorer_element_instancer = MakeUnique<ElementInstancerGeneric<ElementDataModels>>();
	Factory::RegisterElementInstancer("debug-data-models", data_explorer_element_instancer.get());
	data_explorer_element = rmlui_dynamic_cast<ElementDataModels*>(host_context->CreateDocument("debug-data-models"));
	if (!data_explorer_element)
		return false;

	data_explorer_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));

	if (!data_explorer_element->Initialise(debug_context))
	{
		host_context->UnloadDocument(data_explorer_element);
		data_explorer_element = nullptr;
		return false;
	}

	data_explorer_element->AddEventListener(EventId::Hide, this);
	data_explorer_element->AddEventListener(EventId::Show, this);

	return true;
}

void DebuggerPlugin::SetupInfoListeners(Rml::Context* new_context)
{
	RMLUI_ASSERT(info_element);

	if (debug_context)
	{
		debug_context->RemoveEventListener("click", info_element, true);
		debug_context->RemoveEventListener("mouseover", info_element, true);
		debug_context->RemoveEventListener("mouseout", info_element, true);
	}

	if (new_context)
	{
		new_context->AddEventListener("click", info_element, true);
		new_context->AddEventListener("mouseover", info_element, true);
		new_context->AddEventListener("mouseout", info_element, true);
	}
}

void DebuggerPlugin::ReleaseElements()
{
	// Erase event listeners to prevent crashes.
	if (info_element)
		SetupInfoListeners(nullptr);

	if (host_context)
	{
		if (menu_element)
		{
			host_context->UnloadDocument(menu_element);
			menu_element = nullptr;
		}

		if (info_element)
		{
			host_context->UnloadDocument(info_element);
			info_element = nullptr;
		}

		if (log_element)
		{
			host_context->UnloadDocument(log_element);
			log_element = nullptr;
			Rml::SetSystemInterface(application_interface);
			application_interface = nullptr;
			log_interface.reset();
		}
		if (data_explorer_element)
		{
			host_context->UnloadDocument(data_explorer_element);
			data_explorer_element = nullptr;
		}

		// Update to release documents before the plugin gets deleted.
		// Helps avoid cleanup crashes.
		host_context->Update();
	}

	if (debug_context)
	{
		if (hook_element)
		{
			debug_context->UnloadDocument(hook_element);
			hook_element = nullptr;
		}

		// Update to release documents before the plugin gets deleted.
		// Helps avoid cleanup crashes.
		debug_context->Update();
	}
}

} // namespace Debugger
} // namespace Rml
