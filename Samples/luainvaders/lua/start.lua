


function Startup()
	maincontext = rocket.contexts()["main"]
	maincontext:LoadDocument("data/background.rml"):Show()
	maincontext:LoadDocument("data/main_menu.rml"):Show()
end

Startup()