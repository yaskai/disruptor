#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "nav.h"
#include "geo.h"

#define BREAK_RADIUS (8.0f*8.0f)
int FindClosestNavNode(Vector3 ent_position, MapSection *sect) {
	int id = -1;

	float closest_dist = FLT_MAX;

	for(u16 i = 0; i < sect->base_navgraph.node_count; i++) {
		NavNode *node = &sect->base_navgraph.nodes[i];

		float dist = Vector3DistanceSqr(node->position, ent_position);	
		if(dist > closest_dist) 
			continue;

		if(!CheckCollisionSpheres(ent_position, 32, node->position, 32))
			continue;

		closest_dist = dist;
		id = node->id;

		if(dist < BREAK_RADIUS)
			break;
	}

	return id;
}

void AiNavSetup(EntityHandler *handler, MapSection *sect) {
	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];	

		comp_Ai *ai = &ent->comp_ai;
		if(!ai->component_valid) continue;

		comp_Transform *ct = &ent->comp_transform;

		for(u16 j = 0; j < sect->navgraph_count; j++) {
			NavGraph *graph = &sect->navgraphs[j];

			int closest_node = FindClosestNavNodeInGraph(ct->position, graph);
			if(closest_node > -1) {
				ai->navgraph_id = j;
				ai->curr_navnode_id = closest_node;

				NavNode *node = &graph->nodes[closest_node];
				//ct->position.x = node->position.x;
				//ct->position.y = node->position.y;

				break;
			}
		}
	}
}

#define NULL_NODE -1
bool MakeNavPath(Entity *ent, NavGraph *graph, i16 target_id) {
	if(target_id == -1)	
		return false;

	bool dest_found = false;

	comp_Ai *ai = &ent->comp_ai;

	NavPath *path = &ai->task_state.path;

	path->count = 0;
	ai->task_state.is_init = false;

	i16 start = ai->curr_navnode_id;
	u16 node_count = graph->node_count;

	float g_cost[node_count], f_cost[node_count];
	bool open[node_count], closed[node_count];
	i16 parent[node_count];

	for(i16 i = 0; i < node_count; i++) {
		g_cost[i] = FLT_MAX, f_cost[i] = FLT_MAX;
		open[i] = false, closed[i] = false;
		parent[i] = NULL_NODE;
	}

	g_cost[start] = 0.0f;
	f_cost[start] = Vector3Distance(graph->nodes[start].position, graph->nodes[target_id].position);

	open[start] = true;

	parent[start] = NULL_NODE;

	while(true) {
		i16 curr = NULL_NODE;
		float best = FLT_MAX;		

		for(u16 i = 0; i < node_count; i++) {
			if(open[i] && f_cost[i] < best) {
				best = f_cost[i];
				curr = i;
			}
		}

		if(curr == NULL_NODE) {
			ai->task_state.is_init = false;
			path->count = 0;
			path->targ = ai->curr_navnode_id;
			return false;
		}

		if(curr == target_id)
			break;

		open[curr] = false;
		closed[curr] = true;

		u8 adj_count = 0;
		u16 adj[MAX_EDGES_PER_NODE] = { 0 };
		GetConnectedNodes(&graph->nodes[curr], adj, &adj_count, graph);

		for(u8 j = 0; j < adj_count; j++) {
			i16 neighbour = adj[j];

			if(closed[neighbour])
				continue;

			float step_cost = Vector3Distance(graph->nodes[curr].position, graph->nodes[neighbour].position);
			float tentative = g_cost[curr] + step_cost;

			if(!open[neighbour] || tentative < g_cost[neighbour]) {
				parent[neighbour] = curr;
				g_cost[neighbour] = tentative;

				f_cost[neighbour] = tentative + Vector3Distance(graph->nodes[neighbour].position, graph->nodes[target_id].position);
				open[neighbour] = true;
			}
		}
	}

	path->count = 0;
	i16 curr = target_id;

	bool reached_start = false;
	i16 test = target_id;
	while(test != NULL_NODE) {
		if(test == start) {
			reached_start = true;
			break;
		}
		test = parent[test];
	}

	if(!reached_start) {
		//printf("did not reach start\n");
		dest_found = false;
		return dest_found;
	}

	while(curr != NULL_NODE && path->count < MAX_PATH_NODES - 1) {
		path->nodes[path->count++] = curr;
		curr = parent[curr];
	}

	for(u16 i = 0; i < (path->count >> 1); i++) {
		u16 temp = path->nodes[i];
		path->nodes[i] = path->nodes[path->count - 1 - i];
		path->nodes[path->count - 1 - i] = temp;
	}
	
	path->curr = 0;
	ai->task_state.is_init = true;
	ai->curr_navnode_id = path->nodes[0];

	dest_found = true;
	return dest_found;
}

bool AiMoveToNode(Entity *ent, NavGraph *graph, u16 path_id) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	Ai_TaskState *task = &ai->task_state;
	NavPath *path = &task->path;

	if(path_id >= path->count) {
		//printf("move not possible, path max overflow\n");
		//ct->velocity = Vector3Zero();
		ai->wish_dir = Vector3Zero();
		return false;
	}

	if(path_id >= graph->node_count) {
		//printf("move not possible, graph count overflow\n");
		//ct->velocity = Vector3Zero();
		ai->wish_dir = Vector3Zero();
		return false;
	}

	Vector3 point = graph->nodes[path->nodes[path_id]].position;
	ai->targ_data.position = point;
	
	Vector3 dir = (Vector3Subtract(point, ct->position));
	dir.z = 0;
	dir = Vector3Normalize(dir);
	//ct->forward = dir;
	ct->targ_look = dir;

	float angle = atan2f(ct->forward.x, ct->forward.y);
	//ent->model.transform = MatrixRotateZ(angle + 90 * DEG2RAD);
	ai->wish_dir = Vector3Add(Vector3Scale(ai->wish_dir, 0.1f), dir); 
	ai->wish_dir = Vector3Normalize(ai->wish_dir);
 
	ai->curr_navnode_id = path->nodes[path_id];

	ai->state = STATE_MOVE;

	return true;
}

