#include "raylib.h"

#ifndef ANIM_H_
#define ANIM_H_

typedef struct {
	Transform *bone_trs;
	int num_bones;

	int curr_frame;
	int anim_id;

} AnimState;

AnimState anim_Init(Model model); 
void anim_Close(AnimState *anim_state);

void anim_Update(AnimState *anim_state, ModelAnimation *anims);
void anim_Apply(AnimState *anim_state, Model *model, ModelAnimation *anims);

#endif
