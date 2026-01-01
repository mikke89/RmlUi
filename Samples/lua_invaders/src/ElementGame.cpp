#include "ElementGame.h"
#include "Defender.h"
#include "Game.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>

ElementGame::ElementGame(const Rml::String& tag) : Rml::Element(tag)
{
	game = new Game();
}

ElementGame::~ElementGame()
{
	delete game;
}

void ElementGame::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Keydown || event == Rml::EventId::Keyup)
	{
		bool key_down = (event == Rml::EventId::Keydown);
		Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);

		// Process left and right keys
		if (key_down)
		{
			if (key_identifier == Rml::Input::KI_LEFT)
				game->GetDefender()->StartMove(-1.0f);
			if (key_identifier == Rml::Input::KI_RIGHT)
				game->GetDefender()->StartMove(1.0f);
			if (key_identifier == Rml::Input::KI_SPACE)
				game->GetDefender()->Fire();
		}
		else
		{
			if (key_identifier == Rml::Input::KI_LEFT)
				game->GetDefender()->StopMove(-1.0f);
			if (key_identifier == Rml::Input::KI_RIGHT)
				game->GetDefender()->StopMove(1.0f);
		}
	}

	if (event == Rml::EventId::Load)
	{
		game->Initialise();
	}
}

void ElementGame::OnUpdate()
{
	game->Update(Rml::GetSystemInterface()->GetElapsedTime());

	if (game->IsGameOver())
		DispatchEvent("gameover", Rml::Dictionary());
}

void ElementGame::OnRender()
{
	if (Rml::Context* context = GetContext())
		game->Render(context->GetRenderManager(), context->GetDensityIndependentPixelRatio());
}

void ElementGame::OnChildAdd(Rml::Element* element)
{
	Rml::Element::OnChildAdd(element);

	if (element == this)
	{
		GetOwnerDocument()->AddEventListener(Rml::EventId::Load, this);
		GetOwnerDocument()->AddEventListener(Rml::EventId::Keydown, this);
		GetOwnerDocument()->AddEventListener(Rml::EventId::Keyup, this);
	}
}

void ElementGame::OnChildRemove(Rml::Element* element)
{
	Rml::Element::OnChildRemove(element);

	if (element == this)
	{
		GetOwnerDocument()->RemoveEventListener(Rml::EventId::Load, this);
		GetOwnerDocument()->RemoveEventListener(Rml::EventId::Keydown, this);
		GetOwnerDocument()->RemoveEventListener(Rml::EventId::Keyup, this);
	}
}
