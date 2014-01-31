#include "LuaInterface.h"
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include "Game.h"
#include "GameDetails.h"
#include <Rocket/Core/Log.h>
#include <Shell.h>
#include <Rocket/Core/Factory.h>
#include "HighScores.h"
#include <Rocket/Core/Types.h>
#include "ElementGameInstancer.h"

//we have to create the binding ourselves, and these are the functions that will be
//called. It has to match the function signature of int (*ftnptr)(lua_State*)
int GameShutdown(lua_State* L);
int GameSetPaused(lua_State* L);
int GameSetDifficulty(lua_State* L);
int GameSetDefenderColour(lua_State* L);
int GameSubmitHighScore(lua_State* L);
int GameSetHighScoreName(lua_State* L);

void LuaInterface::Initialise(lua_State* L)
{
    InitGame(L);
    Rocket::Core::Factory::RegisterElementInstancer("game",new ElementGameInstancer())->RemoveReference();
}

void LuaInterface::InitGame(lua_State *L)
{
    luaL_dostring(L,"Game = Game or {}"); //doing this in Lua because it would be 10+ lines in C++
    lua_getglobal(L,"Game");
    int game = lua_gettop(L);

    if(lua_isnil(L,game))
    {
        Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Error creating the Game table from C++ in LuaInterface::InitGame");
        return;
    }
    
    //this can be done in a loop similar to the implementation of LuaType::_regfunctions, but I am
    //defining them explicitly so that there is no black magic
    lua_pushcfunction(L,GameShutdown);
    lua_setfield(L, game, "Shutdown");

    lua_pushcfunction(L,GameSetPaused);
    lua_setfield(L,game,"SetPaused");

    lua_pushcfunction(L,GameSetDifficulty);
    lua_setfield(L,game,"SetDifficulty");

    lua_pushcfunction(L,GameSetDefenderColour);
    lua_setfield(L,game,"SetDefenderColour");

    lua_pushcfunction(L,GameSubmitHighScore);
    lua_setfield(L,game,"SubmitHighScore");

    lua_pushcfunction(L,GameSetHighScoreName);
    lua_setfield(L,game,"SetHighScoreName");

    lua_newtable(L); //table, Game
    lua_pushinteger(L,GameDetails::HARD); //int,table,Game
    lua_setfield(L,-2,"HARD");//table,Game

    lua_pushinteger(L,GameDetails::EASY); //int,table,Game
    lua_setfield(L,-2,"EASY"); //table,Game

    lua_setfield(L,game,"difficulty"); //Game

    lua_pop(L,1); //pop Game
}

int GameShutdown(lua_State* L)
{
    Shell::RequestExit();
    return 0;
}

int GameSetPaused(lua_State* L)
{
    bool paused = CHECK_BOOL(L,1); //CHECK_BOOL defined in LuaType.h
    GameDetails::SetPaused(paused);
    return 0;
}

int GameSetDifficulty(lua_State* L)
{
    int difficulty = luaL_checkint(L,1);
    GameDetails::SetDifficulty((GameDetails::Difficulty)difficulty);
    return 0;
}

int GameSetDefenderColour(lua_State* L)
{
    Rocket::Core::Colourb* colour = Rocket::Core::Lua::LuaType<Rocket::Core::Colourb>::check(L,1);
    GameDetails::SetDefenderColour(*colour);
    return 0;
}

int GameSubmitHighScore(lua_State* L)
{
    int score = GameDetails::GetScore();
    if(score > 0)
    {
        // Submit the score the player just got to the high scores chart.
        HighScores::SubmitScore(GameDetails::GetDefenderColour(),GameDetails::GetWave(), GameDetails::GetScore());
        // Reset the score so the chart won't get confused next time we enter.
        GameDetails::ResetScore();
    }
    return 0;
}

int GameSetHighScoreName(lua_State* L)
{
    const char* name = luaL_checkstring(L,1);
    HighScores::SubmitName(name);
    return 0;
}
