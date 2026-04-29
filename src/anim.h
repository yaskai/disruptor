#include "raylib.h"
#include "../include/num_redefs.h"

#ifndef ANIM_H_
#define ANIM_H_

typedef struct {
	Transform *bone_trs;
	int num_bones;

	int weap_socket;

	int curr_frame;
	int anim_id;

	float speed;
	float acc;

	u8 loop_count;

} AnimState;

AnimState anim_Init(Model model); 
void anim_Close(AnimState *anim_state);

void anim_Update(AnimState *anim_state, ModelAnimation *anims, float dt);
void anim_Apply(AnimState *anim_state, Model *model, ModelAnimation *anims);

void anim_Switch(AnimState *anim_state, int anim_id);

void anim_SocketSetup(AnimState *anim_state, Model *model, char *socket_name);

#endif
