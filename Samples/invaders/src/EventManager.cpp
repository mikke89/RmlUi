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

#include "EventManager.h"
#include "EventHandler.h"
#include "GameDetails.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

// The game's element context (declared in main.cpp).
extern Rml::Context* context;

// The event handler for the current screen. This may be nullptr if the current screen has no specific functionality.
static EventHandler* event_handler = nullptr;

// The event handlers registered with the manager.
using EventHandlerMap = Rml::SmallUnorderedMap<Rml::String, Rml::UniquePtr<EventHandler>>;
EventHandlerMap event_handlers;

EventManager::EventManager() {}

EventManager::~EventManager() {}

void EventManager::Shutdown()
{
	event_handlers.clear();
	event_handler = nullptr;
}

void EventManager::RegisterEventHandler(const Rml::String& handler_name, Rml::UniquePtr<EventHandler> handler)
{
	// Any handler bound under the same name will be released.
	event_handlers[handler_name] = std::move(handler);
}

void EventManager::ProcessEvent(Rml::Event& event, const Rml::String& value)
{
	Rml::StringList commands;
	Rml::StringUtilities::ExpandString(commands, value, ';');
	for (size_t i = 0; i < commands.size(); ++i)
	{
		// Check for custom commands.
		Rml::StringList values;
		Rml::StringUtilities::ExpandString(values, commands[i], ' ');

		if (values.empty())
			return;

		if (values[0] == "onescape" && values.size() > 1)
		{
			Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);
			if (key_identifier == Rml::Input::KI_ESCAPE)
				values.erase(values.begin());
		}

		if (values[0] == "goto" && values.size() > 1)
		{
			if (LoadWindow(values[1]))
				event.GetTargetElement()->GetOwnerDocument()->Close();
		}
		else if (values[0] == "load" && values.size() > 1)
		{
			LoadWindow(values[1]);
		}
		else if (values[0] == "close")
		{
			Rml::ElementDocument* target_document = nullptr;

			if (values.size() > 1)
				target_document = context->GetDocument(values[1].c_str());
			else
				target_document = event.GetTargetElement()->GetOwnerDocument();

			if (target_document != nullptr)
				target_document->Close();
		}
		else if (values[0] == "exit")
		{
			Backend::RequestExit();
		}
		else if (values[0] == "pause")
		{
			GameDetails::SetPaused(true);
		}
		else if (values[0] == "unpause")
		{
			GameDetails::SetPaused(false);
		}
		else
		{
			if (event_handler)
				event_handler->ProcessEvent(event, commands[i]);
		}
	}
}

Rml::ElementDocument* EventManager::LoadWindow(const Rml::String& window_name)
{
	// Set the event handler for the new screen, if one has been registered.
	EventHandler* old_event_handler = event_handler;
	auto it = event_handlers.find(window_name);
	if (it != event_handlers.end())
		event_handler = it->second.get();
	else
		event_handler = nullptr;

	// Attempt to load the referenced RML document.
	Rml::String document_path = Rml::String("invaders/data/") + window_name + Rml::String(".rml");
	Rml::ElementDocument* document = context->LoadDocument(document_path.c_str());
	if (!document)
	{
		event_handler = old_event_handler;
		return nullptr;
	}

	// Set the element's title on the title; IDd 'title' in the RML.
	if (Rml::Element* title = document->GetElementById("title"))
		title->SetInnerRML(document->GetTitle());

	document->Show();

	return document;
}
