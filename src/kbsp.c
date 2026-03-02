#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/log_message.h"
#include "kbsp.h"

Bsp_Data LoadBsp(char *path, bool print_output) {
	Bsp_Data data = (Bsp_Data) {0};

	FILE *pF = fopen(path, "rb");

	if(!pF) {
		MessageError("ERROR: Could not load file ", path);
		return data;
	}

	Bsp_Header header = {0};
	fread(&header, sizeof(header), 1, pF);

	if(header.version != BSP_VERSION) {
		MessageError("ERROR: BSP version mismatch", NULL);
		return data;
	}

	printf("%d\n", header.version);

	// ---------------------------------------------------------------------------------------
	// Planes
	Bsp_Lump planes_lump = header.lumps[LUMP_PLANES];
	if(planes_lump.file_size < sizeof(Bsp_Lump)) {
		MessageError("ERROR: Lump size mismatch", NULL);
		return data;
	}

	fseek(pF, planes_lump.file_offset, SEEK_SET);
	i32 plane_count = planes_lump.file_size / sizeof(Bsp_Plane);  
	Bsp_Plane planes[plane_count];
	fread(&planes, sizeof(planes), 1, pF);
	if(print_output) {
		for(i32 i = 0; i < plane_count; i++) {
			Bsp_Plane *p = &planes[i];
			printf("{ %f, %f, %f }\n", p->normal[0], p->normal[1], p->normal[2]);
		}
	}

	data.num_planes = plane_count;
	data.planes = malloc(sizeof(Bsp_Plane) * data.num_planes);
	memcpy(data.planes, planes, sizeof(planes));

	// ---------------------------------------------------------------------------------------
	// Clip nodes
	Bsp_Lump clipnodes_lump = header.lumps[LUMP_CLIPNODES];
	if(clipnodes_lump.file_size < sizeof(Bsp_Lump)) {
		MessageError("ERROR: Lump size mismatch", NULL);
		return data;
	}

	fseek(pF, clipnodes_lump.file_offset, SEEK_SET);
	i32 clipnode_count = clipnodes_lump.file_size / sizeof(Bsp_ClipNode);
	Bsp_ClipNode clipnodes[clipnode_count];
	fread(&clipnodes, sizeof(clipnodes), 1, pF);
	if(print_output) {
		for(i32 i = 0; i < clipnode_count; i++) {
			Bsp_ClipNode *node = &clipnodes[i];
			printf("planenum: %d\n", node->planenum);
			printf("front: %d\n", node->front);
			printf("back: %d\n", node->back);
		}
	}

	data.num_clipnodes = clipnode_count;
	data.clipnodes = malloc(sizeof(Bsp_ClipNode) * data.num_clipnodes);
	memcpy(data.clipnodes, clipnodes, sizeof(clipnodes));

	fclose(pF);
	
	return data;
}

