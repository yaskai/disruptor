#include "../include/num_redefs.h"

#ifndef KBSP_H_
#define KBSP_H_

#define BSP_VERSION 29
#define BSP_LUMPS 	15

enum LUMP_TYPES {
	LUMP_PLANES 		= 1,
	LUMP_CLIPNODES 		= 9,
	LUMP_LEAFS 			= 10,
};

typedef struct {
	i32 file_offset;
	i32 file_size;

} Bsp_Lump;

typedef struct {
	i32 version;
	Bsp_Lump lumps[BSP_LUMPS];

} Bsp_Header;

typedef struct {
	float normal[3];
	float dist;
	i32 type;

} Bsp_Plane;

typedef struct {
	u32 planenum;
	i16 front;
	i16 back;

} Bsp_ClipNode;

typedef struct {
	Bsp_Plane *planes;
	Bsp_ClipNode *clipnodes;

	u32 num_planes;
	u32 num_clipnodes;

} Bsp_Data;

Bsp_Data LoadBsp(char *path, bool print_output);

#endif

