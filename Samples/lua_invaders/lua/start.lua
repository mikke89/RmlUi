function Startup()
	maincontext = rmlui.contexts["main"]
	maincontext:LoadDocument("lua_invaders/data/background.rml"):Show()
	maincontext:LoadDocument("lua_invaders/data/main_menu.rml"):Show()
end

Startup()
