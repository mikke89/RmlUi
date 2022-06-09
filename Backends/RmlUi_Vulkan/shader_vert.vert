#version 330

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set=0, binding=1) uniform UserData 
{
	mat4 m_transform;
	vec2 m_translate;
} userdata;

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec4 inColor0;
layout (location = 2) in vec2 inTexCoord0;

layout (location = 0) out vec2 fragTexCoord;
layout (location = 1) out vec4 fragColor;

void main() {
	fragTexCoord = inTexCoord0;
	fragColor = inColor0;
	vec2 translatedPos = inPosition + userdata.m_translate.xy;
	vec4 outPos = userdata.m_transform * vec4(translatedPos, 0, 1);
    gl_Position = outPos;
}