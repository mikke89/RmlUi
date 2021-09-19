#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out vec4 outColor;

void main() {
   outColor = inColor;
   outTexCoord = inTexCoord;
   gl_Position = vec4(pos, 0.0f, 1.0f);
} 