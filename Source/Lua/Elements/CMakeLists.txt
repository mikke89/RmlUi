# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Using absolute paths to prevent improper interpretation of relative paths. Relative paths can be used once the minimum
# CMake version is greater or equal than CMake 3.13
target_sources(rmlui_lua PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/As.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementForm.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementForm.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControl.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControl.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlInput.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlInput.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlSelect.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlSelect.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlTextArea.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementFormControlTextArea.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementTabSet.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementTabSet.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/SelectOptionsProxy.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SelectOptionsProxy.h"
)
