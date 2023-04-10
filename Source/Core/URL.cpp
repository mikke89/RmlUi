/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "../../Include/RmlUi/Core/URL.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include <stdio.h>
#include <string.h>

namespace Rml {

const char* DEFAULT_PROTOCOL = "file";

URL::URL()
{
	port = 0;
	url_dirty = false;
}

URL::URL(const String& _url)
{
	port = 0;
	RMLUI_VERIFY(SetURL(_url));
}

URL::URL(const char* _url)
{
	port = 0;
	RMLUI_VERIFY(SetURL(_url));
}

URL::~URL() {}

bool URL::SetURL(const String& _url)
{
	url_dirty = false;
	url = _url;

	// Make sure an Empty URL is completely Empty.
	if (url.empty())
	{
		protocol.clear();
		login.clear();
		password.clear();
		host.clear();
		port = 0;
		path.clear();
		file_name.clear();
		extension.clear();

		return true;
	}

	// Find the protocol. This consists of the string appearing before the
	// '://' token (ie, file://, http://).
	const char* host_begin = strchr(_url.c_str(), ':');
	if (nullptr != host_begin)
	{
		protocol = String(_url.c_str(), host_begin);
		if (0 != strncmp(host_begin, "://", 3))
		{
			char malformed_terminator[4] = {0, 0, 0, 0};
			strncpy(malformed_terminator, host_begin, 3);
			Log::Message(Log::LT_ERROR, "Malformed protocol identifier found in URL %s; expected %s://, found %s%s.\n", _url.c_str(),
				protocol.c_str(), protocol.c_str(), malformed_terminator);

			return false;
		}
		host_begin += 3;
	}
	else
	{
		protocol = DEFAULT_PROTOCOL;
		host_begin = _url.c_str();
	}

	// We only want to look for a host if a protocol was specified.
	const char* path_begin;
	if (host_begin != _url.c_str())
	{
		// Find the host. This is the string appearing after the protocol or after
		// the username:password combination, and terminated either with a colon,
		// if a port is specified, or a forward slash if there is no port.

		// Check for a login pair
		const char* at_symbol = strchr(host_begin, '@');
		if (at_symbol)
		{
			String login_password;
			login_password = String(host_begin, at_symbol);
			host_begin = at_symbol + 1;

			const char* password_ptr = strchr(login_password.c_str(), ':');
			if (password_ptr)
			{
				login = String(login_password.c_str(), password_ptr);
				password = String(password_ptr + 1);
			}
			else
			{
				login = login_password;
			}
		}

		// Get the host portion
		path_begin = strchr(host_begin, '/');
		// Search for the colon in the host name, which will indicate a port.
		const char* port_begin = strchr(host_begin, ':');
		if (nullptr != port_begin && (nullptr == path_begin || port_begin < path_begin))
		{
			if (1 != sscanf(port_begin, ":%d", &port))
			{
				Log::Message(Log::LT_ERROR, "Malformed port number found in URL %s.\n", _url.c_str());
				return false;
			}

			host = String(host_begin, port_begin);

			// Don't continue if there is no path.
			if (nullptr == path_begin)
			{
				return true;
			}

			// Increment the path string past the trailing slash.
			++path_begin;
		}
		else
		{
			port = -1;

			if (nullptr == path_begin)
			{
				host = host_begin;
				return true;
			}
			else
			{
				// Assign the host name, then increment the path string past the
				// trailing slash.
				host = String(host_begin, path_begin);
				++path_begin;
			}
		}
	}
	else
	{
		path_begin = _url.c_str();
	}

	// Check for parameters
	String path_segment;
	const char* parameters = strchr(path_begin, '?');
	if (parameters)
	{
		// Pull the path segment out, so further processing doesn't read the parameters
		path_segment = String(path_begin, parameters);
		path_begin = path_segment.c_str();

		// Loop through all parameters, loading them
		StringList parameter_list;
		StringUtilities::ExpandString(parameter_list, parameters + 1, '&');
		for (size_t i = 0; i < parameter_list.size(); i++)
		{
			// Split into key and value
			StringList key_value;
			StringUtilities::ExpandString(key_value, parameter_list[i], '=');

			key_value[0] = UrlDecode(key_value[0]);
			if (key_value.size() == 2)
				this->parameters[key_value[0]] = UrlDecode(key_value[1]);
			else
				this->parameters[key_value[0]] = "";
		}
	}

	// Find the path. This is the string appearing after the host, terminated
	// by the last forward slash.
	const char* file_name_begin = strrchr(path_begin, '/');
	if (nullptr == file_name_begin)
	{
		// No path!
		file_name_begin = path_begin;
		path = "";
	}
	else
	{
		// Copy the path including the trailing slash.
		path = String(path_begin, ++file_name_begin);

		// Normalise the path, stripping any ../'s from it
		size_t parent_dir_pos = String::npos;
		while ((parent_dir_pos = path.find("/../")) != String::npos && parent_dir_pos != 0)
		{
			// Find the start of the parent directory.
			size_t parent_dir_start_pos = path.rfind('/', parent_dir_pos - 1);
			if (parent_dir_start_pos == String::npos)
				parent_dir_start_pos = 0;
			else
				parent_dir_start_pos += 1;

			// Strip out the parent dir and the /..
			path.erase(parent_dir_start_pos, parent_dir_pos - parent_dir_start_pos + 4);

			// We've altered the URL, mark it dirty
			url_dirty = true;
		}
	}

	// Find the file name. This is the string after the trailing slash of the
	// path, and just before the extension.
	const char* extension_begin = strrchr(file_name_begin, '.');
	if (nullptr == extension_begin)
	{
		file_name = file_name_begin;
		extension = "";
	}
	else
	{
		file_name = String(file_name_begin, extension_begin);
		extension = extension_begin + 1;
	}

	return true;
}

const String& URL::GetURL() const
{
	if (url_dirty)
		ConstructURL();

	return url;
}

bool URL::SetProtocol(const String& _protocol)
{
	protocol = _protocol;
	url_dirty = true;

	return true;
}

const String& URL::GetProtocol() const
{
	return protocol;
}

bool URL::SetLogin(const String& _login)
{
	login = _login;
	url_dirty = true;
	return true;
}

const String& URL::GetLogin() const
{
	return login;
}

bool URL::SetPassword(const String& _password)
{
	password = _password;
	url_dirty = true;
	return true;
}

const String& URL::GetPassword() const
{
	return password;
}

bool URL::SetHost(const String& _host)
{
	host = _host;
	url_dirty = true;

	return true;
}

const String& URL::GetHost() const
{
	return host;
}

bool URL::SetPort(int _port)
{
	port = _port;
	url_dirty = true;

	return true;
}

int URL::GetPort() const
{
	return port;
}

bool URL::SetPath(const String& _path)
{
	path = _path;
	url_dirty = true;

	return true;
}

bool URL::PrefixPath(const String& prefix)
{
	// If there's no trailing slash on the end of the prefix, add one.
	if (!prefix.empty() && prefix[prefix.size() - 1] != '/')
		path = prefix + "/" + path;
	else
		path = prefix + path;

	url_dirty = true;

	return true;
}

const String& URL::GetPath() const
{
	return path;
}

bool URL::SetFileName(const String& _file_name)
{
	file_name = _file_name;
	url_dirty = true;

	return true;
}

const String& URL::GetFileName() const
{
	return file_name;
}

bool URL::SetExtension(const String& _extension)
{
	extension = _extension;
	url_dirty = true;

	return true;
}

const String& URL::GetExtension() const
{
	return extension;
}

const URL::Parameters& URL::GetParameters() const
{
	return parameters;
}

void URL::SetParameter(const String& key, const String& value)
{
	parameters[key] = value;
	url_dirty = true;
}

void URL::SetParameters(const Parameters& _parameters)
{
	parameters = _parameters;
	url_dirty = true;
}

void URL::ClearParameters()
{
	parameters.clear();
}

String URL::GetPathedFileName() const
{
	String pathed_file_name = path;

	// Append the file name.
	pathed_file_name += file_name;

	// Append the extension.
	if (!extension.empty())
	{
		pathed_file_name += ".";
		pathed_file_name += extension;
	}

	return pathed_file_name;
}

String URL::GetQueryString() const
{
	String query_string;

	int count = 0;
	for (Parameters::const_iterator itr = parameters.begin(); itr != parameters.end(); ++itr)
	{
		query_string += (count == 0) ? "" : "&";

		query_string += UrlEncode((*itr).first);
		query_string += "=";
		query_string += UrlEncode((*itr).second);

		count++;
	}

	return query_string;
}

bool URL::operator<(const URL& rhs) const
{
	if (url_dirty)
		ConstructURL();
	if (rhs.url_dirty)
		rhs.ConstructURL();

	return url < rhs.url;
}

void URL::ConstructURL() const
{
	url = "";

	// Append the protocol.
	if (!protocol.empty() && !host.empty())
	{
		url = protocol;
		url += "://";
	}

	// Append login and password
	if (!login.empty())
	{
		url += login;
		if (!password.empty())
		{
			url += ":";
			url += password;
		}
		url += "@";
	}
	RMLUI_ASSERTMSG(password.empty() || (!password.empty() && !login.empty()), "Can't have a password without a login!");

	// Append the host.
	url += host;

	// Only check ports if there is some host/protocol part
	if (!url.empty())
	{
		if (port > 0)
		{
			RMLUI_ASSERTMSG(!host.empty(), "Can't have a port without a host!");
			constexpr size_t port_buffer_size = 16;
			char port_string[port_buffer_size];
			snprintf(port_string, port_buffer_size, ":%d/", port);
			url += port_string;
		}
		else
		{
			url += "/";
		}
	}

	// Append the path.
	if (!path.empty())
	{
		url += path;
	}

	// Append the file name.
	url += file_name;

	// Append the extension.
	if (!extension.empty())
	{
		url += ".";
		url += extension;
	}

	// Append parameters
	if (!parameters.empty())
	{
		url += "?";
		url += GetQueryString();
	}

	url_dirty = false;
}

String URL::UrlEncode(const String& value)
{
	String encoded;
	constexpr size_t hex_buffer_size = 4;
	char hex[hex_buffer_size] = {0, 0, 0, 0};

	encoded.clear();

	const char* value_c = value.c_str();
	for (String::size_type i = 0; value_c[i]; i++)
	{
		char c = value_c[i];
		if (IsUnreservedChar(c))
			encoded += c;
		else
		{
			snprintf(hex, hex_buffer_size, "%%%02X", c);
			encoded += hex;
		}
	}

	return encoded;
}

String URL::UrlDecode(const String& value)
{
	String decoded;

	decoded.clear();

	const char* value_c = value.c_str();
	String::size_type value_len = value.size();
	for (String::size_type i = 0; i < value_len; i++)
	{
		char c = value_c[i];
		if (c == '+')
		{
			decoded += ' ';
		}
		else if (c == '%')
		{
			char* endp;
			String t = value.substr(i + 1, 2);
			int ch = strtol(t.c_str(), &endp, 16);
			if (*endp == '\0')
				decoded += char(ch);
			else
				decoded += t;
			i += 2;
		}
		else
		{
			decoded += c;
		}
	}

	return decoded;
}

bool URL::IsUnreservedChar(const char in)
{
	switch (in)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '-':
	case '.':
	case '_':
	case '~': return true;
	default: break;
	}
	return false;
}

} // namespace Rml
