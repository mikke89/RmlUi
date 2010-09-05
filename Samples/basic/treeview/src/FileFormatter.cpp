#include "FileFormatter.h"

FileFormatter::FileFormatter() : Rocket::Controls::DataFormatter("file")
{
}

FileFormatter::~FileFormatter()
{
}

void FileFormatter::FormatData(Rocket::Core::String& formatted_data, const Rocket::Core::StringList& raw_data)
{
	if (raw_data.size() == 3)
	{
		// Add spacers for the depth.
		int depth = atoi(raw_data[1].CString());
		for (int i = 0; i < depth; ++i)
			formatted_data += "<spacer />";

		// If the file has 1 or more children, add an expand button.
		if (raw_data[2] != "0")
			formatted_data += "<datagridexpand />";
		else
			formatted_data += "<spacer />";

		// Lastly, write the filename.
		formatted_data += raw_data[0];
	}
}
