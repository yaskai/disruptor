#version 330

in vec3 vertexPosition; 	// 3D world position of vertex
in vec2 vertexTexCoord; 	// UV texture coordinates
in vec2 vertexTexCoord2;	// UV lightmap coordinates

uniform mat4 mvp;			// Model view projection matrix 
uniform mat4 matModel;

out vec2 fragTexCoord;		// Pass texture coordinates to fragment shader
out vec2 fragTexCoord2;		// Pass texture coordinates to fragment shader

out vec3 fragPosition;
out vec3 fragWorldPos;
out vec3 fragNormal;

void main() {
	fragTexCoord = vertexTexCoord;	
	fragTexCoord2 = vertexTexCoord2;

	fragPosition = vec3(mvp * vec4(vertexPosition, 1.0));
	fragWorldPos = vec3(matModel * vec4(vertexPosition, 1.0));
	
	//gl_Position = fragPosition;
	
	gl_Position = mvp * vec4(vertexPosition, 1.0);
}

