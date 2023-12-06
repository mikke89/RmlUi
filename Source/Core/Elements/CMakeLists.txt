# Declare library
#
# Although not necessary, header files are also included to improve IntelliSense
# capabilities on IDEs and language servers
add_library(rmlui_core_elements INTERFACE)

# Set C++14 as requirement
target_compile_features(rmlui_core_elements INTERFACE "cxx_std_14")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Using absolute paths to prevent improper interpretation of relative paths
# Relative paths can be used once the minimum CMake version is greater or
# equal than CMake 3.13
target_sources(rmlui_core_elements INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlSelect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlTextArea.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementForm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementImage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementImage.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementLabel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementLabel.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementProgress.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementTabSet.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementTextSelection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementTextSelection.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeButton.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeButton.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeCheckbox.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeCheckbox.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputType.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputType.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeRadio.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeRadio.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeRange.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeRange.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeSubmit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeSubmit.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeText.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InputTypeText.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetDropDown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetDropDown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetSlider.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetSlider.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInput.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInputMultiLine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInputMultiLine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInputSingleLine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInputSingleLine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInputSingleLinePassword.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WidgetTextInputSingleLinePassword.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerSelect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerSelect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerTabSet.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerTabSet.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerTextArea.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerTextArea.h"
)
