# Declare RmlUi core library
# Not explicitly setting library type so that it can be chosen by consumer using BUILD_SHARED_LIBS. Header files are not
# necessary, but are included to improve navigation and code completion on IDEs and language servers.
add_library(rmlui_core
	BaseXMLParser.cpp
	Box.cpp
	BoxShadowCache.h
	BoxShadowCache.cpp
	BoxShadowHash.h
	CallbackTexture.cpp
	Clock.cpp
	Clock.h
	CompiledFilterShader.cpp
	ComputedValues.cpp
	ComputeProperty.cpp
	ComputeProperty.h
	Context.cpp
	ContextInstancer.cpp
	ContextInstancerDefault.cpp
	ContextInstancerDefault.h
	ControlledLifetimeResource.h
	ConvolutionFilter.cpp
	Core.cpp
	DataController.cpp
	DataController.h
	DataControllerDefault.cpp
	DataControllerDefault.h
	DataExpression.cpp
	DataExpression.h
	DataModel.cpp
	DataModel.h
	DataModelHandle.cpp
	DataTypeRegister.cpp
	DataVariable.cpp
	DataView.cpp
	DataView.h
	DataViewDefault.cpp
	DataViewDefault.h
	Decorator.cpp
	DecoratorGradient.cpp
	DecoratorGradient.h
	DecoratorNinePatch.cpp
	DecoratorNinePatch.h
	DecoratorShader.cpp
	DecoratorShader.h
	DecoratorText.cpp
	DecoratorText.h
	DecoratorTiled.cpp
	DecoratorTiled.h
	DecoratorTiledBox.cpp
	DecoratorTiledBox.h
	DecoratorTiledHorizontal.cpp
	DecoratorTiledHorizontal.h
	DecoratorTiledImage.cpp
	DecoratorTiledImage.h
	DecoratorTiledVertical.cpp
	DecoratorTiledVertical.h
	DecoratorUtilities.cpp
	DecoratorUtilities.h
	DocumentHeader.cpp
	DocumentHeader.h
	EffectSpecification.cpp
	Element.cpp
	ElementAnimation.cpp
	ElementAnimation.h
	ElementBackgroundBorder.cpp
	ElementBackgroundBorder.h
	ElementDefinition.cpp
	ElementDefinition.h
	ElementDocument.cpp
	ElementEffects.cpp
	ElementEffects.h
	ElementHandle.cpp
	ElementHandle.h
	ElementInstancer.cpp
	ElementMeta.cpp
	ElementMeta.h
	ElementScroll.cpp
	ElementStyle.cpp
	ElementStyle.h
	ElementText.cpp
	ElementUtilities.cpp
	Event.cpp
	EventDispatcher.cpp
	EventDispatcher.h
	EventInstancer.cpp
	EventInstancerDefault.cpp
	EventInstancerDefault.h
	EventListenerInstancer.cpp
	EventSpecification.cpp
	EventSpecification.h
	Factory.cpp
	FileInterface.cpp
	FileInterfaceDefault.cpp
	FileInterfaceDefault.h
	Filter.cpp
	FilterBasic.cpp
	FilterBasic.h
	FilterBlur.cpp
	FilterBlur.h
	FilterDropShadow.cpp
	FilterDropShadow.h
	FontEffect.cpp
	FontEffectBlur.cpp
	FontEffectBlur.h
	FontEffectGlow.cpp
	FontEffectGlow.h
	FontEffectInstancer.cpp
	FontEffectOutline.cpp
	FontEffectOutline.h
	FontEffectShadow.cpp
	FontEffectShadow.h
	FontEngineInterface.cpp
	Geometry.cpp
	GeometryBackgroundBorder.cpp
	GeometryBackgroundBorder.h
	GeometryBoxShadow.cpp
	GeometryBoxShadow.h
	IdNameMap.h
	Log.cpp
	LogDefault.cpp
	LogDefault.h
	Math.cpp
	Memory.cpp
	Memory.h
	MeshUtilities.cpp
	ObserverPtr.cpp
	Plugin.cpp
	PluginRegistry.cpp
	PluginRegistry.h
	Pool.h
	precompiled.h
	Profiling.cpp
	PropertiesIterator.h
	PropertiesIteratorView.cpp
	Property.cpp
	PropertyDefinition.cpp
	PropertyDictionary.cpp
	PropertyParserAnimation.cpp
	PropertyParserAnimation.h
	PropertyParserBoxShadow.cpp
	PropertyParserBoxShadow.h
	PropertyParserColorStopList.cpp
	PropertyParserColorStopList.h
	PropertyParserColour.cpp
	PropertyParserColour.h
	PropertyParserDecorator.cpp
	PropertyParserDecorator.h
	PropertyParserFilter.cpp
	PropertyParserFilter.h
	PropertyParserFontEffect.cpp
	PropertyParserFontEffect.h
	PropertyParserKeyword.cpp
	PropertyParserKeyword.h
	PropertyParserNumber.cpp
	PropertyParserNumber.h
	PropertyParserRatio.cpp
	PropertyParserRatio.h
	PropertyParserString.cpp
	PropertyParserString.h
	PropertyParserTransform.cpp
	PropertyParserTransform.h
	PropertyShorthandDefinition.h
	PropertySpecification.cpp
	RenderInterface.cpp
	RenderInterfaceCompatibility.cpp
	RenderManager.cpp
	RenderManagerAccess.cpp
	RenderManagerAccess.h
	ScrollController.cpp
	ScrollController.h
	Spritesheet.cpp
	Stream.cpp
	StreamFile.cpp
	StreamFile.h
	StreamMemory.cpp
	StringUtilities.cpp
	StyleSheet.cpp
	StyleSheetContainer.cpp
	StyleSheetFactory.cpp
	StyleSheetFactory.h
	StyleSheetNode.cpp
	StyleSheetNode.h
	StyleSheetParser.cpp
	StyleSheetParser.h
	StyleSheetSelector.cpp
	StyleSheetSelector.h
	StyleSheetSpecification.cpp
	SystemInterface.cpp
	Template.cpp
	Template.h
	TemplateCache.cpp
	TemplateCache.h
	Texture.cpp
	TextureDatabase.cpp
	TextureDatabase.h
	TextureLayout.cpp
	TextureLayout.h
	TextureLayoutRectangle.cpp
	TextureLayoutRectangle.h
	TextureLayoutRow.cpp
	TextureLayoutRow.h
	TextureLayoutTexture.cpp
	TextureLayoutTexture.h
	Traits.cpp
	Transform.cpp
	TransformPrimitive.cpp
	TransformState.cpp
	TransformState.h
	TransformUtilities.cpp
	TransformUtilities.h
	Tween.cpp
	TypeConverter.cpp
	URL.cpp
	Variant.cpp
	WidgetScroll.cpp
	WidgetScroll.h
	XMLNodeHandler.cpp
	XMLNodeHandlerBody.cpp
	XMLNodeHandlerBody.h
	XMLNodeHandlerDefault.cpp
	XMLNodeHandlerDefault.h
	XMLNodeHandlerHead.cpp
	XMLNodeHandlerHead.h
	XMLNodeHandlerTemplate.cpp
	XMLNodeHandlerTemplate.h
	XMLParser.cpp
	XMLParseTools.cpp
	XMLParseTools.h
)

