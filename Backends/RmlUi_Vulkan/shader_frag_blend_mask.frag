#version 450

layout(set = 0, binding = 0) uniform sampler2D g_InputTexture;
layout(set = 0, binding = 1) uniform sampler2D g_MaskTexture;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 finalColor;

void main() {
	vec4 texColor = texture(g_InputTexture, fragTexCoord);
	float maskAlpha = texture(g_MaskTexture, fragTexCoord).a;
	finalColor = texColor * maskAlpha;
}
