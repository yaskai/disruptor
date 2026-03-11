#include "../include/num_redefs.h"
#include "raylib.h"
#include "raymath.h"
#include "hash.h"

#ifndef KBSP_H_
#define KBSP_H_

#define BSP_VERSION 29
#define BSP_LUMPS 	15

enum LUMP_TYPES {
	LUMP_ENTS			= 0,
	LUMP_PLANES 		= 1,
	LUMP_MIPTEX			= 2,
	LUMP_VERTICES		= 3,
	LUMP_VIS			= 4,
	LUMP_NODES			= 5,
	LUMP_TEXINFO		= 6,
	LUMP_FACES			= 7,
	LUMP_LIGHTMAPS		= 8,
	LUMP_CLIPNODES 		= 9,
	LUMP_LEAVES			= 10,
	LUMP_LFACES			= 11,
	LUMP_EDGES			= 12,
	LUMP_L_EDGES		= 13,
	LUMP_MODELS			= 14,
};

typedef struct {
	char *key;
	char *val;

} Bsp_EntProp;

typedef struct {
	Bsp_EntProp *properties;
	u32 prop_count;

} Bsp_Ent;

// AABB
typedef struct {
    int16_t min[3];
    int16_t max[3];

} Bsp_Box32;

// Edge
typedef struct {
	u16 v[2];

} Bsp_Edge;

// Lump
typedef struct {
	i32 file_offset;
	i32 file_size;

} Bsp_Lump;

// Header
typedef struct {
	i32 version;
	Bsp_Lump lumps[BSP_LUMPS];

} Bsp_Header;

// Plane
typedef struct {
	float normal[3];
	float dist;
	i32 type;

} Bsp_Plane;

// Mip Header
typedef struct {
	i32 *offset;
	i32 numtex;

} Bsp_Mipheader;

// Mip Texture
typedef struct {
	char name[16];

	u32 width;
	u32 height;

	u32 offset1;
	u32 offset2;
	u32 offset4;
	u32 offset8;

} Bsp_Miptex;

// Surface
typedef struct {
	Vector3 vector_s;
	float dist_s;
	Vector3 vector_t;
	float dist_t;
	u32 texture_id;
	u32 animated;

} Bsp_Surface;

// Lightmap
typedef struct {
	u8 *lightmap;
	u32 num_lightmap;

} Bsp_Lightmap;

// Clip Node
typedef struct {
	u32 planenum;
	i16 children[2];

} Bsp_ClipNode;

// BSP Node
typedef struct {
	i32 planenum;
	i16 children[2];
	i16 mins[3];
	i16 maxs[3];
	u16 first_face;
	u16 num_faces;

} Bsp_Node;

// Leaf
typedef struct {
	i32 type;
	i32 visofs;
	Bsp_Box32 aabb;
	u16 first_face;
	u16 num_faces;
	u8 ambient[4];
	
} Bsp_Leaf;

// Face
typedef struct {
	u16 plane;	
	u16 side;
	i32 first_edge;
	u16 edge_count;
	u16 texinfo;
	u8 type_light;
	u8 base_light;
	u8 light[2];
	i32 lightmap;

} Bsp_Face;

typedef struct {
	u16 *faces;
	u32 num_lface;

} Bsp_LFaces;

typedef struct {
	u16 *edge;
	u32 num_ledge;

} Bsp_LEdges;

// Model
typedef struct {
	float mins[3], maxs[3];
	float origin[3];

	i32 head_nodes[4];
	i32 num_leafs;
	i32 face_id;
	i32 faces;

} Bsp_Model;

// Data
typedef struct {
	Bsp_Plane *planes;
	Bsp_Miptex *miptex;
	Vector3 *verts;
	u8 *vis;
	Bsp_Node *nodes;
	Bsp_Face *faces;
	Bsp_Surface *surfaces;
	Bsp_Lightmap lightmap;
	Bsp_Lightmap *lightmaps;
	Bsp_ClipNode *clipnodes;
	Bsp_Leaf *leaves;
	u16 *lfaces;
	Bsp_Edge *edges;
	i32 *ledges;
	Bsp_Model *models;

	u32 num_planes;
	u32 num_miptex;
	u32 num_verts;
	u32 num_vis;
	u32 num_nodes;
	u32 num_clipnodes;
	u32 num_leaves;
	u32 num_edges;
	u32 num_ledges;
	u32 num_models;
	u32 num_faces;
	u32 num_lfaces;
	u32 num_surfaces;

	Texture2D *textures;
	i32 miptex_lump_offset;

} Bsp_Data;

Bsp_Data LoadBsp(char *path, bool print_output);
void UnloadBsp(Bsp_Data *data);

void Bsp_PrintStructSizes();

typedef struct {
	Bsp_Plane *planes;
	Bsp_ClipNode *nodes;

	int first_node;
	int last_node;

	int num_planes;

} Bsp_Hull;

Bsp_Hull Bsp_BuildHull(Bsp_Data *data, int hull_index);

#define CONTENTS_EMPTY -1
#define CONTENTS_SOLID -2
int Bsp_PointContents(Bsp_Hull *hull, int num, Vector3 point);

bool Bsp_RecursiveTrace(Bsp_Hull *hull, int node_num, Vector3 point_A, Vector3 point_B, Vector3 *interesection);

typedef struct {
	Bsp_Plane plane;

	Vector3 point;
	Vector3 normal;

	float distance;
	float fraction;

	bool start_solid;
	bool all_solid;
	bool in_open;
	bool in_water;

	bool hit;

} Bsp_TraceData;

Bsp_TraceData Bsp_TraceDataEmpty();
bool Bsp_RecursiveTraceEx(Bsp_Hull *hull, int node_num, float p1_frac, float p2_frac, Vector3 p1, Vector3 p2, Bsp_TraceData *trace);

int Bsp_FindLeaf(Bsp_Data *bsp, Vector3 point);
bool Bsp_LeafVisible(Bsp_Data *bsp, int curr_leaf, int test_leaf);

Model *BspLeafToModels(Bsp_Data *bsp, Bsp_Leaf *leaf, int *out_count);

#endif
