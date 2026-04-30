#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "kbsp.h"
#include "../include/log_message.h"
#include "lit.h"
#include "map.h"
#include "ent.h"

int g_loc_count;
int g_loc_position;
int g_loc_color;
int g_loc_radius;
int g_loc_enable;

MapSection *lh_ptr_sect;
void lh_SetSectPointer(MapSection *sect) { lh_ptr_sect = sect; }

EntityHandler *lh_ptr_ent_handler;
void lh_SetEntHandlerPtr(EntityHandler *handler) { lh_ptr_ent_handler = handler; }

Bsp_Data *lh_bsp_ptr;
void lh_SetBspPtr(Bsp_Data *bsp) { 
	lh_bsp_ptr = bsp;  
}

void lh_SetShaderLocs(Bsp_Data *bsp) {
	g_loc_count 	= GetShaderLocation(bsp->lm_shader, "pl_count");	
	g_loc_enable 	= GetShaderLocation(bsp->lm_shader, "pl_enabled");
	g_loc_position 	= GetShaderLocation(bsp->lm_shader, "pl_position");
	g_loc_color 	= GetShaderLocation(bsp->lm_shader, "pl_color");
	g_loc_radius   	= GetShaderLocation(bsp->lm_shader, "pl_radius");
}

LightHandler *lh_ptr_self = NULL;

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
	//ExportImage(img, "litmap.png");
	UnloadImage(img);

	return lm;
}

Color lit_SampleLightGrid(Bsp_Data *bsp, Vector3 world_pos) {
	lm_OctreeHeader *header = &bsp->lm_oct_header;

	// Translate world position to grid indices
	int gx = (int)roundf((world_pos.x - header->grid_mins[0]) / header->grid_ext[0]); 
	int gy = (int)roundf((world_pos.y - header->grid_mins[1]) / header->grid_ext[1]); 
	int gz = (int)roundf((world_pos.z - header->grid_mins[2]) / header->grid_ext[2]); 

	// Check bounds
	if(gx < 0 || gy < 0 || gz < 0)
		return WHITE;

	if(gx >= header->grid_size[0] || gy >= header->grid_size[1] || gz >= header->grid_size[2])
		return WHITE;

	// Walk tree
	u32 node_id = header->root;
	while(1) {
		if(node_id & LG_FLAG_OCCLUDE) {
			return BLACK;
		}

		if(node_id & LG_FLAG_LEAF) {
			u32 leaf_id = node_id & ~LG_FLAG_MASK;
			u8 *ptr = bsp->lm_oct_raw + bsp->oct_leaf_offsets[leaf_id];

			int mins[3], size[3];
			memcpy(mins, ptr, 12); ptr += 12;
			memcpy(size, ptr, 12); ptr += 12;

			// Get local position inside leaf
			int lx = gx - mins[0];
			int ly = gy - mins[1];
			int lz = gz - mins[2];
			int id = (size[0] * size[1] * lz) + (size[0] * ly) + lx;
			
			// Skip to sample id
			for(int i = 0; i < id; i++) {
				u8 used = *ptr++;
				if(used == 0xFF) continue;
				ptr += used * 4;
			}

			u8 used = *ptr++;
			if(used == 0xFF || !used)
				return BLACK;

			// Return first style's color
			ptr++;		// Skip style byte
			return (Color) { ptr[0], ptr[1], ptr[2], 255 };
		}

		// Node found
		lm_OctreeNode *node = &bsp->lm_oct_nodes[node_id];
		int sg_x = (gx >= node->x) ? 1 : 0;
		int sg_y = (gy >= node->y) ? 1 : 0;
		int sg_z = (gz >= node->z) ? 1 : 0;

		int child_id = (4 * sg_x) + (2 * sg_y) + sg_z;
		node_id = node->children[child_id];
	}

	return WHITE;
}

void InitPointLights(LightHandler *lh) {
	lh_ptr_self = lh;	
}

void ManagePointLights(Bsp_Data *bsp, EntityHandler *ent_handler, float dt) {
	int enabled[MAX_POINT_LIGHTS] = {0};
	Vector3 position[MAX_POINT_LIGHTS];
	Vector3 color[MAX_POINT_LIGHTS];
	float radius[MAX_POINT_LIGHTS];

	for(int i = 0; i < MAX_POINT_LIGHTS; i++) {
		PointLight *pl = &lh_ptr_self->point_lights[i];

		if(pl->active) {
			pl->timer -= dt;

			if(pl->timer <= 0.0f)
				pl->active = 0;
		}

		enabled[i] = pl->active ? 1 : 0;
		position[i] = pl->position;
		color[i] = ColorQuantized(pl->color);
		radius[i] = pl->radius;
	}

	SetShaderValueV(bsp->lm_shader, g_loc_enable, enabled, SHADER_UNIFORM_INT, MAX_POINT_LIGHTS);
	SetShaderValueV(bsp->lm_shader, g_loc_position, position, SHADER_UNIFORM_VEC3, MAX_POINT_LIGHTS);
	SetShaderValueV(bsp->lm_shader, g_loc_color, color, SHADER_UNIFORM_VEC3, MAX_POINT_LIGHTS);
	SetShaderValueV(bsp->lm_shader, g_loc_radius, radius, SHADER_UNIFORM_FLOAT, MAX_POINT_LIGHTS);

	SetShaderValueV(ent_handler->ent_shader, ent_handler->ent_shader_locs.pl_locs[LC_PL_ENABLED], enabled, SHADER_UNIFORM_INT, MAX_POINT_LIGHTS);
	SetShaderValueV(ent_handler->ent_shader, ent_handler->ent_shader_locs.pl_locs[LC_PL_POSITION], position, SHADER_UNIFORM_VEC3, MAX_POINT_LIGHTS);
	SetShaderValueV(ent_handler->ent_shader, ent_handler->ent_shader_locs.pl_locs[LC_PL_COLOR], color, SHADER_UNIFORM_VEC3, MAX_POINT_LIGHTS);
	SetShaderValueV(ent_handler->ent_shader, ent_handler->ent_shader_locs.pl_locs[LC_PL_RADIUS], radius, SHADER_UNIFORM_FLOAT, MAX_POINT_LIGHTS);
}

void AddPointlight(PointLight point_light) {
	u8 id = 0;
	for(u8 i = 0; i < MAX_POINT_LIGHTS; i++) {
		PointLight *pl = &lh_ptr_self->point_lights[i];

		if(pl->active)
			continue;

		id = i;
		break;
	}

	point_light.active = 1;
	lh_ptr_self->point_lights[id] = point_light;

	PointLight *pl = &lh_ptr_self->point_lights[id];
}

Vector3 ColorQuantized(Color color) {
	return (Vector3) { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f };
}

