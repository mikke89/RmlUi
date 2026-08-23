#version 450

layout(set = 0, binding = 0) uniform sampler2D g_InputTexture;

// Color matrix (matches the DX12 renderer's 'ConstantBuffer' for the color matrix program, 64 bytes used).
// The matrix is uploaded already transposed for column-major consumption, like the DX12 renderer does.
layout(set = 1, binding = 0) uniform ConstantBuffer
{
	mat4 m_color_matrix;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 finalColor;

void main() {
	vec4 texColor = texture(g_InputTexture, fragTexCoord);
	vec3 transformedColor = (m_color_matrix * texColor).rgb;
	finalColor = vec4(transformedColor, texColor.a);
}
