#version 330

#define PI 3.14159265358979323846

in vec2 fragTexCoord;
in vec2 fragTexCoord2;

in vec3 fragPosition;

out vec4 finalColor;

uniform float time;

/*
float noise(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453 + time);
}
*/

float noise(vec2 co) {
    //return fract(sin(dot(co, vec2(12.9898, 1))) * 43758.5453 * time);
    //return fract(sin(dot(co, vec2(0, 1 + time))) * time);
	float val = sin((co.y*100.5) - (time*20.1));

    val -= fract(cos(dot(co, vec2(12.9898, 78.233))) * 43758.5453 + time) * 0.5;
	val = clamp(val, 0.0, 1.25);

	return val;

	//return 1.0;
}

void main() {
	//vec4 diffuse = texture(texture0, fragTexCoord);
	//vec4 light = texture(texture1, fragTexCoord2);

	//finalColor = vec4(diffuse.x, diffuse.y, diffuse.z, 0.1);
	//finalColor = diffuse;

	vec4 diffuse = vec4(0.5, 0.6, 1.0, 0.25);

	float dither1 = noise(fragTexCoord);
	diffuse = diffuse * dither1;

	diffuse.a = clamp(diffuse.a, 0.0, 0.5);

	finalColor = diffuse;
}
