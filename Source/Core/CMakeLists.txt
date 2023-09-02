# Declare core RmlUi library
# Not explicitly setting library type so that it can be chosen by consumer
# using BUILD_SHARED_LIBS
#
# Although not necessary, header files are also included to improve IntelliSense
# capabilities on IDEs and language servers
#
# Tip: On UNIX systems you can run this on the current source folder to get the
# entire list of files in the folder copied on the files.txt file:
#
#       ls *.cpp *.h > files.txt
#
# After running it, you can copy the text in that file and paste it here as a list
# of source files
#
# Quoting the source file paths isn't necessary because add_library() doesn't read
# CMake variables, just strings
add_library(rmlui_core 
    BaseXMLParser.cpp
    Box.cpp
    Clock.cpp
    Clock.h
    ComputedValues.cpp
    ComputeProperty.cpp
    ComputeProperty.h
    Context.cpp
    ContextInstancer.cpp
    ContextInstancerDefault.cpp
    ContextInstancerDefault.h
    ConvolutionFilter.cpp
    Core.cpp
    DataController.cpp
    DataControllerDefault.cpp
    DataControllerDefault.h
    DataController.h
    DataExpression.cpp
    DataExpression.h
    DataModel.cpp
    DataModel.h
    DataModelHandle.cpp
    DataTypeRegister.cpp
    DataVariable.cpp
    DataView.cpp
    DataViewDefault.cpp
    DataViewDefault.h
    DataView.h
    Decorator.cpp
    DecoratorGradient.cpp
    DecoratorGradient.h
    DecoratorInstancer.cpp
    DecoratorNinePatch.cpp
    DecoratorNinePatch.h
    DecoratorTiledBox.cpp
    DecoratorTiledBox.h
    DecoratorTiledBoxInstancer.cpp
    DecoratorTiledBoxInstancer.h
    DecoratorTiled.cpp
    DecoratorTiled.h
    DecoratorTiledHorizontal.cpp
    DecoratorTiledHorizontal.h
    DecoratorTiledHorizontalInstancer.cpp
    DecoratorTiledHorizontalInstancer.h
    DecoratorTiledImage.cpp
    DecoratorTiledImage.h
    DecoratorTiledImageInstancer.cpp
    DecoratorTiledImageInstancer.h
    DecoratorTiledInstancer.cpp
    DecoratorTiledInstancer.h
    DecoratorTiledVertical.cpp
    DecoratorTiledVertical.h
    DecoratorTiledVerticalInstancer.cpp
    DecoratorTiledVerticalInstancer.h
    DocumentHeader.cpp
    DocumentHeader.h
    ElementAnimation.cpp
    ElementAnimation.h
    ElementBackgroundBorder.cpp
    ElementBackgroundBorder.h
    Element.cpp
    ElementDecoration.cpp
    ElementDecoration.h
    ElementDefinition.cpp
    ElementDefinition.h
    ElementDocument.cpp
    ElementHandle.cpp
    ElementHandle.h
    ElementInstancer.cpp
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
    FontEffectBlur.cpp
    FontEffectBlur.h
    FontEffect.cpp
    FontEffectGlow.cpp
    FontEffectGlow.h
    FontEffectInstancer.cpp
    FontEffectOutline.cpp
    FontEffectOutline.h
    FontEffectShadow.cpp
    FontEffectShadow.h
    FontEngineInterface.cpp
    GeometryBackgroundBorder.cpp
    GeometryBackgroundBorder.h
    Geometry.cpp
    GeometryDatabase.cpp
    GeometryDatabase.h
    GeometryUtilities.cpp
    IdNameMap.h
    Log.cpp
    Math.cpp
    Memory.cpp
    Memory.h
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
    PropertyParserColour.cpp
    PropertyParserColour.h
    PropertyParserDecorator.cpp
    PropertyParserDecorator.h
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
    ScrollController.cpp
    ScrollController.h
    Spritesheet.cpp
    Stream.cpp
    StreamFile.cpp
    StreamFile.h
    StreamMemory.cpp
    StringUtilities.cpp
    StyleSheetContainer.cpp
    StyleSheet.cpp
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
    TemplateCache.cpp
    TemplateCache.h
    Template.cpp
    Template.h
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
    TextureResource.cpp
    TextureResource.h
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
    XMLNodeHandlerBody.cpp
    XMLNodeHandlerBody.h
    XMLNodeHandler.cpp
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

# Set C++14 as requirement
target_compile_features(rmlui_core PUBLIC "cxx_std_14")

# Add public include directories
target_include_directories(rmlui_core PUBLIC "${PROJECT_SOURCE_DIR}/Include")

# Add public headers as files in the project (it's not necessary but convenient for IDE integration)
# Setting them as PRIVATE so that it's addition doesn't propagate, it won't affect availability since
# the entire include directory has already been declared as public
target_sources(rmlui_core PRIVATE
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Config/Config.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Animation.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/BaseXMLParser.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Box.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Colour.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Colour.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ComputedValues.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/itlib/flat_map.hpp
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/itlib/flat_set.hpp
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/LICENSE.txt
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/robin_hood.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Context.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ContextInstancer.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ConvolutionFilter.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Core.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataModelHandle.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataStructHandle.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataTypeRegister.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataTypes.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DataVariable.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Debug.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Decorator.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/DecoratorInstancer.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Dictionary.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Element.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Element.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementDocument.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementInstancer.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Input.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Log.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Math.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Matrix4.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Matrix4.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/NumericValue.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ObserverPtr.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Platform.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Plugin.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Profiling.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertiesIteratorView.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Property.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyDefinition.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyDictionary.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyIdSet.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertyParser.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/PropertySpecification.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Rectangle.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementScroll.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementText.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ElementUtilities.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Event.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/EventInstancer.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/EventListener.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/EventListenerInstancer.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Factory.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FileInterface.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontEffect.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontEffectInstancer.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontEngineInterface.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontGlyph.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/FontMetrics.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Geometry.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/GeometryUtilities.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Header.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ScriptInterface.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ScrollTypes.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Spritesheet.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Stream.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StreamMemory.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StringUtilities.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheet.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheetContainer.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheetSpecification.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleSheetTypes.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/StyleTypes.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/SystemInterface.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Texture.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Traits.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Transform.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TransformPrimitive.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Tween.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TypeConverter.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/TypeConverter.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Types.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Unit.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/URL.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Utilities.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Variant.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Variant.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector2.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector2.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector3.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector3.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector4.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vector4.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Vertex.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/XMLNodeHandler.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/XMLParser.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementForm.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControl.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControlInput.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControlSelect.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementFormControlTextArea.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementProgress.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Elements/ElementTabSet.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/ID.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/RenderInterface.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Core.h
)

# Add alias for the library so that a namespaced name can be used in the rest
# of the project to link against it
add_library(RmlUi::RmlUi ALIAS rmlui_core)

# Set additional target properties
set_target_properties(rmlui_core PROPERTIES
    # Add export name so that it can be exported with a namespaced name instead
    # of using the name we actually used to declare the target
    EXPORT_NAME "RmlUi::RmlUi"

    # Change output name of the final library file
    OUTPUT_NAME "rmlui"
)

# Add the RmlUi element sources
add_subdirectory("Elements")
target_link_libraries(rmlui_core PRIVATE rmlui_core_elements)

# Add the RmlUi layout sources
add_subdirectory("Layout")
target_link_libraries(rmlui_core PRIVATE rmlui_core_layout)

# Avoid problems with MSVC's __dllspec() in static builds
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

# Negotiate usage of the default font engine
# When any new font engine is added, it must be added here with an else if statement before the else case
if(RMLUI_FONT_INTERFACE STREQUAL "freetype")
    # Link against the default font engine
    # Adding the subdirectory inside the conditional block so that compilation doesn't
    # fail when CMake tries to build all targets, effectively hiding the CMake target from
    # the build system if the default font interface is disabled
    add_subdirectory("FontEngineDefault")
    target_link_libraries(rmlui_core PRIVATE rmlui_core_fontenginedefault)
else()
    # Pass information to code via compile definition
    target_compile_definitions(rmlui_core PRIVATE "RMLUI_NO_FONT_INTERFACE_DEFAULT")
endif()

# Negotiate usage of the Lottie plugin
if(RMLUI_ENABLE_LOTTIE_PLUGIN)
    # Link against the Lottie plugin
    target_link_libraries(rmlui_core PRIVATE rmlui_lottie)
endif()
