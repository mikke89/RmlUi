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

#ifndef RMLUI_CORE_H
#define RMLUI_CORE_H

#include "Core/Animation.h"
#include "Core/Box.h"
#include "Core/CallbackTexture.h"
#include "Core/CompiledFilterShader.h"
#include "Core/ComputedValues.h"
#include "Core/Context.h"
#include "Core/ContextInstancer.h"
#include "Core/ConvolutionFilter.h"
#include "Core/Core.h"
#include "Core/DataModelHandle.h"
#include "Core/DataTypeRegister.h"
#include "Core/DataTypes.h"
#include "Core/DataVariable.h"
#include "Core/DecorationTypes.h"
#include "Core/Decorator.h"
#include "Core/EffectSpecification.h"
#include "Core/Element.h"
#include "Core/ElementDocument.h"
#include "Core/ElementInstancer.h"
#include "Core/ElementScroll.h"
#include "Core/ElementText.h"
#include "Core/ElementUtilities.h"
#include "Core/Event.h"
#include "Core/EventInstancer.h"
#include "Core/EventListener.h"
#include "Core/EventListenerInstancer.h"
#include "Core/Factory.h"
#include "Core/FileInterface.h"
#include "Core/Filter.h"
#include "Core/FontEffect.h"
#include "Core/FontEffectInstancer.h"
#include "Core/FontEngineInterface.h"
#include "Core/FontGlyph.h"
#include "Core/Geometry.h"
#include "Core/Header.h"
#include "Core/ID.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/Math.h"
#include "Core/Mesh.h"
#include "Core/MeshUtilities.h"
#include "Core/NumericValue.h"
#include "Core/Plugin.h"
#include "Core/PropertiesIteratorView.h"
#include "Core/Property.h"
#include "Core/PropertyDefinition.h"
#include "Core/PropertyDictionary.h"
#include "Core/PropertyIdSet.h"
#include "Core/PropertyParser.h"
#include "Core/PropertySpecification.h"
#include "Core/RenderInterface.h"
#include "Core/RenderManager.h"
#include "Core/Spritesheet.h"
#include "Core/StringUtilities.h"
#include "Core/StyleSheet.h"
#include "Core/StyleSheetContainer.h"
#include "Core/StyleSheetSpecification.h"
#include "Core/StyleTypes.h"
#include "Core/SystemInterface.h"
#include "Core/TextShapingContext.h"
#include "Core/Texture.h"
#include "Core/Transform.h"
#include "Core/TransformPrimitive.h"
#include "Core/Tween.h"
#include "Core/TypeConverter.h"
#include "Core/Types.h"
#include "Core/UniqueRenderResource.h"
#include "Core/Unit.h"
#include "Core/Vertex.h"
#include "Core/XMLNodeHandler.h"
#include "Core/XMLParser.h"

// Elements
#include "Core/Elements/ElementForm.h"
#include "Core/Elements/ElementFormControl.h"
#include "Core/Elements/ElementFormControlInput.h"
#include "Core/Elements/ElementFormControlSelect.h"
#include "Core/Elements/ElementFormControlTextArea.h"
#include "Core/Elements/ElementProgress.h"
#include "Core/Elements/ElementTabSet.h"

#endif
