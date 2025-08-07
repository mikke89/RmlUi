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

#include "ElementLog.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "BeaconSource.h"
#include "CommonSource.h"
#include "LogSource.h"
#include <limits.h>

namespace Rml {
namespace Debugger {

constexpr int MAX_LOG_MESSAGES = 50;

ElementLog::ElementLog(const String& tag) : ElementDebugDocument(tag)
{
	// Set up the log type buttons.
	log_types[Log::LT_ALWAYS].visible = true;
	log_types[Log::LT_ALWAYS].class_name = "error";
	log_types[Log::LT_ALWAYS].alert_contents = "A";

	log_types[Log::LT_ERROR].visible = true;
	log_types[Log::LT_ERROR].class_name = "error";
	log_types[Log::LT_ERROR].alert_contents = "!";
	log_types[Log::LT_ERROR].button_name = "error_button";

	log_types[Log::LT_ASSERT].visible = true;
	log_types[Log::LT_ASSERT].class_name = "error";
	log_types[Log::LT_ASSERT].alert_contents = "!";

	log_types[Log::LT_WARNING].visible = true;
	log_types[Log::LT_WARNING].class_name = "warning";
	log_types[Log::LT_WARNING].alert_contents = "!";
	log_types[Log::LT_WARNING].button_name = "warning_button";

	log_types[Log::LT_INFO].visible = false;
	log_types[Log::LT_INFO].class_name = "info";
	log_types[Log::LT_INFO].alert_contents = "i";
	log_types[Log::LT_INFO].button_name = "info_button";

	log_types[Log::LT_DEBUG].visible = true;
	log_types[Log::LT_DEBUG].class_name = "debug";
	log_types[Log::LT_DEBUG].alert_contents = "?";
	log_types[Log::LT_DEBUG].button_name = "debug_button";
}

ElementLog::~ElementLog()
{
	RemoveEventListener(EventId::Click, this);

	if (beacon && beacon->GetFirstChild())
		beacon->GetFirstChild()->RemoveEventListener(EventId::Click, this);

	if (beacon && beacon->GetParentNode())
		beacon->GetParentNode()->RemoveChild(beacon);

	if (message_content)
	{
		message_content->RemoveEventListener(EventId::Resize, this);
	}
}

bool ElementLog::Initialise()
{
	SetInnerRML(log_rml);
	SetId("rmlui-debug-log");

	message_content = GetElementById("content");
	if (message_content)
	{
		message_content->AddEventListener(EventId::Resize, this);
	}

	SharedPtr<StyleSheetContainer> style_sheet = Factory::InstanceStyleSheetString(String(common_rcss) + String(log_rcss));
	if (!style_sheet)
		return false;

	SetStyleSheetContainer(std::move(style_sheet));

	AddEventListener(EventId::Click, this);

	// Create the log beacon.
	beacon = GetContext()->CreateDocument("debug-document");
	RMLUI_ASSERT(rmlui_dynamic_cast<ElementDebugDocument*>(beacon));
	if (!beacon)
		return false;

	beacon->SetId("rmlui-debug-log-beacon");
	beacon->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
	beacon->SetInnerRML(beacon_rml);

	Element* button = beacon->GetFirstChild();
	if (button)
		beacon->GetFirstChild()->AddEventListener(EventId::Click, this);

	style_sheet = Factory::InstanceStyleSheetString(String(common_rcss) + String(beacon_rcss));
	if (!style_sheet)
	{
		GetContext()->UnloadDocument(beacon);
		beacon = nullptr;
		return false;
	}

	beacon->SetStyleSheetContainer(style_sheet);

	return true;
}

void ElementLog::AddLogMessage(Log::Type type, const String& message)
{
	if (log_messages.size() >= MAX_LOG_MESSAGES)
	{
		log_messages.pop_front();
	}

	log_messages.push_back(LogMessage{type, StringUtilities::EncodeRml(message)});
	num_new_messages += 1;

	// Force a refresh of the RML.
	dirty_logs = true;
}

void ElementLog::OnUpdate()
{
	ElementDocument::OnUpdate();

	if (beacon && num_new_messages > 0)
	{
		for (int i = Math::Max(0, (int)log_messages.size() - num_new_messages); i < (int)log_messages.size(); i++)
		{
			const LogMessage& log_message = log_messages[i];
			const Log::Type type = log_message.type;

			// If this log type is invisible, and there is a button for this log type, then change its text from
			// "Off" to "Off*" to signal that there are unread logs.
			if (!log_types[type].visible)
			{
				if (!log_types[type].button_name.empty())
				{
					if (Element* button = GetElementById(log_types[type].button_name))
					{
						rmlui_static_cast<ElementText*>(button->GetFirstChild())->SetText("Off*");
					}
				}
			}
			// Trigger the beacon if we're hidden. Override any lower-level log type if it is already visible.
			else if (!IsVisible() && type < current_beacon_level)
			{
				beacon->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));

				current_beacon_level = type;
				Element* beacon_button = beacon->GetFirstChild();
				if (beacon_button)
				{
					beacon_button->SetClassNames(log_types[type].class_name);
					beacon_button->SetInnerRML(log_types[type].alert_contents);
				}
			}
		}
		num_new_messages = 0;
	}

	if (dirty_logs && IsVisible())
	{
		String messages;
		if (message_content)
		{
			for (const LogMessage& log_message : log_messages)
			{
				const int type = log_message.type;
				if (!log_types[type].visible)
					continue;

				messages += CreateString(R"(<div class="log-entry"><div class="icon %s">%s</div><p class="message">)",
					log_types[type].class_name.c_str(), log_types[type].alert_contents.c_str());
				messages += log_message.message;
				messages += "</p></div>";
			}

			if (message_content->HasChildNodes())
			{
				float last_element_top = message_content->GetLastChild()->GetAbsoluteTop();
				auto_scroll = message_content->GetAbsoluteTop() + message_content->GetAbsoluteTop() > last_element_top;
			}
			else
			{
				auto_scroll = true;
			}

			message_content->SetInnerRML(messages);

			dirty_logs = false;
		}
	}
}

void ElementLog::ProcessEvent(Event& event)
{
	if (beacon)
	{
		if (event == EventId::Click)
		{
			if (event.GetTargetElement() == beacon->GetFirstChild())
			{
				if (!IsVisible())
					SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));

				beacon->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
				current_beacon_level = Log::LT_MAX;
			}
			else if (event.GetTargetElement()->GetId() == "close_button")
			{
				SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
			}
			else if (event.GetTargetElement()->GetId() == "clear_button")
			{
				for (int i = 0; i < Log::LT_MAX; i++)
				{
					if (!log_types[i].visible && !log_types[i].button_name.empty())
					{
						if (Element* button = GetElementById(log_types[i].button_name))
						{
							rmlui_static_cast<ElementText*>(button->GetFirstChild())->SetText("Off");
						}
					}
				}
				log_messages.clear();
				dirty_logs = true;
			}
			else
			{
				for (int i = 0; i < Log::LT_MAX; i++)
				{
					if (!log_types[i].button_name.empty() && event.GetTargetElement()->GetId() == log_types[i].button_name)
					{
						log_types[i].visible = !log_types[i].visible;
						rmlui_static_cast<ElementText*>(event.GetTargetElement()->GetFirstChild())->SetText(log_types[i].visible ? "On" : "Off");
						dirty_logs = true;
					}
				}
			}
		}
	}

	if (event == EventId::Resize && auto_scroll)
	{
		if (message_content != nullptr && message_content->HasChildNodes())
			message_content->GetLastChild()->ScrollIntoView();
	}
}

} // namespace Debugger
} // namespace Rml
