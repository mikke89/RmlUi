#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "GeometryBackgroundBorder.h"

namespace Rml {

void MeshUtilities::GenerateQuad(Mesh& mesh, Vector2f origin, Vector2f dimensions, ColourbPremultiplied colour)
{
	GenerateQuad(mesh, origin, dimensions, colour, Vector2f(0, 0), Vector2f(1, 1));
}

void MeshUtilities::GenerateQuad(Mesh& mesh, Vector2f origin, Vector2f dimensions, ColourbPremultiplied colour, Vector2f top_left_texcoord,
	Vector2f bottom_right_texcoord)
{
	const int v0 = (int)mesh.vertices.size();
	const int i0 = (int)mesh.indices.size();

	mesh.vertices.resize(mesh.vertices.size() + 4);
	mesh.indices.resize(mesh.indices.size() + 6);
	Vertex* vertices = mesh.vertices.data();
	int* indices = mesh.indices.data();

	vertices[v0 + 0].position = origin;
	vertices[v0 + 0].colour = colour;
	vertices[v0 + 0].tex_coord = top_left_texcoord;

	vertices[v0 + 1].position = Vector2f(origin.x + dimensions.x, origin.y);
	vertices[v0 + 1].colour = colour;
	vertices[v0 + 1].tex_coord = Vector2f(bottom_right_texcoord.x, top_left_texcoord.y);

	vertices[v0 + 2].position = origin + dimensions;
	vertices[v0 + 2].colour = colour;
	vertices[v0 + 2].tex_coord = bottom_right_texcoord;

	vertices[v0 + 3].position = Vector2f(origin.x, origin.y + dimensions.y);
	vertices[v0 + 3].colour = colour;
	vertices[v0 + 3].tex_coord = Vector2f(top_left_texcoord.x, bottom_right_texcoord.y);

	indices[i0 + 0] = v0 + 0;
	indices[i0 + 1] = v0 + 3;
	indices[i0 + 2] = v0 + 1;

	indices[i0 + 3] = v0 + 1;
	indices[i0 + 4] = v0 + 3;
	indices[i0 + 5] = v0 + 2;
}

void MeshUtilities::GenerateLine(Mesh& mesh, Vector2f position, Vector2f size, ColourbPremultiplied color)
{
	Math::SnapToPixelGrid(position, size);
	MeshUtilities::GenerateQuad(mesh, position, size, color);
}

void MeshUtilities::GenerateBackgroundBorder(Mesh& out_mesh, const RenderBox& render_box, ColourbPremultiplied background_color,
	const ColourbPremultiplied border_colors[4])
{
	RMLUI_ASSERT(border_colors);

	Vector<Vertex>& vertices = out_mesh.vertices;
	Vector<int>& indices = out_mesh.indices;

	const EdgeSizes& border_widths = render_box.GetBorderWidths();
	int num_borders = 0;
	for (int i = 0; i < 4; i++)
		if (border_colors[i].alpha > 0 && border_widths[i] > 0)
			num_borders += 1;

	const Vector2f fill_size = render_box.GetFillSize();
	const bool has_background = (background_color.alpha > 0 && fill_size.x > 0 && fill_size.y > 0);
	const bool has_border = (num_borders > 0);

	if (!has_background && !has_border)
		return;

	// Reserve geometry. A conservative estimate, does not take border-radii into account and assumes same-colored borders.
	const int estimated_num_vertices = 4 * int(has_background) + 2 * num_borders;
	const int estimated_num_triangles = 2 * int(has_background) + 2 * num_borders;
	vertices.reserve((int)vertices.size() + estimated_num_vertices);
	indices.reserve((int)indices.size() + 3 * estimated_num_triangles);

	// Generate the geometry.
	GeometryBackgroundBorder geometry(vertices, indices);
	const BorderMetrics metrics =
		GeometryBackgroundBorder::ComputeBorderMetrics(render_box.GetBorderOffset(), border_widths, fill_size, render_box.GetBorderRadius());

	if (has_background)
		geometry.DrawBackground(metrics, background_color);

	if (has_border)
		geometry.DrawBorder(metrics, border_widths, border_colors);

#if 0
	// Debug draw vertices
	if (render_box.border_radius != CornerSizes{})
	{
		const int num_vertices = (int)vertices.size();
		const int num_indices = (int)indices.size();
		vertices.reserve(num_vertices + 4 * num_vertices);
		indices.reserve(num_indices + 6 * num_indices);

		for (int i = 0; i < num_vertices; i++)
			MeshUtilities::GenerateQuad(out_mesh, vertices[i].position, Vector2f(3, 3), ColourbPremultiplied(255, 0, (i % 2) == 0 ? 0 : 255));
	}
#endif

#ifdef RMLUI_DEBUG
	const int num_vertices = (int)vertices.size();
	for (int index : indices)
	{
		RMLUI_ASSERT(index < num_vertices);
	}
#endif
}

void MeshUtilities::GenerateBackground(Mesh& out_mesh, const RenderBox& render_box, ColourbPremultiplied color)
{
	const Vector2f fill_size = render_box.GetFillSize();
	const bool has_background = (color.alpha > 0 && fill_size.x > 0 && fill_size.y > 0);
	if (!has_background)
		return;

	const BorderMetrics metrics = GeometryBackgroundBorder::ComputeBorderMetrics(render_box.GetBorderOffset(), render_box.GetBorderWidths(),
		fill_size, render_box.GetBorderRadius());

	Vector<Vertex>& vertices = out_mesh.vertices;
	Vector<int>& indices = out_mesh.indices;

	// Reserve geometry. A conservative estimate, does not take border-radii into account.
	vertices.reserve((int)vertices.size() + 4);
	indices.reserve((int)indices.size() + 6);

	// Generate the geometry
	GeometryBackgroundBorder geometry(vertices, indices);
	geometry.DrawBackground(metrics, color);
}

} // namespace Rml
