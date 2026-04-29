#version 330

in vec2 fragTexCoord;
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D texture0;

uniform vec3 light_pos[3];
uniform vec3 light_clr[3];
uniform vec3 view_pos;

out vec4 finalColor;

void main() {
    vec4 diffuse = texture(texture0, fragTexCoord);
    
    // Simple normal-based vertical gradient using lightgrid samples
    vec3 normal = normalize(fragNormal);
    
    vec3 ambient;
	ambient = light_clr[1];
	ambient = clamp(ambient, 0.25, 1.0);

	if(diffuse.x >= 1.0)
		ambient = vec3(1.0);

    finalColor = vec4(diffuse.rgb * ambient, 1.0);
}
