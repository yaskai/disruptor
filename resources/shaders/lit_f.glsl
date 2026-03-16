#version 330

in vec2 fragTexCoord;
in vec2 fragTexCoord2;

uniform sampler2D texture0;
uniform sampler2D texture1;

out vec4 finalColor;

void main() {
	vec4 diffuse = texture(texture0, fragTexCoord);
	vec4 light = texture(texture1, fragTexCoord2);

	finalColor = diffuse * light * 2.5;
	//finalColor = diffuse * light * 0.5;
	//finalColor.a = 1.0;
	//finalColor = vec4(fragTexCoord2.x, fragTexCoord2.y, 0.0, 1.0);
	//finalColor = texture(texture1, fragTexCoord);
	//finalColor = light;
	//finalColor = diffuse;
}
