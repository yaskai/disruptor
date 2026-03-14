#include <stdlib.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "kbsp.h"
#include "../include/log_message.h"

Lightmap BuildLightmap(Bsp_Data *bsp, char *path) {
	Lightmap lm = (Lightmap) {0};

	char header[4] = {0};
	int version; 
	int len = GetFileLength(path) - 8;

	FILE *pf = fopen(path, "rb");
	if(!pf) {
		MessageError("ERROR: Could not load .lit", path);
		return lm;
	}

	fread(header, 4, 1, pf);
	printf("HEADER: %s\n", header);

	fread(&version, 4, 1, pf);
	printf("VERSION: %d\n", version);

	u8 *data = calloc(len, 1);
	fread(data, len, 1, pf);

	lm.uvs = calloc(bsp->num_faces, sizeof(Rectangle));
	lm.uv_count = bsp->num_faces;

	// Cursor position
	int cX = 0, cY = 0;
	int atlas_width = 1024;
	int atlas_height = 0;
	int row_height = 0;

	for(int i = 0; i < bsp->num_faces; i++) {
		Bsp_Face *face = &bsp->faces[i];
		if(face->lightmap < 0)
			continue;

		Vector2 size = Bsp_FaceLightmapSize(bsp, face); 

		if(cX + size.x > atlas_width) {
			cX = 0;
			cY += row_height;
			row_height = 0;
		}

		lm.uvs[i].x = cX;
		lm.uvs[i].y = cY;
		lm.uvs[i].width = size.x;
		lm.uvs[i].height = size.y;

		cX += size.x;
		if(size.y > row_height) 
			row_height = size.y;
	}

	atlas_height = cY + row_height;
	int powtwo = 1;
	while(powtwo < atlas_height)
		powtwo = powtwo << 1;

	atlas_height = powtwo;

	u8 *px = calloc(atlas_width * atlas_height * 4, 1);

	for(int i = 0; i < bsp->num_faces; i++) {
		Bsp_Face *face = &bsp->faces[i];
		if(face->lightmap < 0)
			continue;

		u8 *src_face_ptr = data + (face->lightmap * 3);

		int w = lm.uvs[i].width, h = lm.uvs[i].height;
		int ax = lm.uvs[i].x, ay = lm.uvs[i].y;

		/*
		u8 *src_px = data + (face->lightmap) * 3;

		for(int y = 0; y < h; y++) {
			for(int x = 0; x < w; x++) {
				int src_id = (y * w + x) * 3;
				int dst_id = ((ay + y) * atlas_width + (ax + x)) * 4;

				px[dst_id+0] = src_px[src_id+0];	// R
				px[dst_id+1] = src_px[src_id+1];	// G
				px[dst_id+2] = src_px[src_id+2];	// B
				px[dst_id+3] = 255;					// A
			}
		}
		*/
		Vector2 size = Bsp_FaceLightmapSize(bsp, face);

		for(int y = 0; y < h; y++) {
			u8 *src_row_ptr = src_face_ptr + (y * (int)size.x * 3); 

			for(int x = 0; x < w; x++) {
				int dst_id = ((ay + y) * atlas_width + (ax + x)) * 4;

				px[dst_id+0] = src_row_ptr[x * 3 + 0];	// R
				px[dst_id+1] = src_row_ptr[x * 3 + 1];	// G
				px[dst_id+2] = src_row_ptr[x * 3 + 2];	// B
				px[dst_id+3] = 255;						// A
			}
		}
	}

	Image img = {
		.data = px,
		.width = atlas_width,
		.height = atlas_height,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
		.mipmaps = 1
	};

	lm.tex = LoadTextureFromImage(img);
	ExportImage(img, "litmap.png");
	UnloadImage(img);
	free(data);

	return lm;
}

Vector2 Bsp_FaceLightmapSize(Bsp_Data *bsp, Bsp_Face *face) {
	Bsp_Surface *surface = &bsp->surfaces[face->texinfo];

	float min_u = FLT_MAX, min_v = FLT_MAX;
	float max_u = -FLT_MAX, max_v = -FLT_MAX;

	for(int i = 0; i < face->edge_count; i++) {
		i32 ledge = bsp->ledges[face->first_edge + i];
		Vector3 vert = (ledge >= 0) ? bsp->verts[bsp->edges[ledge].v[0]] : bsp->verts[bsp->edges[-ledge].v[1]];
		
		float u = (Vector3DotProduct(vert, surface->vector_s) + surface->dist_s) / Vector3Length(surface->vector_s);
		float v = (Vector3DotProduct(vert, surface->vector_t) + surface->dist_t) / Vector3Length(surface->vector_t);

		if(u < min_u)
			min_u = u;

		if(v < min_v)
			min_v = v;
		
		if(u > max_u)
			max_u = u;

		if(v > max_v)
			max_v = v;
	}
	
	return (Vector2) {
		.x = floorf(max_u / 16) - floorf(min_u / 16) + 1, 	// U
		.y = floorf(max_v / 16) - floorf(min_v / 16) + 1	// V
	};
}

FaceLightmapInfo GetFaceLightmapInfo(Bsp_Data *bsp, Bsp_Face *face) {
	Bsp_Surface *surface = &bsp->surfaces[face->texinfo];

	FaceLightmapInfo info = (FaceLightmapInfo) {
		.min_u = FLT_MAX, .min_v = FLT_MAX,
		.max_u = -FLT_MAX, .max_v = -FLT_MAX
	};

	for(int i = 0; i < face->edge_count; i++) {
		i32 ledge = bsp->ledges[face->first_edge + i];
		Vector3 vert = (ledge >= 0) ? bsp->verts[bsp->edges[ledge].v[0]] : bsp->verts[bsp->edges[-ledge].v[1]];
		
		float u = (Vector3DotProduct(vert, surface->vector_s) + surface->dist_s) / Vector3Length(surface->vector_s);
		float v = (Vector3DotProduct(vert, surface->vector_t) + surface->dist_t) / Vector3Length(surface->vector_t);

		if(u < info.min_u)
			info.min_u = u;

		if(v < info.min_v)
			info.min_v = v;
		
		if(u > info.max_u)
			info.max_u = u;

		if(v > info.max_v)
			info.max_v = v;
	}

	info.w = (info.max_u / 16) - floorf(info.min_u / 16) + 1;
	info.h = (info.max_v / 16) - (info.min_v / 16) + 1;

	return info;
}
