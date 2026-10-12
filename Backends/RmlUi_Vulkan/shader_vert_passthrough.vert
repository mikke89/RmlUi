#version 450

// Fullscreen/postprocess vertex shader. Unlike the DX12 version there is no UV Y-flip here: this backend flips NDC Y
// through the projection correction matrix, so framebuffer space is top-left-origin and UVs are used as-is.
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor0;
layout(location = 2) in vec2 inTexCoord0;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

void main() {
	gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
	fragColor = inColor0;
	fragTexCoord = inTexCoord0;
}
