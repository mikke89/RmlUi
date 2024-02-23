set(SAMPLE_NAME "luainvaders")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
	src/DecoratorDefender.cpp
	src/DecoratorDefender.h
	src/DecoratorInstancerDefender.cpp
	src/DecoratorInstancerDefender.h
	src/DecoratorInstancerStarfield.cpp
	src/DecoratorInstancerStarfield.h
	src/DecoratorStarfield.cpp
	src/DecoratorStarfield.h
	src/Defender.cpp
	src/Defender.h
	src/ElementGame.cpp
	src/ElementGame.h
	src/ElementGameInstancer.cpp
	src/ElementGameInstancer.h
	src/Game.cpp
	src/Game.h
	src/GameDetails.cpp
	src/GameDetails.h
	src/HighScores.cpp
	src/HighScores.h
	src/Invader.cpp
	src/Invader.h
	src/LuaInterface.cpp
	src/LuaInterface.h
	src/main.cpp
	src/Mothership.cpp
	src/Mothership.h
	src/Shield.cpp
	src/Shield.h
	src/Sprite.cpp
	src/Sprite.h
)

target_link_libraries(${TARGET_NAME} PRIVATE
	rmlui_shell
	RmlUi::Lua
	RmlUi::External::Lua
)

install(TARGETS ${TARGET_NAME}
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY data lua
	DESTINATION "samples/luainvaders"
)
