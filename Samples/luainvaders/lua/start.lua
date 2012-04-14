
function Startup()
    local escape = rocket.key_identifier["ESCAPE"]
    if escape ~= nil then
        maincontext = rocket.contexts()["main"]
        maincontext:LoadDocument("data/background.rml"):Show()
        local doc = maincontext:LoadDocument("data/main_menu.rml")
        doc:Show()
    end
end

Startup()