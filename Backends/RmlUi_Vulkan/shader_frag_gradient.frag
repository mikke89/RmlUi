#version 450

#define MAX_NUM_STOPS 16
#define MAX_NUM_STOPS_PACKED (MAX_NUM_STOPS / 4)
#define LINEAR 0
#define RADIAL 1
#define CONIC 2
#define REPEATING_LINEAR 3
#define REPEATING_RADIAL 4
#define REPEATING_CONIC 5
#define PI 3.14159265

// Gradient parameters (matches the DX12 renderer's gradient 'SharedConstantBuffer', 416 bytes, zero padding).
layout(set = 0, binding = 0) uniform SharedConstantBuffer
{
	mat4 m_transform;
	vec2 m_translate;

	int m_func;     // one of the above definitions
	int m_num_stops;
	vec2 m_p;       // linear: starting point,         radial: center,                        conic: center
	vec2 m_v;       // linear: vector to ending point, radial: 2d curvature (inverse radius), conic: angled unit vector
	vec4 m_stop_colors[MAX_NUM_STOPS];
	vec4 m_stop_positions[MAX_NUM_STOPS_PACKED]; // normalized, 0 -> starting point, 1 -> ending point
};

// Hide the way the data is packed in the constant buffer through a macro.
#define GET_STOP_POS(i) (m_stop_positions[i >> 2][i & 3])

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 finalColor;

vec4 lerp_stop_colors(float t) {
	vec4 color = m_stop_colors[0];
	for (int i = 1; i < m_num_stops; i++)
		color = mix(color, m_stop_colors[i], smoothstep(GET_STOP_POS(i - 1), GET_STOP_POS(i), t));
	return color;
}

void main() {
	float t = 0.0;
	if (m_func == LINEAR || m_func == REPEATING_LINEAR) {
		float dist_square = dot(m_v, m_v);
		vec2 V = fragTexCoord.xy - m_p;
		t = dot(m_v, V) / dist_square;
	}
	else if (m_func == RADIAL || m_func == REPEATING_RADIAL) {
		vec2 V = fragTexCoord.xy - m_p;
		t = length(m_v * V);
	}
	else if (m_func == CONIC || m_func == REPEATING_CONIC) {
		// Same rotation as the DX12 shader: V = mul((uv - p), R) with R = float2x2(m_v.x, -m_v.y, m_v.y, m_v.x).
		mat2 R = mat2(m_v.x, -m_v.y, m_v.y, m_v.x);
		vec2 V = R * (fragTexCoord.xy - m_p);
		t = 0.5 + atan(-V.x, V.y) / (2.0 * PI);
	}
	if (m_func == REPEATING_LINEAR || m_func == REPEATING_RADIAL || m_func == REPEATING_CONIC) {
		float t0 = GET_STOP_POS(0);
		float t1 = GET_STOP_POS(m_num_stops - 1);
		t = t0 + mod(t - t0, t1 - t0);
	}
	finalColor = fragColor * lerp_stop_colors(t);
}
