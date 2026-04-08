#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "anim.h"

AnimState anim_Init(Model model) {
	AnimState anim_state = (AnimState) {0}; 

	anim_state.num_bones = model.boneCount;
	int mem_size = anim_state.num_bones * sizeof(Transform);

	anim_state.bone_trs = malloc(mem_size);
	memcpy(anim_state.bone_trs, model.bindPose, mem_size);

	anim_state.acc = 0.0f;

	return anim_state;
}

void anim_Close(AnimState *anim_state) {
	if(anim_state->bone_trs) free(anim_state->bone_trs);
	*anim_state = (AnimState) {0};
}

void anim_Update(AnimState *anim_state, ModelAnimation *anims, float dt) {
	anim_state->acc += anim_state->speed * dt;
	if(anim_state->acc < 1.0f) {
		return;
	}

	int next = anim_state->curr_frame + 1;
	if(next >= anims[anim_state->anim_id].frameCount) {
		next = 0;
		anim_state->loop_count++;
	}

	anim_state->curr_frame = next; 
	anim_state->acc = 0.0f;
}

void anim_Apply(AnimState *anim_state, Model *model, ModelAnimation *anims) {
	model->bindPose = anim_state->bone_trs;
	UpdateModelAnimation(*model, anims[anim_state->anim_id], anim_state->curr_frame);
}

void anim_Switch(AnimState *anim_state, int anim_id) {
	if(anim_id == anim_state->anim_id) {
		return;
	}

	anim_state->anim_id = anim_id;

	anim_state->curr_frame = 0;
	anim_state->acc = 0;
	
	anim_state->loop_count = 0;
}

