
Formatters = Formatters or {} --table to hold the formatters so that they don't get GC'd

--this will use two different ways to show how to create DataFormatter objects
--first way is to set the FormatData function explicitly
local formatter = DataFormatter.new("name")
formatter.FormatData = function(raw_data)
    --[[ 
    Data format:
    raw_data[0] is the name.
    raw_data[1] is a bool - True means the name has to be entered. False means the name has been entered already.
    ]]
    
    formatted_data = ""
    
    if (raw_data[1] == "1") then
        --because we know that it is only used in the high_score.rml file, use that namespace for the OnKeyDown function
        formatted_data = "<input id=\"player_input\" type=\"text\" name=\"name\" onkeydown=\"HighScore.OnKeyDown(event)\" />"
    else
        formatted_data = raw_data[0]
    end
        
    return formatted_data
end
Formatters["name"] = formatter --using "name" as the key only for convenience

--second example uses a previously defined function, and passes in as a parameter for 'new'
function SecondFormatData(raw_data)
    return "<defender style=\"color: rgba(" .. raw_data[0] .. ");\" />"
end
Formatters["ship"] = DataFormatter.new("ship",SecondFormatData)


function Startup()
	maincontext = rocket.contexts["main"]
	maincontext:LoadDocument("data/background.rml"):Show()
	maincontext:LoadDocument("data/main_menu.rml"):Show()
end

Startup()
