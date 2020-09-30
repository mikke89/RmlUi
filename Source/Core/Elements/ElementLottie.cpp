#include "ElementLottie.h"
#include "../TextureDatabase.h"
#include "../../../Include/RmlUi/Core/URL.h"
#include "../../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/StyleSheet.h"

namespace Rml
{
	ElementLottie::ElementLottie(const String& tag) : Element(tag), geometry(this), dimensions(-1.0f, -1.0f)
	{
	}

	ElementLottie::~ElementLottie(void)
	{
	}

	bool ElementLottie::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
	{
		return false;
	}

	void ElementLottie::OnRender()
	{
	}

	void ElementLottie::OnResize()
	{
	}

	void ElementLottie::OnAttributeChange(const ElementAttributes& changed_attributes)
	{
	}

	void ElementLottie::OnPropertyChange(const PropertyIdSet& changed_properties)
	{
	}

	void ElementLottie::GenerateGeometry()
	{
	}

	bool ElementLottie::LoadTexture()
	{
		return false;
	}

	void ElementLottie::UpdateRect()
	{
	}
}