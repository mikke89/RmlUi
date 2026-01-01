#pragma once

#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/TypeConverter.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Variant.h>
#include <RmlUi/Core/Vertex.h>
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
