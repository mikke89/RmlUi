function Startup()
	maincontext = rmlui.contexts["main"]
	maincontext:LoadDocument("luainvaders/data/background.rml"):Show()
	maincontext:LoadDocument("luainvaders/data/main_menu.rml"):Show()
end

Startup()
