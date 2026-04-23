#include "../include/num_redefs.h"
#include "raylib.h"
#include "raymath.h"
#include "hash.h"
#include "audioplayer.h"

#ifndef KBSP_H_
#define KBSP_H_

#define RBRUSH_NOFLAGS			0x00
#define RBRUSH_TRANSLUCENT		0x01
#define RBRUSH_FORCEFIELD		0x02
typedef struct {
	Model model;
	BoundingBox aabb;

	int id;
	u8 flags;

} RenderBrush;

typedef struct {
	RenderBrush *render_brushes;
	int *ids;

	int count;
	int cap;

} rBrushList;

#define LG_FLAG_LEAF 	(1u << 31)
#define LG_FLAG_OCCLUDE (1u << 30)
#define LG_FLAG_MASK	(LG_FLAG_LEAF | LG_FLAG_OCCLUDE)

typedef struct {
	float grid_ext[3];
	int grid_size[3];
	float grid_mins[3];
	u8 num_styles;
	u32 root;

} lm_OctreeHeader; 

typedef struct {
	int x, y, z;
	u32 children[8];
	
} lm_OctreeNode;

typedef struct {
	u8 r, g, b;
	u8 style;

} lm_OctreeSample;

typedef struct {
	lm_OctreeSample samples[4];
	u8 used;
	u8 occluded;

} lm_SampleList;

typedef struct {
	int mins[3];
	int size[3];

} lg_LeafHeader;

typedef struct {
	Texture2D tex;
	Rectangle *uvs;	
	int uv_count;

} Lightmap;

typedef struct {
	float min_u, max_u;
	float min_v, max_v;
	int w, h;

} FaceLightmapInfo;

typedef struct {
	u16 w;
	u16 h;

	u32 lm_offset;

	Vector3 vs;
	float dist_s;

	Vector3 vt;
	float dist_t;

} Lm_Decoupled;

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
	LUMP_BSPX			= 15	// Extra lump from ericw-tools
};

// Entity property
// Just a key value pair
typedef struct {
	char key[64], val[64];

} Bsp_EntProp;

typedef struct {
	Bsp_EntProp properties[64];
	int prop_count;

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
	i32 first_face;
	i32 num_faces;

} Bsp_Model;

typedef struct {
	Bsp_Plane *planes;
	Bsp_ClipNode *nodes;

	int first_node;
	int last_node;

	int num_planes;

} Bsp_Hull;

#define HULLGROUP_ACTIVE	0x01
typedef struct {
	Bsp_Hull hulls[4];
	int model_id;

	Vector3 origin;

	u8 flags;
	u8 collision_flags;
	
} Bsp_HullGroup;

// Data
typedef struct {
	char *ent_str;

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

	Lightmap lm;
	Shader lm_shader;	// Default lightmap shader

	Shader ff_shader;	// Force field shader
	int ff_locs[8];

	u8 *lm_rgb;


	Lm_Decoupled *decouple_lm;
	i32 *lm_offsets;

	u8 *lm_oct_raw;	// Raw octree bspx lump data
	u32 lm_oct_raw_size;

	lm_OctreeHeader lm_oct_header;

	lm_OctreeNode *lm_oct_nodes;
	u32 num_oct_nodes;

	u32 *oct_leaf_offsets;
	u32 num_oct_leaves;

	Bsp_HullGroup *hull_groups;

} Bsp_Data;

Bsp_Data LoadBsp(char *path, bool print_output);
void UnloadBsp(Bsp_Data *data);

void Bsp_PrintStructSizes();

Bsp_Hull Bsp_BuildHull(Bsp_Data *data, int hull_index);
Bsp_HullGroup Bsp_BuildHullGroup(Bsp_Data *data, int model_id);

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
RenderBrush *BspLeafToRenderBrushes(Bsp_Data *bsp, Bsp_Leaf *leaf, int *out_count);

Model BspModelToRenderModel(Bsp_Data *bsp, int submodel_id);

Lightmap BuildLightmap(Bsp_Data *bsp);

void BspRenderSetup(Bsp_Data *bsp);
void UpdateBspShaders(Bsp_Data *bsp);

Color lit_SampleLightGrid(Bsp_Data *bsp, Vector3 world_pos);

#endif
