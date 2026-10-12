#version 450

// Creation effect parameters (matches the DX12 renderer's creation 'SharedConstantBuffer', 84 bytes used).
layout(set = 0, binding = 0) uniform SharedConstantBuffer
{
	mat4 m_transform;
	vec2 m_translate;
	vec2 m_dimensions;
	float m_value;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 finalColor;

void main() {
	float t = m_value;
	vec3 c;
	float l;
	for (int i = 0; i < 3; i++) {
		vec2 p = fragTexCoord;
		vec2 uv = p;
		p -= .5;
		p.x *= m_dimensions.x / m_dimensions.y;
		float z = t + float(i) * .07;
		l = length(p);
		uv += p / l * (sin(z) + 1.) * abs(sin(l * 9. - z - z));
		c[i] = .01 / length(mod(uv, 1.) - .5);
	}
	finalColor = vec4(c / l, fragColor.a);
}
