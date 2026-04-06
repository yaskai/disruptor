#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef FABRIK_H_
#define FABRIK_H_

#define FABRIK_MAX_BONES 32
typedef struct {
	Vector3 points[FABRIK_MAX_BONES];
	float lengths[FABRIK_MAX_BONES - 1];

	Vector3 root_pos;
	Vector3 targ_pos;

	int count;

} FabrikChain;

#define FABRIK_MAX_ITERS 8
void FabrikSolve(FabrikChain *chain);

#endif // !FABRIK_H_
