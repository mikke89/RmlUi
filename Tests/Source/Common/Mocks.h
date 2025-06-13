#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>
#include <RmlUi/Core/RenderInterface.h>
#include <doctest.h>
#include <doctest/trompeloeil.hpp>

class MockEventListener : public trompeloeil::mock_interface<Rml::EventListener> {
public:
	IMPLEMENT_MOCK1(OnAttach);
	IMPLEMENT_MOCK1(OnDetach);
	IMPLEMENT_MOCK1(ProcessEvent);
};

class MockEventListenerInstancer : public trompeloeil::mock_interface<Rml::EventListenerInstancer> {
public:
	IMPLEMENT_MOCK2(InstanceEventListener);
};

class MockRenderInterface : public trompeloeil::mock_interface<Rml::RenderInterface> {
public:
	IMPLEMENT_MOCK2(CompileGeometry);
	IMPLEMENT_MOCK3(RenderGeometry);
	IMPLEMENT_MOCK1(ReleaseGeometry);

	IMPLEMENT_MOCK2(LoadTexture);
	IMPLEMENT_MOCK2(GenerateTexture);
	IMPLEMENT_MOCK1(ReleaseTexture);

	IMPLEMENT_MOCK1(EnableScissorRegion);
	IMPLEMENT_MOCK1(SetScissorRegion);

	IMPLEMENT_MOCK2(CompileShader);
	IMPLEMENT_MOCK4(RenderShader);
	IMPLEMENT_MOCK1(ReleaseShader);
};
