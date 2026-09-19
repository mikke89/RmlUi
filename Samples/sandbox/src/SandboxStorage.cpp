#include "SandboxStorage.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/StringUtilities.h>
#include <cstdio>

Rml::String LoadFile(const Rml::String& path)
{
	FILE* file_handle = std::fopen(path.c_str(), "rb");
	if (!file_handle)
		return {};

	std::fseek(file_handle, 0, SEEK_END);
	const long file_size = std::ftell(file_handle);
	std::fseek(file_handle, 0, SEEK_SET);
	if (file_size < 0)
	{
		std::fclose(file_handle);
		return {};
	}

	Rml::String content((size_t)file_size, char{});
	const size_t bytes_read = std::fread(content.data(), 1, content.size(), file_handle);
	content.resize(bytes_read);

	std::fclose(file_handle);
	return content;
}

bool SaveFile(const Rml::String& path, const Rml::String& content)
{
	FILE* file_handle = std::fopen(path.c_str(), "wb");
	if (!file_handle)
		return false;

	size_t written_bytes = std::fwrite(content.c_str(), 1, content.size(), file_handle);
	std::fclose(file_handle);
	return written_bytes == content.size();
}

Rml::UnorderedMap<Rml::String, Rml::String> LoadOptions(const Rml::String& path)
{
	Rml::String content = LoadFile(path);
	if (content.empty())
		return {};

	Rml::UnorderedMap<Rml::String, Rml::String> result;

	size_t line_index = 0;
	while (true)
	{
		size_t separator_index = content.find('=', line_index);

		if (separator_index >= content.size())
			break;

		size_t value_index = separator_index + 1;

		size_t end_line_index = content.find('\n', value_index);
		if (end_line_index == Rml::String::npos)
			end_line_index = content.size();

		std::string key = Rml::StringUtilities::StripWhitespace(content.substr(line_index, separator_index - line_index));
		std::string value = Rml::StringUtilities::StripWhitespace(content.substr(value_index, end_line_index - value_index));
		result.emplace(std::move(key), std::move(value));

		if (end_line_index >= content.size())
			break;

		line_index = end_line_index + 1;
	}

	return result;
}

void SaveOptions(const Rml::String& path, const Rml::UnorderedMap<Rml::String, Rml::String>& options)
{
	Rml::String content;
	content.reserve(options.size() * 50);

	for (const auto& option : options)
	{
		content += option.first;
		content += '=';
		content += option.second;
		content += '\n';
	}

	if (!SaveFile(path, content))
		Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to write options to file '%s'", path.c_str());
}
