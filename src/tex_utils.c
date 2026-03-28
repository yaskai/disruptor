#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "tex_utils.h"

void GetTextureAxes(Vector3 normal, Vector3 *axis_s, Vector3 *axis_t) {
	Vector3 base[] = {
        {0,0,1},  {1,0,0},  {0,-1,0},   // floor
        {0,0,-1}, {1,0,0},  {0,-1,0},   // ceiling
        {1,0,0},  {0,1,0},  {0,0,-1},   // west
        {-1,0,0}, {0,1,0},  {0,0,-1},   // east
        {0,1,0},  {1,0,0},  {0,0,-1},   // south
        {0,-1,0}, {1,0,0},  {0,0,-1},   // north
	};

	float best_score = -FLT_MAX;
	int best_axis = 0;

	for(int i = 0; i < 6; i++) {
		float dot = Vector3DotProduct(normal, base[i*3]);
		if(dot > best_score) {
			best_score = dot;
			best_axis = i;
		}
	}

	*axis_s = base[best_axis*3+1];
	*axis_t = base[best_axis*3+2];
} 
