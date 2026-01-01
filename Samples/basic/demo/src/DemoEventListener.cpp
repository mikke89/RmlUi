#include "DemoEventListener.h"
#include "DemoWindow.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi_Backend.h>

DemoEventListener::DemoEventListener(const Rml::String& value, Rml::Element* element, DemoWindow* demo_window) :
	value(value), element(element), demo_window(demo_window)
{}

void DemoEventListener::ProcessEvent(Rml::Event& event)
{
	using namespace Rml;

	if (value == "change_color")
	{
		const TweeningParameters tweening_parameters = demo_window->GetTweeningParameters();
		const Colourb color((byte)Math::RandomInteger(255), (byte)Math::RandomInteger(255), (byte)Math::RandomInteger(255));

		element->Animate("image-color", Property(color, Unit::COLOUR), tweening_parameters.duration,
			Tween(tweening_parameters.type, tweening_parameters.direction));

		event.StopPropagation();
	}
	else if (value == "move_child")
	{
		const Vector2f mouse_pos = {event.GetParameter("mouse_x", 0.0f), event.GetParameter("mouse_y", 0.0f)};
		if (Element* child = element->GetFirstChild())
		{
			Vector2f new_pos = mouse_pos - element->GetAbsoluteOffset() - Vector2f(0.35f * child->GetClientWidth(), 0.9f * child->GetClientHeight());
			Property destination = Transform::MakeProperty({Transforms::Translate2D(new_pos.x, new_pos.y)});

			const TweeningParameters tweening_parameters = demo_window->GetTweeningParameters();
			if (tweening_parameters.duration <= 0)
				child->SetProperty(PropertyId::Transform, destination);
			else
				child->Animate("transform", destination, tweening_parameters.duration,
					Tween(tweening_parameters.type, tweening_parameters.direction));
		}
	}
	else if (value == "tween_function")
	{
		static const SmallUnorderedMap<String, Tween::Type> tweening_functions = {
			{"back", Tween::Back},
			{"bounce", Tween::Bounce},
			{"circular", Tween::Circular},
			{"cubic", Tween::Cubic},
			{"elastic", Tween::Elastic},
			{"exponential", Tween::Exponential},
			{"linear", Tween::Linear},
			{"quadratic", Tween::Quadratic},
			{"quartic", Tween::Quartic},
			{"quintic", Tween::Quintic},
			{"sine", Tween::Sine},
		};

		const String value = event.GetParameter("value", String());
		auto it = tweening_functions.find(value);
		if (it != tweening_functions.end())
		{
			TweeningParameters tweening_parameters = demo_window->GetTweeningParameters();
			tweening_parameters.type = it->second;
			demo_window->SetTweeningParameters(tweening_parameters);
		}
		else
		{
			RMLUI_ERROR;
		}
	}
	else if (value == "tween_direction")
	{
		const String value = event.GetParameter("value", String());
		TweeningParameters tweening_parameters = demo_window->GetTweeningParameters();
		if (value == "in")
			tweening_parameters.direction = Tween::In;
		else if (value == "out")
			tweening_parameters.direction = Tween::Out;
		else if (value == "in-out")
			tweening_parameters.direction = Tween::InOut;
		else
		{
			RMLUI_ERROR;
		}
		demo_window->SetTweeningParameters(tweening_parameters);
	}
	else if (value == "tween_duration")
	{
		const float value = (float)std::atof(rmlui_static_cast<Rml::ElementFormControl*>(element)->GetValue().c_str());

		TweeningParameters tweening_parameters = demo_window->GetTweeningParameters();
		tweening_parameters.duration = value;
		demo_window->SetTweeningParameters(tweening_parameters);

		if (auto el_duration = element->GetElementById("duration"))
			el_duration->SetInnerRML(CreateString("%2.2f", value));
	}
	else if (value == "rating")
	{
		auto el_rating = element->GetElementById("rating");
		auto el_rating_emoji = element->GetElementById("rating_emoji");
		if (el_rating && el_rating_emoji)
		{
			enum { Sad, Mediocre, Exciting, Celebrate, Champion, CountEmojis };
			static const char* emojis[CountEmojis] = {"😢", "😐", "😮", "😎", "🏆"};
			int value = event.GetParameter("value", 50);

			Rml::String emoji;
			if (value <= 0)
				emoji = emojis[Sad];
			else if (value < 50)
				emoji = emojis[Mediocre];
			else if (value < 75)
				emoji = emojis[Exciting];
			else if (value < 100)
				emoji = emojis[Celebrate];
			else
				emoji = emojis[Champion];

			el_rating->SetInnerRML(Rml::CreateString("%d%%", value));
			el_rating_emoji->SetInnerRML(emoji);
		}
	}
	else if (value == "submit_form")
	{
		const auto& p = event.GetParameters();
		Rml::String output = "<p>";
		for (auto& entry : p)
		{
			auto value = Rml::StringUtilities::EncodeRml(entry.second.Get<Rml::String>());
			if (entry.first == "message")
				value = "<br/>" + value;
			output += "<strong>" + entry.first + "</strong>: " + value + "<br/>";
		}
		output += "</p>";

		demo_window->SubmitForm(output);
	}
	else if (value == "set_sandbox_body")
	{
		if (auto source = rmlui_dynamic_cast<Rml::ElementFormControl*>(element->GetElementById("sandbox_rml_source")))
		{
			auto value = source->GetValue();
			demo_window->SetSandboxBody(value);
		}
	}
	else if (value == "set_sandbox_style")
	{
		if (auto source = rmlui_dynamic_cast<Rml::ElementFormControl*>(element->GetElementById("sandbox_rcss_source")))
		{
			auto value = source->GetValue();
			demo_window->SetSandboxStylesheet(value);
		}
	}
	else if (value == "cancel_selection_on_escape")
	{
		if (event.GetParameter("key_identifier", 0) == Input::KeyIdentifier ::KI_ESCAPE)
		{
			auto select_control = rmlui_dynamic_cast<Rml::ElementFormControlSelect*>(element);
			if (select_control && select_control->IsSelectBoxVisible())
			{
				select_control->CancelSelectBox();
				event.StopPropagation();
			}
		}
	}
}

void DemoEventListener::OnDetach(Rml::Element* /*element*/)
{
	delete this;
}

DemoEventListenerInstancer::DemoEventListenerInstancer(DemoWindow* demo_window) : demo_window(demo_window) {}

Rml::EventListener* DemoEventListenerInstancer::InstanceEventListener(const Rml::String& value, Rml::Element* element)
{
	return new DemoEventListener(value, element, demo_window);
}
