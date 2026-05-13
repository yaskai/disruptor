#include "ai.h"

static Ai_SchedDef sched_defs[] = {
	[SCHED_SENTRY_IDLE] = {
		.timers = { 0.0f },
		.tasks = { TASK_WAIT_TIME },
		.num_tasks = 1,
		.interrupt_mask = ( AI_INPUT_SEE_PLAYER | AI_INPUT_HEAR_PLAYER ),
		.fail_mask = ( 0 ),
		.interrupt_sched = SCHED_SENTRY,
		.fail_sched = SCHED_SENTRY_IDLE,
		.next_sched = SCHED_SENTRY_IDLE
	},

	[SCHED_SENTRY] = {
		.timers = { 0.0f },
		.tasks = { TASK_LOOK_AT_ENTITY, TASK_FIRE_WEAPON, TASK_RELOAD_WEAPON, TASK_WAIT_TIME },
		.num_tasks = 4,
		.interrupt_mask = (0),
		.fail_mask = ( 0 ),
		.interrupt_sched = SCHED_SENTRY_IDLE,
		.fail_sched = SCHED_SENTRY_IDLE,
		.next_sched = SCHED_SENTRY_IDLE
	},

	[SCHED_PATROL] = {
		.timers = { 0.0f },
		.tasks = { TASK_STOP_MOVE, TASK_MAKE_PATROL_PATH, TASK_FACE_DIR, TASK_GOTO_POS, TASK_STOP_MOVE, TASK_WAIT_TIME },
		.num_tasks = 6,
		.interrupt_mask = (0),
		.fail_mask = ( 0 ), 
		.interrupt_sched = SCHED_CHASE_PLAYER,
		.fail_sched = SCHED_PATROL,
		.next_sched = SCHED_PATROL,
	},

	[SCHED_MAINTAINER_IDLE] = {
		.timers = { 0.0f },
		.tasks = { TASK_STOP_MOVE, TASK_WAIT_TIME },
		.num_tasks = 2,
		.interrupt_mask = ( AI_INPUT_SEE_PLAYER | AI_INPUT_HEAR_PLAYER | AI_INPUT_TAKE_DAMAGE ),
		.fail_mask = ( 0 ), 
		.interrupt_sched = SCHED_CHASE_PLAYER,
		.fail_sched = SCHED_MAINTAINER_IDLE,
		.next_sched = SCHED_MAINTAINER_IDLE,
	},

	[SCHED_CHASE_PLAYER] = {
		.timers = { 0.0f },
		//.tasks = { TASK_FIND_POS, TASK_GOTO_POS, TASK_STOP_MOVE }, 
		.tasks = { TASK_FACE_DIR, TASK_WAIT_TIME, TASK_MAKE_CHASE_PATH, TASK_GOTO_POS, TASK_FACE_DIR, TASK_STOP_MOVE },
		.num_tasks = 6,
		.interrupt_mask = ( AI_INPUT_MEELEE_RANGE ),
		.fail_mask = ( 0 ),
		.interrupt_sched = SCHED_MAINTAINER_ATTACK,
		.fail_sched = SCHED_MAINTAINER_IDLE,
		.next_sched = SCHED_CHASE_PLAYER
	},

	[SCHED_MAINTAINER_ATTACK] = {
		.timers = { 0.0f },
		.tasks = { TASK_STOP_MOVE, TASK_MEELEE_ATTACK, TASK_WAIT_TIME },
		.num_tasks = 3,
		.interrupt_mask = ( 0 ),
		.fail_mask = ( 0 ), 
		.interrupt_sched = SCHED_MAINTAINER_IDLE,
		.fail_sched = SCHED_MAINTAINER_IDLE,
		.next_sched = SCHED_CHASE_PLAYER
	},

	[SCHED_FIX_FRIEND_A] = {
		.timers = { 0.0f },
		.tasks = { TASK_STOP_MOVE, TASK_MAKE_CHASE_PATH, TASK_GOTO_POS },
		.num_tasks = 3,
		.interrupt_mask = ( AI_INPUT_MEELEE_RANGE ), 
		.fail_mask = ( 0 ),
		.interrupt_sched = SCHED_FIX_FRIEND_B,
		.fail_sched = SCHED_MAINTAINER_IDLE, 
		.next_sched = SCHED_FIX_FRIEND_A
	},

	[SCHED_FIX_FRIEND_B] = {
		.timers = { 0.0f },
		.tasks = { TASK_STOP_MOVE, TASK_FACE_DIR, TASK_DO_FIX },
		.num_tasks = 3,
		.interrupt_mask = ( 0 ),
		.fail_mask = ( AI_INPUT_TARG_DEAD ),
		.interrupt_sched = SCHED_FIX_FRIEND_A,
		.fail_sched = SCHED_CHASE_PLAYER,
		.next_sched = SCHED_FIX_FRIEND_A,
	},

	[SCHED_STUN] = {
		.timers = { 0.0f },
		.tasks = { TASK_STOP_MOVE, TASK_WAIT_TIME, TASK_RESTORE_SCHED },
		.num_tasks = 3,
		.interrupt_mask = ( 0 ),
		.fail_mask = ( 0 ), 
		.interrupt_sched = 0,
		.fail_sched = 0,
		.next_sched = 0,
	},

	[SCHED_GOTO_COVER] = {
		.timers = { 0.0f },
		.tasks = { TASK_STOP_MOVE, TASK_FIND_COVER, TASK_GOTO_POS, TASK_STOP_MOVE },
		.num_tasks = 5, 
		.interrupt_mask = (0),
		.interrupt_sched = 0,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_IDLE, 
	},

	/*
	[SCHED_REGULATOR_ATTACK] = {
		.timers = { 0.0f },
		.tasks = { TASK_FACE_DIR, TASK_FIND_FIRING_POS, TASK_GOTO_POS, TASK_STOP_MOVE, TASK_FACE_DIR, TASK_FIRE_WEAPON },
		.num_tasks = 6,
		.interrupt_mask = (0),
		.interrupt_sched = SCHED_REGULATOR_RELOAD,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_RELOAD,
	},

	[SCHED_REGULATOR_IDLE] = {
		.timers = { 0.0f },
		.tasks = { TASK_WAIT_TIME },
		.num_tasks = 1,
		.interrupt_mask = (AI_INPUT_SEE_PLAYER | AI_INPUT_HEAR_PLAYER),
		.interrupt_sched = SCHED_REGULATOR_ATTACK,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_IDLE,
	},

	[SCHED_REGULATOR_RELOAD] = {
		.timers = { 0.0f },
		.tasks = { TASK_FIND_COVER, TASK_GOTO_POS, TASK_STOP_MOVE, TASK_RELOAD_WEAPON },
		.num_tasks = 5, 
		.interrupt_mask = (0),
		.interrupt_sched = 0,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_IDLE, 
	},
	*/

	[SCHED_REGULATOR_FIND_FIRE_POS] = {
		.timers = { 0.0f },
		.tasks = { TASK_FACE_DIR, TASK_FIND_FIRING_POS, TASK_GOTO_POS, TASK_STOP_MOVE },
		.num_tasks = 4,
		.interrupt_mask = (AI_INPUT_SEE_PLAYER),
		.interrupt_sched = SCHED_REGULATOR_ATTACK,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_ATTACK,
	},

	[SCHED_REGULATOR_IDLE] = {
		.timers = { 0.0f },
		.tasks = { TASK_WAIT_TIME },
		.num_tasks = 1,
		.interrupt_mask = (AI_INPUT_SEE_PLAYER | AI_INPUT_HEAR_PLAYER),
		.interrupt_sched = SCHED_REGULATOR_FIND_FIRE_POS,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_IDLE,
	},

	[SCHED_REGULATOR_RELOAD] = {
		.timers = { 0.0f },
		.tasks = { TASK_FIND_COVER, TASK_GOTO_POS, TASK_STOP_MOVE, TASK_RELOAD_WEAPON },
		.num_tasks = 4, 
		.interrupt_mask = (0),
		.interrupt_sched = 0,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_IDLE, 
	},

	[SCHED_REGULATOR_ATTACK] = {
		.timers = { 0.0f, 0.0f, 0.0f },
		.tasks = { TASK_FACE_DIR, TASK_FIRE_WEAPON, TASK_WAIT_TIME },
		.num_tasks = 3,
		.interrupt_mask = (0),
		.interrupt_sched = 0,
		.fail_sched = 0,
		.next_sched = SCHED_REGULATOR_RELOAD,
	}
};

