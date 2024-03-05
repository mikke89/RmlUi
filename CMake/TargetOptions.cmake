#[[
    Set compile options and features that are common to all RmlUi targets.
    Arguments:
        - target: The name of the target to set
]]
function(set_common_target_options target)
	target_compile_features(${target} PUBLIC cxx_std_14)
	set_target_properties(${target} PROPERTIES C_EXTENSIONS OFF CXX_EXTENSIONS OFF)

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)

		if(RMLUI_WARNINGS_AS_ERRORS)
			target_compile_options(${target} PRIVATE -Werror)
		endif()
	elseif(MSVC)
		target_compile_options(${target} PRIVATE /W4 /w44062 /permissive-)
		target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)

		if(CMAKE_GENERATOR MATCHES "Visual Studio")
			target_compile_options(${target} PRIVATE /MP)
		endif()

		if(RMLUI_WARNINGS_AS_ERRORS)
			target_compile_options(${target} PRIVATE /WX)
		endif()
	endif()
endfunction()
