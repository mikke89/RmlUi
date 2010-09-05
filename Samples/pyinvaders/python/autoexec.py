import rocket

def Startup():
	context = rocket.contexts['main']
	context.LoadDocument('data/background.rml').Show()
	doc = context.LoadDocument('data/main_menu.rml')
	doc.Show()
	
Startup()


class NameDataFormatter(rocket.DataFormatter):
	def __init__(self):
		rocket.DataFormatter.__init__(self, "name")

	def FormatData(self, raw_data):
		""" 
		Data format:
		raw_data[0] is the name.
		raw_data[1] is a bool - True means the name has to be entered. False means the name has been entered already.
		"""
		
		formatted_data = ""
		
		if (raw_data[1] == "1"):
			formatted_data = "<input id=\"player_input\" type=\"text\" name=\"name\" onkeydown=\"OnKeyDown(event)\" />"
		else:
			formatted_data = raw_data[0]
			
		return formatted_data
		
class ShipDataFormatter(rocket.DataFormatter):
	def __init__(self):
		rocket.DataFormatter.__init__(self, "ship")
		
	def FormatData(self, raw_data):
		return "<defender style=\"color: rgba(" + raw_data[0] + ");\" />";

name_formatter = NameDataFormatter()
ship_formatter = ShipDataFormatter()
