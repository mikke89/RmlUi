add_executable(rmlui_unit_tests
	Animation.cpp
	Core.cpp
	DataBinding.cpp
	DataExpression.cpp
	DataModel.cpp
	Debugger.cpp
	Element.cpp
	ElementDocument.cpp
	ElementFormControlSelect.cpp
	ElementImage.cpp
	ElementStyle.cpp
	GeometryDatabase.cpp
	Layout.cpp
	main.cpp
	Math.cpp
	MediaQuery.cpp
	Properties.cpp
	PropertySpecification.cpp
	Selectors.cpp
	Specificity_Basic.cpp
	Specificity_MediaQuery.cpp
	StringUtilities.cpp
	Template.cpp
	URL.cpp
	Variant.cpp
	XMLParser.cpp
)

target_link_libraries(rmlui_unit_tests PRIVATE
	rmlui_tests_common
	rmlui_core
	doctest::doctest
	trompeloeil::trompeloeil
)

doctest_discover_tests(rmlui_unit_tests)
