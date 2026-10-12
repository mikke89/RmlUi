#version 450

#define BLUR_SIZE 7
#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

// Shared blur constant buffer (matches the DX12 renderer's 'SharedConstantBuffer', 112 bytes used).
// Only m_texelOffset is used by the vertex stage; the fragment stage reads the rest.
layout(set = 1, binding = 0) uniform SharedConstantBuffer
{
	mat4 m_transform;
	vec2 m_translate;
	vec2 m_texelOffset;
	vec4 m_weights;
	vec2 m_texCoordMin;
	vec2 m_texCoordMax;
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor0;
layout(location = 2) in vec2 inTexCoord0;

layout(location = 0) out vec2 fragTexCoord[BLUR_SIZE];

void main() {
	for (int i = 0; i < BLUR_SIZE; i++) {
		fragTexCoord[i] = inTexCoord0 - float(i - BLUR_NUM_WEIGHTS + 1) * m_texelOffset;
	}
	gl_Position = vec4(inPosition.xy, 1.0, 1.0);
}
