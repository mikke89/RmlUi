#version 450

#define BLUR_SIZE 7
#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

layout(set = 0, binding = 0) uniform sampler2D g_InputTexture;

layout(set = 1, binding = 0) uniform SharedConstantBuffer
{
	mat4 m_transform;
	vec2 m_translate;
	vec2 m_texelOffset;
	vec4 m_weights;
	vec2 m_texCoordMin;
	vec2 m_texCoordMax;
};

layout(location = 0) in vec2 fragTexCoord[BLUR_SIZE];

layout(location = 0) out vec4 finalColor;

void main() {
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	for (int i = 0; i < BLUR_SIZE; i++) {
		vec2 in_region = step(m_texCoordMin, fragTexCoord[i]) * step(fragTexCoord[i], m_texCoordMax);
		color += texture(g_InputTexture, fragTexCoord[i]) * in_region.x * in_region.y * m_weights[abs(i - BLUR_NUM_WEIGHTS + 1)];
	}
	finalColor = color;
}
