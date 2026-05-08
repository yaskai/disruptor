#include "../include/num_redefs.h"
#include "raylib.h"
#include "nav.h"

#ifndef AI_H_
#define AI_H_

#define AI_TICK_RATE 2.0f

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
#define AI_INPUT_DEST_REACHED	0x0100
#define AI_INPUT_TARG_DEAD		0x0200
#define AI_INPUT_CLIP_EMPTY		0x0400
// *** 

enum ANIM_STATES : u8 {
	STATE_IDLE		= 0,
	STATE_MOVE		= 1,	
	STATE_ATTACK    = 2,
	STATE_RELOAD	= 3,
	STATE_DEAD		= 4,
	STATE_DISABLED  = 5,
	STATE_STUNNED	= 6
};

enum SCHED_TYPES : u8 {
	SCHED_IDLE							=  0,
	SCHED_DEAD							=  1,
	SCHED_SENTRY_IDLE					=  2,
	SCHED_SENTRY						=  3,
	SCHED_MAINTAINER_IDLE				=  4,
	SCHED_PATROL						=  5,
	SCHED_MAINTAINER_ATTACK				=  6,
	SCHED_CHASE_PLAYER					=  7,
	SCHED_FIX_FRIEND_A					=  8,
	SCHED_FIX_FRIEND_B					=  9,
	SCHED_STUN							= 10,
	SCHED_GOTO_COVER					= 11,
	SCHED_REGULATOR_ATTACK				= 12,
	SCHED_REGULATOR_IDLE				= 13,
	SCHED_REGULATOR_RELOAD				= 14,
	SCHED_REGULATOR_FIND_FIRE_POS		= 15,
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
	TASK_FIND_POS,
	TASK_MEELEE_ATTACK,
	TASK_MAKE_CHASE_PATH,
	TASK_GOTO_ENT,
	TASK_RESTORE_SCHED,
	TASK_FIND_COVER,
	TASK_FIND_FIRING_POS,
};

typedef struct {
	u32 sched_id, prev_sched;
	u32 interrupt_mask;

	u8 curr_task;

} Ai_SchedState;

typedef struct {
	float timers[8];		// Task timer array
	u8 tasks[8];			// Task ID array
	u8 num_tasks;			// Number of tasks

	u32 interrupt_mask;		// Which inputs end the schedule
	u32 fail_mask;			// Which inputs count as failure 

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
	bool use_path;

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

#define MAX_ALERT_SPHERES	16
#define ALERT_SPHERE_ACTIVE 0x01
typedef struct {
	Vector3 position;
	float radius;

	u16 graph_id;

	u8 flags;

} AlertSphere;

#endif
