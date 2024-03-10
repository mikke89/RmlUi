#[[
	Set compiler options and features that are common to all RmlUi targets.
	Arguments:
		- target: The name of the target to set
]]
function(set_common_target_options target)
	target_compile_features(${target} PUBLIC cxx_std_14)
	set_target_properties(${target} PROPERTIES C_EXTENSIONS OFF CXX_EXTENSIONS OFF)

	if(RMLUI_COMPILER_OPTIONS)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
			target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
		elseif(MSVC)
			target_compile_options(${target} PRIVATE /W4 /w44062 /permissive-)
			target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
			if(CMAKE_GENERATOR MATCHES "Visual Studio")
				target_compile_options(${target} PRIVATE /MP)
			endif()
		endif()
	endif()

	if(RMLUI_WARNINGS_AS_ERRORS)
		if(NOT RMLUI_COMPILER_OPTIONS)
			message(FATAL_ERROR "Option RMLUI_WARNINGS_AS_ERRORS requires RMLUI_COMPILER_OPTIONS=ON.")
		endif()

		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
			target_compile_options(${target} PRIVATE -Werror)
		elseif(MSVC)
			target_compile_options(${target} PRIVATE /WX)
		else()
			message(FATAL_ERROR "Unknown compiler, cannot enable option RMLUI_WARNINGS_AS_ERRORS.")
		endif()
	endif()
endfunction()
