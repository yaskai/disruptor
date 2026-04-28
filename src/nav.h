#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef NAV_H_
#define NAV_H_

#define MAX_EDGES_PER_NODE	64

#define IS_COVER	0x01
typedef struct {
	Vector3 position;

	u16 edges[MAX_EDGES_PER_NODE];
	u16 edge_count;

	u16 id;

	u8 flags;

} NavNode;

typedef struct {
	u16 id_A;
	u16 id_B;

} NavEdge;

typedef struct {
	NavNode *nodes;
	NavEdge *edges;

	BoundingBox bounds;

	u16 node_count, node_cap;
	u16 edge_count, edge_cap;

} NavGraph;

#define MAX_PATH_NODES 64
typedef struct {
	u16 nodes[MAX_PATH_NODES];
	u16 count;	

	u16 curr;
	u16 targ;

} NavPath;

#endif