# Add public headers as files in the project (it's not necessary but convenient for IDE integration)
# Setting them as PRIVATE so that it's addition doesn't propagate, it won't affect availability since
# the entire include directory has already been declared as public
target_sources(rmlui_core PRIVATE
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Config/Config.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/BaseXMLParser.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Box.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/CallbackTexture.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Colour.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Colour.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/CompiledFilterShader.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ComputedValues.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/itlib/flat_map.hpp"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/itlib/flat_set.hpp"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/robin_hood.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Context.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ContextInstancer.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ConvolutionFilter.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Core.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataModelHandle.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataStructHandle.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataTypeRegister.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataTypes.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataVariable.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Debug.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DecorationTypes.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Decorator.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Dictionary.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/EffectSpecification.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Element.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Element.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementDocument.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementInstancer.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementScroll.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementForm.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControl.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControlInput.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControlSelect.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControlTextArea.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementProgress.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementTabSet.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementText.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementUtilities.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Event.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/EventInstancer.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/EventListener.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/EventListenerInstancer.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Factory.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FileInterface.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Filter.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontEffect.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontEffectInstancer.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontEngineInterface.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontGlyph.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontMetrics.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Geometry.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Header.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ID.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Input.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Log.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Math.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Matrix4.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Matrix4.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Mesh.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/MeshUtilities.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/NumericValue.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ObserverPtr.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Platform.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Plugin.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Profiling.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertiesIteratorView.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Property.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyDefinition.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyDictionary.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyIdSet.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyParser.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertySpecification.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Rectangle.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/RenderBox.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/RenderInterface.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/RenderInterfaceCompatibility.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/RenderManager.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ScriptInterface.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ScrollTypes.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Span.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Spritesheet.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StableVector.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Stream.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StreamMemory.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StringUtilities.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheet.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheetContainer.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheetSpecification.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheetTypes.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleTypes.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/SystemInterface.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TextInputContext.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TextInputHandler.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TextShapingContext.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Texture.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Traits.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Transform.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TransformPrimitive.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Tween.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TypeConverter.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TypeConverter.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Types.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/UniqueRenderResource.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Unit.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/URL.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Utilities.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Variant.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Variant.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector2.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector2.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector3.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector3.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector4.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector4.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vertex.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/XMLNodeHandler.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/XMLParser.h"
)

