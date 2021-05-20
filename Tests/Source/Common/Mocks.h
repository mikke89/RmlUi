#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

#include <doctest.h>
#include <doctest/trompeloeil.hpp>

class MockEventListener : public trompeloeil::mock_interface<Rml::EventListener>
{
public:
	IMPLEMENT_MOCK1(OnAttach);
	IMPLEMENT_MOCK1(OnDetach);
	IMPLEMENT_MOCK1(ProcessEvent);
};

class MockEventListenerInstancer : public trompeloeil::mock_interface<Rml::EventListenerInstancer>
{
public:
	IMPLEMENT_MOCK2(InstanceEventListener);
};
