#version 330

in vec2 fragTexCoord;
in vec3 fragPosition;
in vec3 fragNormal;
in vec3 fragWorldPos;

uniform sampler2D texture0;

uniform vec3 light_pos[3];
uniform vec3 light_clr[3];
uniform vec3 view_pos;

#define MAX_PL 16
uniform int pl_enabled[MAX_PL];
uniform vec3 pl_position[MAX_PL];
uniform vec3 pl_color[MAX_PL];
uniform float pl_radius[MAX_PL];

out vec4 finalColor;

void main() {
    vec4 diffuse = texture(texture0, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    
    vec3 ambient;
	ambient = light_clr[1];
	ambient = clamp(ambient, 0.25, 1.0);

	if(diffuse.x >= 1.0)
		ambient = vec3(1.0);

	vec3 pl_add = vec3(0.0);
	int c = 0;
	for(int i = 0; i < MAX_PL; i++) {
		if(pl_enabled[i] != 1.0)
			continue;

		vec3 light_dir = normalize(pl_position[i] - fragWorldPos);
		float dist = distance(pl_position[i], fragWorldPos); 
		float attenuation = 1.0 - smoothstep(0.0, pl_radius[i], dist*1.75);
		float ndot = max(dot(fragNormal, light_dir), 0.0);

		pl_add += pl_color[i] * attenuation * ndot;

	}

    finalColor = vec4((diffuse.rgb * ambient) + pl_add, 1.0);
	//finalColor = (diffuse * max(vec4(ambient, 1.0), vec4(pl_add, 1.0)) * 2.0); 
}
