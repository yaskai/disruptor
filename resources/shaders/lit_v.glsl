#version 330

in vec3 vertex_position; 	// 3D world position of vertex
in vec2 vertex_texcoord; 	// UV texture coordinates
in vec2 vertex_texcoord2;	// UV lightmap coordinates

uniform mat4 mvp;			// Model view projection matrix 

out vec2 frag_texcoord;		// Pass texture coordinates to fragment shader
out vec2 frag_texcoord2;	// Pass texture coordinates to fragment shader

void main() {
	frag_texcoord = vertex_texcoord;	
	frag_texcoord2 = vertex_texcoord2;
	gl_Position = mvp * vec4(vertex_position, 1.0);
}
