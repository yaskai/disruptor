#version 330

in vec2 fragTexCoord;
in vec2 fragTexCoord2;

in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform sampler2D texture1;

uniform vec3 light_pos[3];
uniform vec3 light_clr[3];

out vec4 finalColor;

void main() {
	//vec4 diffuse = texture(texture0, fragTexCoord);
	//vec4 light = texture(texture1, fragTexCoord2);

	//finalColor = diffuse * light * 2.25;

	vec4 diffuse = texture(texture0, fragTexCoord);
	finalColor = diffuse;
}
