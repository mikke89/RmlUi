#version 450

layout(set = 0, binding = 0) uniform sampler2D g_InputTexture;

// Drop shadow parameters (matches the DX12 renderer's 'DropShadowBuffer', 32 bytes used).
layout(set = 1, binding = 0) uniform DropShadowBuffer
{
	vec2 m_texCoordMin;
	vec2 m_texCoordMax;
	vec4 m_color;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 finalColor;

void main() {
	vec2 in_region = step(m_texCoordMin, fragTexCoord) * step(fragTexCoord, m_texCoordMax);
	finalColor = texture(g_InputTexture, fragTexCoord).a * in_region.x * in_region.y * m_color;
}
