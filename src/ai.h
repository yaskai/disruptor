#include "../include/num_redefs.h"
#include "raylib.h"
#include "nav.h"

#ifndef AI_H_
#define AI_H_

#define AI_TICK_RATE 11.0f

// ** Input mask definitions ** //
//
#define AI_INPUT_SEE_PLAYER		0x0001
#define AI_INPUT_SEE_PET		0x0002	
#define AI_INPUT_SEE_GLITCHED	0x0004
#define AI_INPUT_SELF_GLITCHED	0x0008
#define AI_INPUT_TAKE_DAMAGE	0x0010
#define AI_INPUT_LOST_PLAYER	0x0020
#define AI_INPUT_HEAR_PLAYER	0x0040
#define AI_INPUT_MEELEE_RANGE	0x0080
// *** 

enum ANIM_STATES : u8 {
	STATE_IDLE,
	STATE_MOVE,	
	STATE_ATTACK,
	STATE_RELOAD,
	STATE_DEAD,
	STATE_DISABLED
};

enum SCHED_TYPES : u8 {
	SCHED_IDLE,
	SCHED_DEAD,
	SCHED_SENTRY_IDLE,
	SCHED_SENTRY,
	SCHED_MAINTAINER_IDLE,
	SCHED_PATROL,
	SCHED_MAINTAINER_ATTACK,
	SCHED_CHASE_PLAYER,
};

enum TASK_TYPES : u8 {
	TASK_GOTO_POINT,	
	TASK_FIRE_WEAPON,
	TASK_RELOAD_WEAPON,
	TASK_WAIT_TIME,
	TASK_FACE_DIR,
	TASK_FIND_POINT,
	TASK_MAKE_PATROL_PATH,
	TASK_LOOK_AT_ENTITY,
	TASK_LOOK_AROUND,
	TASK_DO_FIX,
	TASK_THROW_PROJECTILE,
	TASK_STOP_MOVE,
	TASK_GOTO_POS,
	TASK_MEELEE_ATTACK,
};

typedef struct {
	u32 sched_id;
	u32 interrupt_mask;

	u8 curr_task;

} Ai_SchedState;

typedef struct {
	u8 tasks[8];			// Task ID array
	u8 num_tasks;			// Number of tasks

	u32 interrupt_mask;		// Which inputs end the schedule
	u8 interrupt_sched;		// What schedule to switch to upon interrupt

	u8 fail_sched;			// What schedule to switch to upon failure
	u8 next_sched;			// What schedule to switch to upon completion

} Ai_SchedDef;

typedef struct {
	Vector3 position;
	Vector3 known_position;
	i16 ent_id;

} Ai_TargetData;

typedef struct {
	Vector3 move_dest;
	NavPath path;

	float timer;

	u8 task_id;

	bool is_init;
	bool complete;

} Ai_TaskState;

typedef struct {
	Ai_SchedState sched_state;
	Ai_TaskState task_state;
	Ai_TargetData targ_data;

	Vector3 wish_dir;
	Vector3 move_input;

	float sight_cone;
	float speed;

	u32 input_mask;

	int curr_navnode_id;

	float hear_distance;

	float disrupt_timer;

	u16 self_ent_id;

	i16 navgraph_id;

	u8 state;

	bool component_valid;

} comp_Ai;

void AiSetSchedule(comp_Ai *ai, u8 sched_id);

#endif
