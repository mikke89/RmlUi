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
layout (location = 3) in vec2 inRelativePosition;

layout (location = 0) out vec2 fragTexCoord;
layout (location = 1) out vec4 fragColor;

// You might wonder why not to use round() function instead. It turns out that if you use round(), fonts are blurry!
float perfect_round(float n) {
	return float(int(n + 0.5));
}

vec2 perfect_round(vec2 v) {
	return vec2(perfect_round(v.x), perfect_round(v.y));
}

void main() {
	fragTexCoord = inTexCoord0;
	fragColor = inColor0;
	vec2 translatedPos = inPosition + userdata.m_translate.xy;

	if (inRelativePosition.x > 0.f || inRelativePosition.y > 0.f)
	{
		vec2 translatedRelativePos = inRelativePosition + userdata.m_translate.xy;
		vec2 translatedRelativePosSnapped = perfect_round(translatedRelativePos);
		vec2 diff = translatedRelativePosSnapped - translatedRelativePos;
		translatedPos += diff;
	}
	else
	{
		translatedPos = perfect_round( translatedPos );
	}

	vec4 outPos = userdata.m_transform * vec4(translatedPos, 0, 1);
    gl_Position = outPos;
}