set(SAMPLE_NAME "invaders")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
	src/DecoratorDefender.cpp
	src/DecoratorDefender.h
	src/DecoratorStarfield.cpp
	src/DecoratorStarfield.h
	src/Defender.cpp
	src/Defender.h
	src/ElementGame.cpp
	src/ElementGame.h
	src/EventHandler.cpp
	src/EventHandler.h
	src/EventHandlerHighScore.cpp
	src/EventHandlerHighScore.h
	src/EventHandlerOptions.cpp
	src/EventHandlerOptions.h
	src/EventHandlerStartGame.cpp
	src/EventHandlerStartGame.h
	src/EventListener.cpp
	src/EventListener.h
	src/EventListenerInstancer.cpp
	src/EventListenerInstancer.h
	src/EventManager.cpp
	src/EventManager.h
	src/Game.cpp
	src/Game.h
	src/GameDetails.cpp
	src/GameDetails.h
	src/HighScores.cpp
	src/HighScores.h
	src/Invader.cpp
	src/Invader.h
	src/main.cpp
	src/Mothership.cpp
	src/Mothership.h
	src/Shield.cpp
	src/Shield.h
	src/Sprite.cpp
	src/Sprite.h
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install_sample_target(${TARGET_NAME})