set_common_target_options(rmlui_core)

target_include_directories(rmlui_core PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_include_directories(rmlui_core INTERFACE
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_library(RmlUi::Core ALIAS rmlui_core)

set_target_properties(rmlui_core PROPERTIES
	# Add export name so that it can be exported with a namespaced name instead
	# of using the name we actually used to declare the target
	EXPORT_NAME "Core"

	# Change output name of the final library file
	OUTPUT_NAME "rmlui"
)

generate_rmlui_version_string()
target_compile_definitions(rmlui_core PRIVATE "RMLUI_VERSION=\"${RMLUI_VERSION_SHORT}\"")

add_subdirectory("Elements")
add_subdirectory("Layout")

# Set up definitions to export functions and classes as appropriate
get_target_property(rmlui_core_TYPE rmlui_core "TYPE")
if(rmlui_core_TYPE STREQUAL "STATIC_LIBRARY")
	# If RmlUi is being compiled as a static library, notify code to disable usage of __dllspec()
	# Since public headers also detect it, it needs to be a public definition
	target_compile_definitions(rmlui_core PUBLIC "RMLUI_STATIC_LIB")
elseif(rmlui_core_TYPE STREQUAL "SHARED_LIBRARY")
	# If RmlUi is being compiled as a shared library, notify code to export functions using __dllspec()
	# For applications consuming the library, the headers will automatically use dllimport
	# Platform and compiler handling is already done by code via pre-processor macros
	target_compile_definitions(rmlui_core PRIVATE "RMLUI_CORE_EXPORTS")
endif()
unset(rmlui_core_TYPE)

if(RMLUI_FONT_ENGINE STREQUAL "freetype")
	# Include the source files for the default font engine.
	add_subdirectory("FontEngineDefault")

	# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
	# From CMake 3.13 the next line can be moved into `FontEngineDefault/CMakeLists.txt`, see CMP0079.
	target_link_libraries(rmlui_core PRIVATE Freetype::Freetype)
endif()

if(RMLUI_LOTTIE_PLUGIN)
	# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
	# From CMake 3.13 we could move this to `Lottie/CMakeLists.txt`, see CMP0079.
	target_link_libraries(rmlui_core PRIVATE rlottie::rlottie)
endif()

if(RMLUI_SVG_PLUGIN)
	# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
	# From CMake 3.13 we could move this to `Lottie/CMakeLists.txt`, see CMP0079.
	target_link_libraries(rmlui_core PRIVATE lunasvg::lunasvg)
endif()

if(RMLUI_TRACY_PROFILING)
	if(CMAKE_CONFIGURATION_TYPES AND RMLUI_TRACY_CONFIGURATION)
		target_link_libraries(rmlui_core PUBLIC "$<$<CONFIG:Tracy>:Tracy::TracyClient>")
		target_compile_definitions(rmlui_core PUBLIC "$<$<CONFIG:Tracy>:RMLUI_TRACY_PROFILING>")
		if(RMLUI_TRACY_MEMORY_PROFILING)
			target_compile_definitions(rmlui_core PRIVATE "$<$<CONFIG:Tracy>:RMLUI_TRACY_MEMORY_PROFILING>")
		endif()
		message(STATUS "Tracy profiling enabled in configuration `Tracy`.")
	else()
		target_link_libraries(rmlui_core PUBLIC Tracy::TracyClient)
		target_compile_definitions(rmlui_core PUBLIC "RMLUI_TRACY_PROFILING")
		if(RMLUI_TRACY_MEMORY_PROFILING)
			target_compile_definitions(rmlui_core PRIVATE "RMLUI_TRACY_MEMORY_PROFILING")
		endif()
		message(STATUS "Tracy profiling enabled.")
	endif()
endif()

if(NOT RMLUI_THIRDPARTY_CONTAINERS)
	target_compile_definitions(rmlui_core PUBLIC "RMLUI_NO_THIRDPARTY_CONTAINERS")
	message(STATUS "Disabling third-party containers for RmlUi.")
endif()

if(RMLUI_CUSTOM_RTTI)
	target_compile_definitions(rmlui_core PUBLIC "RMLUI_CUSTOM_RTTI")
	message(STATUS "Enabling custom RTTI for RmlUi.")
endif()

if(RMLUI_MATRIX_ROW_MAJOR)
	target_compile_definitions(rmlui_core PUBLIC "RMLUI_MATRIX_ROW_MAJOR")
	message(STATUS "Configuring RmlUi to use row-major matrix types.")
endif()

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# From CMake 3.16 we can skip the version check.
if(RMLUI_PRECOMPILED_HEADERS AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
	target_precompile_headers(rmlui_core PRIVATE "${PROJECT_SOURCE_DIR}/Source/Core/precompiled.h")
elseif(RMLUI_PRECOMPILED_HEADERS)
	message(STATUS "Could not enable precompiled headers, requires CMake version 3.16 or greater.")
endif()

if(RMLUI_CUSTOM_CONFIGURATION AND RMLUI_CUSTOM_CONFIGURATION_FILE)
	target_compile_definitions(rmlui_core PUBLIC "RMLUI_CUSTOM_CONFIGURATION_FILE=\"${RMLUI_CUSTOM_CONFIGURATION_FILE}\"")
	message(STATUS "Including ${RMLUI_CUSTOM_CONFIGURATION_FILE} instead of <RmlUi/Config/Config.h>")
endif()
if(RMLUI_CUSTOM_CONFIGURATION AND RMLUI_CUSTOM_INCLUDE_DIRS)
	target_include_directories(rmlui_core PUBLIC ${RMLUI_CUSTOM_INCLUDE_DIRS})
endif()
if(RMLUI_CUSTOM_CONFIGURATION AND RMLUI_CUSTOM_LINK_LIBRARIES)
	target_link_libraries(rmlui_core PUBLIC ${RMLUI_CUSTOM_LINK_LIBRARIES})
endif()

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# We use default paths provided from GNUInstallDirs. From CMake 3.14 these paths (CMAKE_INSTALL_...) will be used
# automatically and can be removed from the following call. The same applies to many other calls to install(TARGETS...).
# Note that GNUInstallDirs should still be included in the project root.
install(TARGETS rmlui_core
	EXPORT RmlUiTargets
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install_target_pdb(rmlui_core)
