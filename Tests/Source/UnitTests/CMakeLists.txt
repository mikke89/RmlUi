set(TARGET_NAME "rmlui_unit_tests")

add_executable(${TARGET_NAME}
	Animation.cpp
	BoxShadow.cpp
	Colour.cpp
	Core.cpp
	CustomProperties.cpp
	DataBinding.cpp
	DataExpression.cpp
	DataModel.cpp
	Debugger.cpp
	Decorator.cpp
	Element.cpp
	ElementBackgroundBorder.cpp
	ElementDocument.cpp
	ElementHandle.cpp
	ElementFormControlInput.cpp
	ElementFormControlSelect.cpp
	ElementImage.cpp
	ElementStyle.cpp
	EventListener.cpp
	Filter.cpp
	FlexFormatting.cpp
	Layout.cpp
	Localization.cpp
	main.cpp
	Math.cpp
	MediaQuery.cpp
	Navigation.cpp
	Properties.cpp
	PropertySpecification.cpp
	Selectors.cpp
	Specificity_Basic.cpp
	Specificity_MediaQuery.cpp
	StableVector.cpp
	StringUtilities.cpp
	StyleSheetParser.cpp
	Template.cpp
	URL.cpp
	Variant.cpp
	XMLParser.cpp
	PluginDataModel.cpp
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE
	rmlui_tests_common
	rmlui_core
	doctest::doctest
	trompeloeil::trompeloeil
)

if(RMLUI_LUA_BINDINGS)
	target_sources(${TARGET_NAME} PRIVATE Lua.cpp)
	target_link_libraries(${TARGET_NAME} PRIVATE rmlui_lua)
endif()

if(NOT EMSCRIPTEN)
	doctest_discover_tests(${TARGET_NAME})
endif()
