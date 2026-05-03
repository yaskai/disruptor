#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

uniform float time;
uniform vec2 resolution;

out vec4 finalColor;

float noise(vec2 co) {
	//float val = sin((co.y*1000.0) - (sin(time*0.5)*5.0));
	//float val = sin((co.y*700.0) - time*20.5);
	float val = sin((co.y*resolution.y*0.65) - time*5.0);

    val += fract(cos(dot(co, vec2(12.9898, 78.233))) * 43758.5453 + time);
	val = clamp(val, 0.04, 100.0);

	return val;
}

void main() {
	vec4 color = texture(texture0, fragTexCoord);

	float dither = noise(fragTexCoord);

	vec2 uv = fragTexCoord - 0.5;
	float vignette = 1.0 - dot(uv, uv) * 1.5;

	dither *= vignette;

	//color *= dither;
	color.rgb = mix(color.rgb, color.rgb * dither, 0.75);

	//color.rgb = mix(color.rgb, color.rgb * (vignette), 1.95);

	//finalColor = color * fragColor * vignette;
	finalColor = color * fragColor;
}

