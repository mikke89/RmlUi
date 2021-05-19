#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

#include <doctest.h>

class FakeEventListener : public Rml::EventListener
{
public:
	virtual void OnDetach(Rml::Element*) override { delete this; }
	virtual void ProcessEvent(Rml::Event& event) override { event.StopPropagation(); }
};

class FakeEventListenerInstancer : public Rml::EventListenerInstancer
{
public:
	const Rml::String& GetLastInstancedValue()
	{
		if (!last_instanced_value)
			DOCTEST_FAIL("No event listeners have been instanced so far!");
		
		return *last_instanced_value;
	}

	virtual Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element*) override
	{
		last_instanced_value = std::make_unique<Rml::String>(value);
		return new FakeEventListener();
	}

private:
	Rml::UniquePtr<Rml::String> last_instanced_value;
};
