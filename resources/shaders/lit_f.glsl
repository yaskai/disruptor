#version 330

in vec2 fragTexCoord;
in vec2 fragTexCoord2;
in vec3 fragPosition;
in vec3 fragWorldPos;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform sampler2D texture1;

out vec4 finalColor;

#define MAX_PL 16
uniform int pl_enabled[MAX_PL];
uniform vec3 pl_position[MAX_PL];
uniform vec3 pl_color[MAX_PL];
uniform float pl_radius[MAX_PL];

void main() {
	vec4 diffuse = texture(texture0, fragTexCoord);
	vec4 light = texture(texture1, fragTexCoord2);

	vec3 pl_add = vec3(0.0);
	for(int i = 0; i < MAX_PL; i++) {
		if(pl_enabled[i] != 1.0)
			continue;

		vec3 light_dir = normalize(pl_position[i] - fragWorldPos);
		float dist = distance(pl_position[i], fragWorldPos); 
		float attenuation = 1.0 - smoothstep(0.0, pl_radius[i], dist*1.5);
		float dot = max(dot(fragNormal, light_dir), 0.0);
		pl_add += pl_color[i] * attenuation;
	}
		
	finalColor = (diffuse * max(light, vec4(pl_add, 1.0)) * 2.0); 
	finalColor.a = diffuse.a;
}
