#include <stdlib.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "kbsp.h"
#include "../include/log_message.h"

Lightmap BuildLightmap(Bsp_Data *bsp) {
	Message("BuildLightmap()", ANSI_BLUE);

	Lightmap lm = (Lightmap) {0};
	
	lm.uvs = calloc(bsp->num_faces, sizeof(Rectangle));
	lm.uv_count = bsp->num_faces;

	int atlas_w = 1024;
	int cX = 0, cY = 0;
	int row_h = 0;

	for(int i = 0; i < bsp->num_faces; i++) {
		Lm_Decoupled *dlm = &bsp->decouple_lm[i];

		if(dlm->w == 0 || dlm->h == 0)
			continue;

		if(cX + dlm->w > atlas_w) {
			cX = 0;
			cY += row_h;
			row_h = 0;
		}

		lm.uvs[i] = (Rectangle) { cX, cY, dlm->w, dlm->h };
		cX += dlm->w;

		if(dlm->h > row_h) 
			row_h = dlm->h;
	}

	int atlas_h = cY + row_h;

	u8 *px = calloc(atlas_w * atlas_h * 4, 1);
	for(int i = 0; i < atlas_w * atlas_h * 4; i+=4) {
		px[i+0] = 0;
		px[i+1] = 0;
		px[i+2] = 0;
		px[i+3] = 255;
	}

	for(int i = 0; i < bsp->num_faces; i++) {
		Lm_Decoupled *dlm = &bsp->decouple_lm[i];

		if(dlm->w == 0 || dlm->h == 0)
			continue;

		int w = dlm->w;
		int h = dlm->h;
		int ax = lm.uvs[i].x;		
		int ay = lm.uvs[i].y;

		u8 *src_px = bsp->lm_rgb + dlm->lm_offset * 3;

		for(int y = 0; y < h; y++) {
			for(int x = 0; x < w; x++) {
				int src_id = (y * w + x) * 3;
				int dst_id = ((ay + y) * atlas_w + (ax + x)) * 4;

				px[dst_id+0] = src_px[src_id+0]; 
				px[dst_id+1] = src_px[src_id+1]; 
				px[dst_id+2] = src_px[src_id+2]; 
				px[dst_id+3] = 255; 
			}
		}
	}

	Image img = (Image) {
		.data = px,
		.width = atlas_w,
		.height = atlas_h,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
		.mipmaps = 1
	};
	lm.tex = LoadTextureFromImage(img);
	SetTextureFilter(lm.tex, TEXTURE_FILTER_TRILINEAR);
	ExportImage(img, "litmap.png");
	UnloadImage(img);

	return lm;
}

