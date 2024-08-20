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

#ifndef RMLUI_TESTS_COMMON_TYPESTOSTRING_H
#define RMLUI_TESTS_COMMON_TYPESTOSTRING_H

#include <RmlUi/Core/TypeConverter.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Variant.h>
#include <doctest.h>
#include <ostream>

/*
 *   Provides string conversion of types for doctest.
 */
namespace Rml {

inline std::ostream& operator<<(std::ostream& os, const Colourb& value)
{
	os << ToString(value);
	return os;
}
inline std::ostream& operator<<(std::ostream& os, const ColourbPremultiplied& value)
{
	os << (int)value.red << ", " << (int)value.green << ", " << (int)value.blue << ", " << (int)value.alpha;
	return os;
}
inline std::ostream& operator<<(std::ostream& os, const Vector2f& value)
{
	os << ToString(value);
	return os;
}
inline std::ostream& operator<<(std::ostream& os, const Vector2i& value)
{
	os << ToString(value);
	return os;
}

inline std::ostream& operator<<(std::ostream& os, Span<const Vertex> vertices)
{
	for (const Vertex& vertex : vertices)
	{
		os << "{{" << vertex.position << "}, {" << vertex.colour << "}, {" << vertex.tex_coord << "}}," << std::endl;
	}
	return os;
}
inline std::ostream& operator<<(std::ostream& os, const Vector<Vertex>& vertices)
{
	os << Span<const Vertex>(vertices);
	return os;
}

inline std::ostream& operator<<(std::ostream& os, Span<const int> indices)
{
	for (int index : indices)
		os << index << ", ";
	return os;
}
inline std::ostream& operator<<(std::ostream& os, const Vector<int>& indices)
{
	os << Span<const int>(indices);
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const Mesh& value)
{
	os << "vertices: {\n" << value.vertices << "}\n";
	os << "indices: {\n" << value.indices << "\n}\n";
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const Variant& value)
{
	os << value.Get<String>();
	return os;
}

} // namespace Rml

#endif
