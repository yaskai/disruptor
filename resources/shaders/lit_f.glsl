#version 330

in vec2 frag_texcoord;
in vec2 frag_texcoord2;

uniform sampler2D texture0;
uniform sampler2D texture1;

out vec4 final_color;

void main() {
	vec4 diffuse = texture(texture0, frag_texcoord);
	diffuse = clamp(diffuse, vec4(0.0), vec4(1.0));
	vec4 light = texture(texture1, frag_texcoord2);
	light = clamp(light, vec4(0.0), vec4(1.0));

	final_color = diffuse * (light);
}
