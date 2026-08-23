#version 450

// Per-draw transform data (matches the DX12 renderer's 'ConstantBuffer' b0, 72 bytes used).
layout(set = 0, binding = 0) uniform UserData
{
	mat4 m_transform;
	vec2 m_translate;
} userdata;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor0;
layout(location = 2) in vec2 inTexCoord0;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

void main() {
	fragTexCoord = inTexCoord0;
	fragColor = inColor0;
	vec2 translatedPos = inPosition + userdata.m_translate.xy;
	vec4 outPos = userdata.m_transform * vec4(translatedPos, 0, 1);
	gl_Position = outPos;
}
