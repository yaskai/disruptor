#include "ai.h"

static Ai_SchedDef sched_defs[] = {
	[SCHED_SENTRY_IDLE] = {
		.tasks = { TASK_WAIT_TIME },
		.num_tasks = 1,
		.interrupt_mask = ( AI_INPUT_SEE_PLAYER | AI_INPUT_HEAR_PLAYER ),
		.interrupt_sched = SCHED_SENTRY,
		.fail_sched = SCHED_SENTRY_IDLE,
		.next_sched = SCHED_SENTRY_IDLE
	},

	[SCHED_SENTRY] = {
		.tasks = { TASK_LOOK_AT_ENTITY, TASK_FIRE_WEAPON, TASK_RELOAD_WEAPON, TASK_WAIT_TIME },
		.num_tasks = 4,
		.interrupt_mask = (0),
		.interrupt_sched = SCHED_SENTRY_IDLE,
		.fail_sched = SCHED_SENTRY_IDLE,
		.next_sched = SCHED_SENTRY_IDLE
	},

	[SCHED_PATROL] = {
		.tasks = { TASK_MAKE_PATROL_PATH, TASK_FACE_DIR, TASK_GOTO_POINT, TASK_STOP_MOVE, TASK_WAIT_TIME },
		.num_tasks = 5,
		.interrupt_mask = ( AI_INPUT_SEE_PLAYER | AI_INPUT_HEAR_PLAYER ),
		.interrupt_sched = SCHED_CHASE_PLAYER,
		.fail_sched = SCHED_PATROL,
		.next_sched = SCHED_MAINTAINER_IDLE
	},

	[SCHED_MAINTAINER_IDLE] = {
		.tasks = { TASK_STOP_MOVE, TASK_WAIT_TIME },
		.num_tasks = 2,
		.interrupt_mask = ( AI_INPUT_SEE_PLAYER | AI_INPUT_HEAR_PLAYER ),
		.interrupt_sched = SCHED_CHASE_PLAYER,
		.fail_sched = SCHED_MAINTAINER_IDLE,
		.next_sched = SCHED_MAINTAINER_IDLE,
	},

	[SCHED_CHASE_PLAYER] = {
		//.tasks = { TASK_FIND_POS, TASK_GOTO_POS, TASK_STOP_MOVE }, 
		.tasks = { TASK_MAKE_CHASE_PATH, TASK_GOTO_POS, TASK_STOP_MOVE },
		.num_tasks = 3,
		.interrupt_mask = ( AI_INPUT_MEELEE_RANGE ),
		.interrupt_sched = SCHED_MAINTAINER_ATTACK,
		.fail_sched = SCHED_MAINTAINER_IDLE,
		.next_sched = SCHED_CHASE_PLAYER
	},

	[SCHED_MAINTAINER_ATTACK] = {
		.tasks = { TASK_MEELEE_ATTACK, TASK_WAIT_TIME },
		.num_tasks = 2,
		.interrupt_mask = ( 0 ),
		.interrupt_sched = SCHED_MAINTAINER_IDLE,
		.fail_sched = SCHED_MAINTAINER_IDLE,
		.next_sched = SCHED_CHASE_PLAYER
	},

	[SCHED_FIX_FRIEND] = {
		.tasks = { TASK_MAKE_CHASE_PATH, TASK_GOTO_POS, TASK_STOP_MOVE, TASK_DO_FIX },
		.num_tasks = 4,
		.interrupt_mask = ( 0 ), 
		.interrupt_sched = SCHED_MAINTAINER_IDLE,
		.fail_sched = SCHED_MAINTAINER_IDLE, 
		.next_sched = SCHED_PATROL
	},
};

