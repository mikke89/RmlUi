#ifndef FILEFORMATTER_H
#define FILEFORMATTER_H

#include <Rocket/Controls/DataFormatter.h>

/**
	Data formatter for file and directory columns generated from the file system data source.

	@author Peter Curry
 */

class FileFormatter : public Rocket::Controls::DataFormatter
{
public:
	FileFormatter();
	virtual ~FileFormatter();

	void FormatData(Rocket::Core::String& formatted_data, const Rocket::Core::StringList& raw_data);
};

#endif
