#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

uniform vec4 bg_color;
uniform float time;

float rand(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
    float size = 100.0;
	float prob = 0.9;
	vec2 pos = floor(1.0 / size * fragTexCoord.xy);
	float color = 0.0;
	float starValue = rand(pos);

    if (starValue > prob)
	{
		vec2 center = size * pos + vec2(size, size) * 0.5;
		float t = 0.9 + 0.2 * sin(time * 8.0 + (starValue - prob) / (1.0 - prob) * 45.0);
		color = 1.0 - distance(fragTexCoord.xy, center) / (0.5 * size);
		color = color * t / (abs(fragTexCoord.y - center.y)) * t / (abs(fragTexCoord.x - center.x));

	} 
    else if (rand(fragTexCoord.xy / 20.0) > 0.996)
	{
		float r = rand(fragTexCoord.xy);
		color = r * (0.85 * sin(time * (r * 5.0) + 720.0 * r) + 0.95);
	}

    finalColor = vec4(vec3(color),1.0) + bg_color;
}


