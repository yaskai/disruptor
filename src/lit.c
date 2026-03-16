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

	// Instead of building atlas, just load the precomputed ones:
	lm.tex = LoadTexture("litmap_correct.png");
	lm.uv_count = bsp->num_faces;
	lm.uvs = calloc(bsp->num_faces, sizeof(Rectangle));

	FILE *f = fopen("litmap_uvs.bin", "rb");
	uint32_t num_faces, atlas_width;
	fread(&num_faces, 4, 1, f);
	fread(&atlas_width, 4, 1, f);

	uint16_t *uv_data = malloc(num_faces * 4 * sizeof(uint16_t));
	fread(uv_data, sizeof(uint16_t), num_faces * 4, f);
	fclose(f);

	for(int i = 0; i < (int)num_faces; i++) {
		lm.uvs[i].x      = uv_data[i*4+0];
		lm.uvs[i].y      = uv_data[i*4+1];
		lm.uvs[i].width  = uv_data[i*4+2];
		lm.uvs[i].height = uv_data[i*4+3];
	}
	free(uv_data);

	/*
	u8 *data = calloc(len, 1);
	fread(data, len, 1, pf);

	lm.uvs = calloc(bsp->num_faces, sizeof(Rectangle));
	lm.uv_count = bsp->num_faces;

	// Cursor position
	int cX = 0, cY = 0;
	int atlas_width = 512;
	int atlas_height = 0;
	int row_height = 0;

	for(int i = 0; i < bsp->num_faces; i++) {
		Bsp_Face *face = &bsp->faces[i];
		if(face->lightmap < 0)
			continue;

		Vector2 size = Bsp_FaceLightmapSize(bsp, i); 

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

		int w = lm.uvs[i].width, h = lm.uvs[i].height;
		int ax = lm.uvs[i].x, ay = lm.uvs[i].y; 

		//u8 *src_px = data + face->lightmap * 3;
		u8 *src_px = bsp->lm_rgb + (face->lightmap * 3);
		for(int y = 0; y < h; y++) {
			for(int x = 0; x < w; x++) {
				int src_id = (y * w + x) * 3;
				//int src_id = (x * h + y) * 3;
				int dst_id = ((ay + y) * atlas_width + (ax + x)) * 4;
				px[dst_id+0] = src_px[src_id+0];
				px[dst_id+1] = src_px[src_id+1];
				px[dst_id+2] = src_px[src_id+2];
				px[dst_id+3] = 255;
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
	SetTextureFilter(lm.tex, TEXTURE_FILTER_POINT);
	ExportImage(img, "litmap.png");
	UnloadImage(img);
	free(data);
	*/

	//SetShaderValueTexture(bsp->lm_shader, GetShaderLocation(bsp->lm_shader, "texture1"), lm.tex);

	return lm;
}

Vector2 Bsp_FaceLightmapSize(Bsp_Data *bsp, int face_id) {
	Bsp_Face *face = &bsp->faces[face_id];
	Bsp_Surface *surface = &bsp->surfaces[face->texinfo];

	//int lmscale = 1 << bsp->lm_shift[face_id];
	int lmscale = bsp->lm_shift[face_id];

	float min_u = FLT_MAX, min_v = FLT_MAX;
	float max_u = -FLT_MAX, max_v = -FLT_MAX;

	for(int i = 0; i < face->edge_count; i++) {
		i32 ledge = bsp->ledges[face->first_edge + i];
		Vector3 vert = (ledge >= 0) ? bsp->verts[bsp->edges[ledge].v[0]] : bsp->verts[bsp->edges[-ledge].v[1]];
		
		float u = (Vector3DotProduct(vert, surface->vector_s) + surface->dist_s);
		float v = (Vector3DotProduct(vert, surface->vector_t) + surface->dist_t);

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
		.x = floorf(max_u / lmscale) - floorf(min_u / lmscale) + 1, // U
		.y = floorf(max_v / lmscale) - floorf(min_v / lmscale) + 1	// V
	};
}

FaceLightmapInfo GetFaceLightmapInfo(Bsp_Data *bsp, int face_id) {
	Bsp_Face *face = &bsp->faces[face_id];
	Bsp_Surface *surface = &bsp->surfaces[face->texinfo];

	//int lmscale = 1 << bsp->lm_shift[face_id];
	int lmscale = bsp->lm_shift[face_id];

	FaceLightmapInfo info = (FaceLightmapInfo) {
		.min_u = FLT_MAX, .min_v = FLT_MAX,
		.max_u = -FLT_MAX, .max_v = -FLT_MAX
	};

	for(int i = 0; i < face->edge_count; i++) {
		i32 ledge = bsp->ledges[face->first_edge + i];
		Vector3 vert = (ledge >= 0) ? bsp->verts[bsp->edges[ledge].v[0]] : bsp->verts[bsp->edges[-ledge].v[1]];
		
		//float u = (Vector3DotProduct(vert, surface->vector_s) + surface->dist_s) / Vector3Length(surface->vector_s);
		//float v = (Vector3DotProduct(vert, surface->vector_t) + surface->dist_t) / Vector3Length(surface->vector_t);
		float u = (Vector3DotProduct(vert, surface->vector_s) + surface->dist_s);
		float v = (Vector3DotProduct(vert, surface->vector_t) + surface->dist_t);

		if(u < info.min_u)
			info.min_u = u;

		if(v < info.min_v)
			info.min_v = v;
		
		if(u > info.max_u)
			info.max_u = u;

		if(v > info.max_v)
			info.max_v = v;
	}

	info.w = floorf(info.max_u / lmscale) - floorf(info.min_u / lmscale) + 1;
	info.h = floorf(info.max_v / lmscale) - floorf(info.min_v / lmscale) + 1;

	return info;
}
