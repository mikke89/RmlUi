
function Startup()
    --for k,v in pairs(rocket) do Log(k) end
    --yes, maincontext is supposed to be global
    --rocket.CreateContext("notmain",Vector2i(20,15))
    local escape = rocket.key_identifier["ESCAPE"]
    if escape ~= nil then
    
        maincontext = rocket.contexts()["main"]
        maincontext:LoadDocument("data/background.rml"):Show()
        local doc = maincontext:LoadDocument("data/main_menu.rml")
        doc:Show()
    end
end

Startup()