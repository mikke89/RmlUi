#ifndef LUAINTERFACE_H
#define LUAINTERFACE_H 
/*
    This will define the "Game" global table in Lua and some functions with it.

    In Lua, the skeleton definition of Game with the fake function definitions (because it calls c++ code) would look something like

    Game = Game or {} --so if something else made a "Game" table, we would save the previous table, and just add on to it
    Game.Shutdown = function() Shell::RequestExit() end
    Game.SetPaused = function(paused) GameDetails::SetPaused(paused) end --where paused is a bool
    Game.SetDifficulty = function(difficulty) GameDetails::SetDifficulty(difficulty) end --difficulty is a value from Game.difficulty
    Game.SetDefenderColour = function(colour) GameDetails::SetDefenderColour(colour) end --colour is of type Colourb
    Game.SubmitHighScore = function() HighScores::SubmitScore(stuff from GameDetails) end
    Game.SetHighScoreName = function(name) HighScore::SubmitName(name) end -- name is a string
    Game.difficulty = { "HARD" = GameDetails::HARD, "EASY" = GameDetails::EASY }
*/

struct lua_State;
class Game;
class LuaInterface
{
public:
    static void Initialise(lua_State* L);
    static void InitGame(lua_State* L);
};

#endif