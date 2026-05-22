#include <math.h>
#include <string.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "map.h"
#include "geo.h"
#include "../include/sort.h"
#include "../include/log_message.h"
#include "config.h"
#include "rlgl.h"
#include "tex_utils.h"
#include "ent.h"

#define PLANE_EPS 0.001f

Tri *TrisFromBspModel(Bsp_Data *bsp, u16 *out_count, int model_id) {
	Bsp_Model *bsp_m = &bsp->models[model_id];

	u16 tri_count = 0;
	for(int i = 0; i < bsp_m->num_faces; i++) {
		Bsp_Face *face = &bsp->faces[bsp_m->first_face + i];
		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		Bsp_Miptex *mip = &bsp->miptex[surface->texture_id];

		tri_count += face->edge_count -2;
	}

	Tri *tris = malloc(sizeof(Tri) * tri_count);

	int tri_id = 0;
	for(int i = 0; i < bsp_m->num_faces; i++) {
		Bsp_Face *face = &bsp->faces[bsp_m->first_face + i];
		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		Bsp_Miptex *mip = &bsp->miptex[surface->texture_id];

		u8 coll_flags = 0;

		if(strcmp(mip->name, "{fence00") == 0) { 
			coll_flags |= COLL_IGNORE_BULLET;
			coll_flags |= COLL_IGNORE_VIS;
		}

		if(strcmp(mip->name, "{rustmaskwall") == 0) { 
			coll_flags |= COLL_IGNORE_BULLET;
			coll_flags |= COLL_IGNORE_VIS;
		}

		if(strcmp(mip->name, "{rustfloor") == 0) { 
			coll_flags |= COLL_IGNORE_BULLET;
			coll_flags |= COLL_IGNORE_VIS;
		}

		if(strcmp(mip->name, "{mask_wall") == 0) { 
			coll_flags |= COLL_IGNORE_BULLET;
			coll_flags |= COLL_IGNORE_VIS;
		}

		if(strcmp(mip->name, "{mask_floor") == 0) { 
			coll_flags |= COLL_IGNORE_BULLET;
			coll_flags |= COLL_IGNORE_VIS;
		}

		if(strcmp(mip->name, "{ff") == 0) {
			coll_flags |= COLL_IGNORE_BULLET;
			coll_flags |= COLL_IGNORE_VIS;
		}

		if(strcmp(mip->name, "{ind_fnc1") == 0) {
			coll_flags |= COLL_IGNORE_BULLET;
			coll_flags |= COLL_IGNORE_VIS;
		}

		/*
		if(mip->name[0] == '{')
			coll_flags |= COLL_IGNORE_VIS;
		*/

		if(strcmp(mip->name, "{glass00") == 0)
			coll_flags &= ~COLL_IGNORE_BULLET;

		Vector3 face_verts[face->edge_count];
		for(int j = 0; j < face->edge_count; j++) {
			i32 list_edge = bsp->ledges[face->first_edge + j];
			face_verts[j] = (list_edge >= 0) ? bsp->verts[bsp->edges[list_edge].v[0]] : bsp->verts[bsp->edges[-list_edge].v[1]]; 
		}

		Bsp_Plane *plane = &bsp->planes[face->plane];
		Vector3 normal = *(Vector3 *) plane->normal;
		if(face->side) normal = Vector3Negate(normal); 

		for(int j = 1; j < face->edge_count - 1; j++) {
			Tri tri = (Tri) {
				.vertices[0] = face_verts[0],
				.vertices[1] = face_verts[j+1],
				.vertices[2] = face_verts[j],
				.normal = normal,
				.model_id = model_id,
				.collision_flags = coll_flags
			};

			tris[tri_id++] = tri;
		}
	}

	*out_count = tri_id;

	return tris;
}

Shader default_shader;

rBrushList rbrush_list = {0};
rBrushList translucent_rbrush_list = {0};

Plane BuildPlane(Vector3 v0, Vector3 v1, Vector3 v2) {
	Vector3 edge_0 = Vector3Subtract(v1, v0);
	Vector3 edge_1 = Vector3Subtract(v2, v0);

	Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge_0, edge_1));
	float distance = -Vector3DotProduct(normal, v0);
	
	return (Plane) { .normal = normal, .d = distance };
}

Vector3 BrushCenter(Brush *brush) {
	float3 min = Vector3ToFloatV(Vector3Scale(Vector3One(),  FLT_MAX));
	float3 max = Vector3ToFloatV(Vector3Scale(Vector3One(), -FLT_MAX));
	
	for(u16 i = 0; i < brush->vert_count; i++) {
		float3 point = Vector3ToFloatV(brush->verts[i]);

		for(short a = 0; a < 3; a++) {
			if(point.v[a] < min.v[a])	
				min.v[a] = point.v[a];

			if(point.v[a] > max.v[a])	
				max.v[a] = point.v[a];
		}
	}

	float3 out = {0};
	for(short a = 0; a < 3; a++)
		out.v[a] = (min.v[a] + max.v[a]) * 0.5f;

	return (Vector3) {
		.x = out.v[0],
		.y = out.v[1],
		.z = out.v[2]
	};
}

short ThreePlaneIntersect(Plane a, Plane b, Plane c, Vector3 *v) {
	float denom = Vector3DotProduct(a.normal, Vector3CrossProduct(b.normal, c.normal));
	if(fabsf(denom) < PLANE_EPS) return 0;

	Vector3 tA = Vector3Scale(Vector3CrossProduct(b.normal, c.normal), -a.d);
	Vector3 tB = Vector3Scale(Vector3CrossProduct(c.normal, a.normal), -b.d);
	Vector3 tC = Vector3Scale(Vector3CrossProduct(a.normal, b.normal), -c.d);

	*v = Vector3Scale(Vector3Add(Vector3Add(tA, tB), tC), 1.0f / denom);
	return 1;
} 

Vector3 *CollectTriplets(Brush *brush, u8 *t_count) {
	u8 count = 0;
	Vector3 *vertices = calloc(64, sizeof(Vector3));

	for(u8 i = 0; i < brush->plane_count; i++) {
		Plane a = brush->planes[i];
		for(u8 j = i+1; j < brush->plane_count; j++) {
			Plane b = brush->planes[j];
			for(u8 k = j+1; k < brush->plane_count; k++) {
				Plane c = brush->planes[k];

				Vector3 v;
				if(ThreePlaneIntersect(a, b, c, &v)) {
					i8 in = true;
					for(u8 p = 0; p < brush->plane_count; p++) {
						Plane plane = brush->planes[p];
						if(Vector3DotProduct(plane.normal, v) + plane.d > 0.1f) {
							in = 0;
							break;
						}
					}
					if(in) {
						bool unique = true;
						for(u8 t = 0; t < count; t++) {
							if(Vector3Distance(vertices[t], v) < EPSILON) {
								unique = false;
								break;
							}
						}
						if(unique)
							vertices[count++] = v;
					}
				}
			}
		}
	}

	*t_count = count;
	return vertices;
}

void BrushGetVertices(Brush *brush) {
	// Collect triple-plane intersections
	u8 count = 0;
	Vector3 *vertices = CollectTriplets(brush, &count);

	// Collect edge intersections
	for(u8 i = 0; i < brush->plane_count; i++) {
		for(u8 j = i +1; j < brush->plane_count; j++) {
			Plane *pl_A = &brush->planes[i];
			Plane *pl_B = &brush->planes[j];

			Vector3 edge_dir = Vector3CrossProduct(pl_A->normal, pl_B->normal);
			if(Vector3Length(edge_dir) < 0.001f) continue;

			for(u8 k = 0; k < brush->plane_count; k++) {
				if(k == i || k == j) continue;

				Vector3 v = (Vector3) {0};
				if(ThreePlaneIntersect(*pl_A, *pl_B, brush->planes[k], &v)) {
					i8 in = true;
					for(u8 p = 0; p < brush->plane_count; p++) {
						if(Vector3DotProduct(brush->planes[p].normal, v) + brush->planes[p].d > PLANE_EPS) {
							in = false;
							break;
						}
					}

					if(in) {
						bool unique = true;
						for(u8 t = 0; t < count; t++) {
							if(Vector3Distance(vertices[t], v) < 0.001f) {
								unique = false;
								break;
							}
						}
						if(unique) 
							vertices[count++] = v;
					}
				}
			}
		}
	}

	for(u8 i = 0; i < count; i++) {
		Vector3 v = vertices[i];
		
		brush->bounds.min = Vector3Min(brush->bounds.min, v);
		brush->bounds.max = Vector3Max(brush->bounds.max, v);
	}

	brush->center = BoxCenter(brush->bounds);

	brush->vert_count = count;
	memcpy(brush->verts, vertices, sizeof(Vector3) * count);

	free(vertices);
}

#define PARSE_NONE 	   -1
#define PARSE_BRUSH 	0
#define PARSE_ENT 		1
void LoadMapFile(BrushPool *brush_pool, char *path, SpawnList *spawn_list) {
	FILE *pF = fopen(path, "r");

	if(!pF) {
		MessageError("ERROR: no .map file at ->", path);
		return;
	}

	brush_pool->count = 0;
	brush_pool->brushes = calloc(4096, sizeof(Brush));
	for(int i = 0; i < 4096; i++) {
		Brush *brush = &brush_pool->brushes[i];
		brush->bounds = EmptyBox();
	}

	int curr_brush = 0;
	int curr_ent = 1;

	int curr_model_id = 0;
	bool model_incremented = false;

	short parse_mode = -1;

	char line[256];
	while(fgets(line, sizeof(line), pF)) {
		Brush *brush = &brush_pool->brushes[curr_brush];
		EntSpawn *curr_entspawn = &spawn_list->arr[curr_ent];

		// Set mode & id of brush or entity 
		if(line[0] == '/' && line[1] == '/' && line[2] == ' ') {
			if(line[3] == 'b') {
				if(!curr_ent) {
					parse_mode = PARSE_BRUSH;
					char *tok; 
					tok = strtok(line, " ");
					while(tok != NULL) {
						sscanf(tok, "%d", &curr_brush);
						tok = strtok(NULL, " ");
					}
					brush_pool->count++;
					memcpy(brush_pool->brushes[curr_brush].class_name, curr_entspawn->classname, 64);

				} else {
					parse_mode = PARSE_BRUSH;
					curr_brush = brush_pool->count++;
					memcpy(brush_pool->brushes[curr_brush].class_name, curr_entspawn->classname, 64);
					brush_pool->brushes[curr_brush].model_id = curr_model_id;
				}

				if(!model_incremented) {
					if( strncmp(curr_entspawn->classname, "func_forcefield", strlen("func_forcefield")) == 0 ||
						strncmp(curr_entspawn->classname, "func_door", strlen("func_door")) 			== 0 ||
						strncmp(curr_entspawn->classname, "func_lift", strlen("func_lift")) 			== 0 || 
						strncmp(curr_entspawn->classname, "func_glass", strlen("func_glass")) 			== 0 ||
						strncmp(curr_entspawn->classname, "func_ladder", strlen("func_ladder"))			== 0 ||
						strncmp(curr_entspawn->classname, "func_box", strlen("func_box"))				== 0) {

						curr_model_id++;
						model_incremented = true;
						brush_pool->brushes[curr_brush].model_id = curr_model_id;
					} else {
						brush_pool->brushes[curr_brush].model_id = 0;
					}
				}

				/*
				if(strcmp(curr_entspawn->classname, "func_forcefield"))
					continue;
					*/

			} else if(line[3] == 'e') {
				parse_mode = PARSE_ENT;
				char *tok = strtok(line, " ");
				curr_ent++;
				spawn_list->count++;
				model_incremented = false;
			}
		}

		if(parse_mode == PARSE_NONE)
			continue;

		if(parse_mode == PARSE_BRUSH && line[0] == '(') {
			char *points_str = line;
			char *last_par = strrchr(line, ')') + 1;
			*last_par = '\0';
			
			// Get points
			Vector3 points[3] = {0};
			sscanf(
				points_str, 
				"( %f %f %f ) ( %f %f %f ) ( %f %f %f )", 
				&points[0].x, &points[0].y, &points[0].z,
				&points[1].x, &points[1].y, &points[1].z,
				&points[2].x, &points[2].y, &points[2].z
			);

			// Get texture name
			char *tex_str = last_par + 1;
			char *space = strchr(tex_str, ' ');
			*space = '\0';
			memcpy(brush->tex_name, tex_str, strlen(tex_str));
			char *uv_str = space + 1;

			float u = 0, v = 0, r = 0, scale_x = 1, scale_y = 1;
			sscanf(
				uv_str,
				"%f %f %f %f %f",
				&u, &v, &r, &scale_x, &scale_y
			);

			brush->uv = (Vector2) { .x = u, .y = v };
			brush->uv_scale = (Vector2) { .x = scale_x, .y = scale_y };
			brush->uv_rot = r;
			
			// Make plane
			Plane plane = BuildPlane(points[1], points[0], points[2]);
			brush->planes[brush->plane_count++] = plane;

			brush->model_id = curr_model_id;

			//printf("creating brush %d from entity classname: '%s'\n", curr_brush, curr_entspawn->classname);
			memcpy(brush_pool->brushes[curr_brush].class_name, curr_entspawn->classname, 64);
		}

		if(parse_mode == PARSE_ENT) {
			if(line[0] != '"') continue;

			// Get key string
			char *key = strtok(line, "\"");

			// Get value string
			char *end = &line[strlen(key) + 2];
			char *val = strchr(end, '"');
			val = strtok(val, "\"");

			//MessageKeyValPair(key, val);

			if(streq(key, "classname")) {
				memcpy(curr_entspawn->classname, val, strlen(val));
			}

			if(streq(key, "origin")) {
				int x, y, z;
				sscanf(val, "%d %d %d", &x, &y, &z);

				curr_entspawn->position = (Vector3) { x, y, z }; 
			}

			if(streq(key, "enum_id")) {
				sscanf(val, "%d", &curr_entspawn->ent_type);
			}

			if(streq(key, "angle")) {
				sscanf(val, "%d", &curr_entspawn->angle);
			}

			if(streq(key, "trigger_group")) {
				sscanf(val, "%d", &curr_entspawn->trigger_group);
			}

			if(streq(key, "on_trigger")) {
				sscanf(val, "%d", &curr_entspawn->on_trigger);
			}
		}
	}

	fclose(pF);

	for(u16 i = 0; i < brush_pool->count; i++) {
		Brush *brush = &brush_pool->brushes[i];

		//puts("--------------------------");
		//printf("%s\n", brush->class_name);
		//printf("%s\n", brush->tex_name);
		//puts("--------------------------");
	
		// Build vertices, AABBs
		BrushGetVertices(brush);
		for(u16 j = 0; j < brush->vert_count; j++) {
			brush->bounds.min = Vector3Min(brush->bounds.min, brush->verts[j]);
			brush->bounds.max = Vector3Max(brush->bounds.max, brush->verts[j]);
		}
	}

	if(GetLogState()) {
		Message("--------------- [ ENTITIES ] -----------------", ANSI_GREEN);
		for(int i = 0; i < spawn_list->count; i++) {
			Message("-----------------------", ANSI_GREEN);
			MessageKeyValPair("classname", spawn_list->arr[i].classname);
			MessageKeyValPairInt("type", spawn_list->arr[i].ent_type);
			MessageKeyValPairVec3("pos", spawn_list->arr[i].position.x, spawn_list->arr[i].position.y, spawn_list->arr[i].position.z);
			MessageKeyValPairFloat("angle", spawn_list->arr[i].angle);
		}
		Message("----------------------------------------------", ANSI_GREEN);
	}
}

// Expand brushes to use as fitting collision volumes 
BrushPool ExpandBrushes(BrushPool *brush_pool, Vector3 aabb_extents) {
	BrushPool exp = (BrushPool) {0};
	//exp.count = brush_pool->count;
	exp.brushes = calloc(brush_pool->count, sizeof(Brush));
	exp.count = 0;

	// 1. Extend plane by it's normal
	Vector3 half_extents = Vector3Scale(aabb_extents, 0.5f);
	for(u16 i = 0; i < brush_pool->count; i++) {
		/*
		if(strcmp(brush_pool->brushes[i].tex_name, "{ff") == 0) {
			Message("EXP SKIP ff", ANSI_RED);
			continue;
		}
		*/

		exp.brushes[exp.count] = (Brush) { .plane_count = brush_pool->brushes[i].plane_count, .vert_count = brush_pool->brushes[i].vert_count };
		Brush *brush = &exp.brushes[exp.count];

		memcpy(brush->planes, brush_pool->brushes[i].planes, sizeof(Plane) * brush->plane_count);
		memcpy(brush->verts, brush_pool->brushes[i].verts, sizeof(Vector3) * brush->vert_count);
		brush->model_id = brush_pool->brushes[i].model_id;

		for(u8 j = 0; j < brush->plane_count; j++) {
			Plane *plane = &brush->planes[j];

			float diff = MinkowskiDiff(plane->normal, half_extents);
			plane->d -= diff;
		}

		// 2. Rebuild vertices, AABBs
		BrushGetVertices(brush);
		for(u16 j = 0; j < brush->vert_count; j++) {
			brush->bounds.min = Vector3Min(brush->bounds.min, brush->verts[j]);
			brush->bounds.max = Vector3Max(brush->bounds.max, brush->verts[j]);
		}

		exp.count++;
	}	
	
	return exp;
}

Tri *BrushToTris(Brush *brush, u16 *count, u16 brush_id) {
	Tri *tris = calloc(128, sizeof(Tri));
	u16 tri_count = 0;

	u8 coll_flags = 0;
	if(strcmp(brush->tex_name, "{fence00") == 0) 
		coll_flags |= COLL_IGNORE_BULLET;
	if(strcmp(brush->tex_name, "{ff") == 0)
		coll_flags |= COLL_IGNORE_BULLET;

	for(u8 i = 0; i < brush->plane_count; i++) {
		FaceVert face_verts[64] = {0};
		u8 fv_count = 0;

	 	Plane *plane = &brush->planes[i];

		for(u8 j = 0; j < brush->vert_count; j++) {
			Vector3 v = brush->verts[j];

			bool in = (fabsf(Vector3DotProduct(plane->normal, v) + plane->d) <= 0.1f);
			if(!in) continue;

			face_verts[fv_count++].p = v;
		}

		if(fv_count < 3) continue;

		Vector3 center = Vector3Zero();
		for(u8 j = 0; j < fv_count; j++) 
			center = Vector3Add(center, face_verts[j].p);
		center = Vector3Scale(center, 1.0f / fv_count);		

		Vector3 u = Vector3Normalize(
			(fabsf(plane->normal.x) > 0.9f) ? 
			Vector3CrossProduct(plane->normal, UP) :
			Vector3CrossProduct(plane->normal, (Vector3) { 1, 0, 0 } )
		);
		Vector3 v = Vector3CrossProduct(plane->normal, u); 	

		for(u8 j = 0; j < fv_count; j++) {
			Vector3 d = Vector3Subtract(face_verts[j].p, center);
			float x = Vector3DotProduct(d, u);
			float y = Vector3DotProduct(d, v);
			face_verts[j].a = atan2f(y, x);
		}

		for(u8 a = 0; a < fv_count; a++) {
			for(u8 b = a+1; b < fv_count; b++) {
				if(face_verts[a].a > face_verts[b].a) {
					FaceVert temp = face_verts[b];
					face_verts[b] = face_verts[a];
					face_verts[a] = temp;
				}
			}
		}

		for(u8 j = 1; j < fv_count - 1; j++) {
			tris[tri_count++] = (Tri) {
				.vertices[0] = face_verts[0].p,
				.vertices[1] = face_verts[j].p,
				.vertices[2] = face_verts[j+1].p,
				.normal = plane->normal,
				.hull_id = brush_id,
				.collision_flags = coll_flags
			};
		}
	}
	
	*count = tri_count;

 	if(tri_count)
		tris = realloc(tris, sizeof(Tri) * tri_count);

	return tris;
}

Tri *TrisFromBrushPoolFiltered(BrushPool *brush_pool, u16 *count, int model_id) {
	u16 tri_count = 0;
	u16 tri_cap = 1024;
	Tri *tris = calloc(tri_cap, sizeof(Tri));

	u8 coll_flags = 0;

	for(u16 i = 0; i < brush_pool->count; i++) {
		Brush *brush = &brush_pool->brushes[i];

		if(brush->model_id != model_id)
			continue;

		if(strcmp(brush->tex_name, "{ff") == 0) {
			Message("SKIPPED {ff", ANSI_RED);
			continue;
		}

		u16 temp_count = 0;
		Tri *brush_tris = BrushToTris(brush, &temp_count, i);

		if(tri_count + temp_count > tri_cap) {
			tri_cap = (tri_cap << 1);
			tris = realloc(tris, sizeof(Tri) * tri_cap);
		}

		memcpy(tris + tri_count, brush_tris, sizeof(Tri) * temp_count);
		tri_count += temp_count;

		free(brush_tris);
	}

	*count = tri_count;
	tri_cap = tri_count;
	if(tri_count) tris = realloc(tris, sizeof(Tri) * tri_cap);
	return tris;
}

Tri *TrisFromBrushPool(BrushPool *brush_pool, u16 *count) {
	u16 tri_count = 0;
	u16 tri_cap = 1024;
	Tri *tris = calloc(tri_cap, sizeof(Tri));

	for(u16 i = 0; i < brush_pool->count; i++) {
		Brush *brush = &brush_pool->brushes[i];

		if(strcmp(brush->tex_name, "{ff") == 0) {
			Message("SKIPPED {ff", ANSI_RED);
			continue;
		}

		u16 temp_count = 0;
		Tri *brush_tris = BrushToTris(brush, &temp_count, i);

		if(tri_count + temp_count > tri_cap) {
			tri_cap = (tri_cap << 1);
			tris = realloc(tris, sizeof(Tri) * tri_cap);
		}

		memcpy(tris + tri_count, brush_tris, sizeof(Tri) * temp_count);
		tri_count += temp_count;

		free(brush_tris);
	}

	*count = tri_count;
	tri_cap = tri_count;
	if(tri_count) tris = realloc(tris, sizeof(Tri) * tri_cap);
	return tris;
}

Model BrushToModel(Brush *brush, Bsp_Data *bsp, u8 *out_flags) {
	Model model = (Model) {0};
	Texture2D tex;

	Shader shader;
	bool use_shader = false;
	*out_flags = 0;

	// Load texture and shader for model to use
	if(strcmp(brush->tex_name, "{ff") == 0) {
		tex = LoadTextureFromImage(GenImageColor(1, 1, ColorAlpha(BLUE, 0.5)));
		shader = bsp->ff_shader;
		use_shader = true;
		
		*out_flags |= RBRUSH_FORCEFIELD;

	} else {
		tex = LoadTexture(TextFormat("tools/Disruptor/textures/custom/%s.png", brush->tex_name));
		use_shader = false;
	}

	GenTextureMipmaps(&tex);
	
	// Build triangles
	u16 tri_count = 0;
	Tri *tris = BrushToTris(brush, &tri_count, 0);

	// Convert triangles to mesh
	Mesh mesh = (Mesh) {0};

	mesh.triangleCount 	= tri_count;
	mesh.vertexCount 	= tri_count * 3;

	mesh.vertices 	= MemAlloc(sizeof(float) * mesh.vertexCount * 3);
	mesh.normals 	= MemAlloc(sizeof(float) * mesh.vertexCount * 3);	
	mesh.texcoords 	= MemAlloc(sizeof(float) * mesh.vertexCount * 2);
	mesh.texcoords2 = MemAlloc(sizeof(float) * mesh.vertexCount * 2);

	u16 vert_id = 0;
	for(u16 i = 0; i < mesh.triangleCount; i++) {

		Vector3 vs, vt;
		GetTextureAxes(tris[i].normal, &vs, &vt);

		for(u16 j = 0; j < 3; j++) {
			mesh.vertices[vert_id*3+0] = tris[i].vertices[j].x;
			mesh.vertices[vert_id*3+1] = tris[i].vertices[j].y;
			mesh.vertices[vert_id*3+2] = tris[i].vertices[j].z;

			mesh.normals[vert_id*3+0] = tris[i].normal.x;
			mesh.normals[vert_id*3+1] = tris[i].normal.y;
			mesh.normals[vert_id*3+2] = tris[i].normal.z;

			float u = (Vector3DotProduct(tris[i].vertices[j], vs) + brush->uv.x) / brush->uv_scale.x;
			float v = (Vector3DotProduct(tris[i].vertices[j], vt) + brush->uv.y) / brush->uv_scale.y;

			mesh.texcoords[vert_id*2+0] = u / tex.width;
			mesh.texcoords[vert_id*2+1] = v / tex.height;

			vert_id++;	
		}
	}

	free(tris);

	UploadMesh(&mesh, false);
	model = LoadModelFromMesh(mesh);

	model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;
	if(use_shader) model.materials[0].shader = shader;
	else {
		//model.materials[0].maps[MATERIAL_MAP_METALNESS].texture = bsp->lm.tex;
		//model.materials[0].shader = bsp->lm_shader;
	}

	return model;
}

void BrushTestView(BrushPool *brush_pool, Color color) {
	for(u16 i = 0; i < brush_pool->count; i++) {
		Brush *brush = &brush_pool->brushes[i];
		//DrawBoundingBox(brush->bounds, SKYBLUE);
		
		for(short j = 0; j < brush->vert_count; j++) {
			//DrawSphere( (Vector3) { brush->verts[j].x, brush->verts[j].y, brush->verts[j].z }, 5, SKYBLUE);
			//Vector3 v = Vector3Negate(brush->verts[j]);
			Vector3 v = brush->verts[j];
		}
	}
}

// * NOTE:
// The plane math for expanding the hulls is completely unsusable and fucked for use in in-game tracing. 
// However, the tris do build correctly it's just the end planes (terrible coordinate coversion from id format /:)
// A hacky solution could be to reconstruct the planes *post* tri construction and remove the duplicates...  2 tris = 1 plane
MapSection BuildMapSect(char *path, SpawnList *spawn_list) {
	MessageDiag("Constructing map section", path, ANSI_BLUE);

	MapSection sect = (MapSection) {0};

	if(!DirectoryExists(path)) {
		MessageError("Missing directory", path);
		return sect;
	}

	FilePathList path_list = LoadDirectoryFiles(path);

	// -------------------------------------------------------------------------------
	// Load bsp data
	Message("Loading bsp", ANSI_BLUE);

	short bsp_id = -1;
	for(short i = 0; i < path_list.count; i++)
		if(strcmp(GetFileExtension(path_list.paths[i]), ".bsp") == 0) bsp_id = i;

	if(bsp_id == -1) { 
		MessageError("Missing .bsp file", path);
		return sect;
	}

	sect.bsp_data = LoadBsp(path_list.paths[bsp_id], false);

	for(short i = 0; i < 4; i++) {
		sect.bsp[i] = Bsp_BuildHull(&sect.bsp_data, i);
	}

	short lit_path_id = 0;
	for(short i = 0; i < path_list.count; i++)
		if(strcmp(GetFileExtension(path_list.paths[i]), ".lit") == 0) lit_path_id = i;

	// Make BSP lightmap for lighting level geometry
	sect.bsp_data.lm = BuildLightmap(&sect.bsp_data);

	// Make render brush models (from bsp leaves)
	rbrush_list.count = 0;
	rbrush_list.cap = 8192;
	//rbrush_list.render_brushes = malloc(sizeof(RenderBrush) * rbrush_list.cap);
	rbrush_list.render_brushes = calloc(rbrush_list.cap, sizeof(RenderBrush));
	rbrush_list.ids = malloc(sizeof(int) * rbrush_list.cap);

	for(int i = 0; i < sect.bsp_data.models[0].num_leafs; i++) {
		int temp_count = 0;
		RenderBrush *temp_brushes = BspLeafToRenderBrushes(&sect.bsp_data, &sect.bsp_data.leaves[i], &temp_count); 

		for(int j = 0; j < temp_count; j++) {
			rbrush_list.render_brushes[rbrush_list.count + j] = temp_brushes[j];
			rbrush_list.ids[rbrush_list.count + j] = i;
		}

		rbrush_list.count += temp_count;
	}

	rbrush_list.cap = rbrush_list.count;
	rbrush_list.render_brushes = realloc(rbrush_list.render_brushes, sizeof(RenderBrush) * rbrush_list.cap);
	rbrush_list.ids = realloc(rbrush_list.ids, sizeof(int) * rbrush_list.cap);
	// -------------------------------------------------------------------------------

	// Load .map file, collision, physics, ai logic, etc. 
	Message("Loading map file...", ANSI_BLUE);
	short mpf_id = -1;
	for(short i = 0; i < path_list.count; i++) if(strcmp(GetFileExtension(path_list.paths[i]), ".map") == 0) mpf_id = i;

	// No .map, exit
	if(mpf_id == -1) { 
		MessageError("Missing .map file", NULL);
		return sect;
	}

	*spawn_list = (SpawnList) {
		.count = 0,
		.capacity = 255,
	};
	spawn_list->arr = calloc(spawn_list->capacity, sizeof(EntSpawn));
	
	BrushPool brush_pools[3] = {0};
	LoadMapFile(&brush_pools[0], path_list.paths[mpf_id], spawn_list);

	// * NOTE:
	// Change this...
	// Brushes belonging to separate bsp models should not be included in tri contrstruction
	//sect._tris[0].arr = TrisFromBrushPool(&brush_pools[0], &sect._tris[0].count);
	sect._tris[0].arr = TrisFromBrushPoolFiltered(&brush_pools[0], &sect._tris[0].count, 0);
	//sect._tris[0].arr = TrisFromBspModel(&sect.bsp_data, &sect._tris[0].count, 0);
	sect._tris[0].ids = calloc(sect._tris[0].count, sizeof(u16));
	for(u16 j = 0; j < sect._tris[0].count; j++) sect._tris[0].ids[j] = j;

	// 2. Build expanded geometry for character to world collsions 
	for(short i = 1; i < 3; i++) {
		brush_pools[i].count = brush_pools[0].count;
		brush_pools[i].brushes = calloc(brush_pools[i].count, sizeof(Brush));
		memcpy(brush_pools[i].brushes, brush_pools[0].brushes, sizeof(Brush) * brush_pools[0].count);

		// Expand brush planes/vertices
		Vector3 volume = (i == 1) ? BODY_VOLUME_MEDIUM : BODY_VOLUME_SMALL;
		BrushPool exp = ExpandBrushes(&brush_pools[i], volume);

		// Extract tris
		//sect._tris[i].arr = TrisFromBrushPool(&exp, &sect._tris[i].count);
		sect._tris[i].arr = TrisFromBrushPoolFiltered(&exp, &sect._tris[i].count, 0);
		//sect._tris[i].arr = TrisFromBspModel(&sect.bsp_data, &sect._tris[i].count, 0);
		sect._tris[i].ids = calloc(sect._tris[i].count, sizeof(u16));
		for(u16 j = 0; j < sect._tris[i].count; j++) sect._tris[i].ids[j] = j;
	}

	// 3. Construct BVH trees for each geometry set
	for(short i = 0; i < 3; i++) {
		BvhTree *bvh = &sect.bvh[i];
		bvh->tris = (TriPool) {0};
		bvh->tris.count = sect._tris[i].count;
		bvh->tris.arr = calloc(sect._tris[i].count, sizeof(Tri));
		bvh->tris.ids = calloc(sect._tris[i].count, sizeof(u16));
		for(u16 j = 0; j < sect._tris[i].count; j++) {
			bvh->tris.arr[j] = sect._tris[i].arr[j];
			bvh->tris.ids[j] = sect._tris[i].ids[j];
		}

		Vector3 volume = Vector3Zero();
		//if(i == 1)
			//volume = BODY_VOLUME_MEDIUM;

		BvhConstruct(&sect, &sect.bvh[i], volume, &sect._tris[i]);
		if(GetLogState()) printf("bvh[%d] node count: %d\n", i, sect.bvh->count);
	}
	
	// 4. Copy/convert brushes to hulls
	for(short i = 0; i < 3; i++) {
		BrushPool *bp = &brush_pools[i];

		sect._hulls[i] = (HullPool) {
			.arr = malloc(sizeof(Brush) * bp->count),
			.count = bp->count
		};

		for(u16 j = 0; j < bp->count; j++) {
			Brush *brush = &bp->brushes[j];

			Hull hull = (Hull) {
				.aabb = brush->bounds,
				.center = brush->center,
				.plane_count = brush->plane_count,
				.vert_count = brush->vert_count,
				.id = j
			};

			memcpy(hull.planes, brush->planes, sizeof(Plane) * brush->plane_count);
			memcpy(hull.verts, brush->verts, sizeof(Vector3) * brush->vert_count);

			sect._hulls[i].arr[j] = hull;
		}
	}

	// Make render brush models for translucent objects
	translucent_rbrush_list.count = 0;
	translucent_rbrush_list.cap = 4096;
	translucent_rbrush_list.render_brushes = malloc(sizeof(RenderBrush) * translucent_rbrush_list.cap);
	translucent_rbrush_list.ids = malloc(sizeof(int) * translucent_rbrush_list.cap);

	for(int i = 0; i < brush_pools[0].count; i++) {
		// Skip non translucent brushes (tranlucent brushes are denoted with first char being '{', HL1/goldsrc convention)
		if(brush_pools[0].brushes[i].tex_name[0] != '{')
			continue;

		// Skip force field, they are classified as entities
		if(strcmp(brush_pools[0].brushes[i].tex_name, "{ff") == 0) {
			continue;
		}

		RenderBrush render_brush = (RenderBrush) {0};
		render_brush.model = BrushToModel(&brush_pools[0].brushes[i], &sect.bsp_data, &render_brush.flags);
		translucent_rbrush_list.render_brushes[translucent_rbrush_list.count++] = render_brush;
	}

	translucent_rbrush_list.cap = translucent_rbrush_list.count;
	translucent_rbrush_list.render_brushes = realloc(translucent_rbrush_list.render_brushes, sizeof(RenderBrush) * translucent_rbrush_list.cap);
	translucent_rbrush_list.ids = realloc(translucent_rbrush_list.ids, sizeof(int) * translucent_rbrush_list.cap);

	sect.bvh_hullgroup_count = sect.bsp_data.num_models;
	sect.bvh_hullgroups = malloc(sizeof(Bvh_HullGroup) * sect.bvh_hullgroup_count);
	for(int i = 0; i < sect.bvh_hullgroup_count; i++) {
		Bvh_HullGroup *hg = &sect.bvh_hullgroups[i];
		*hg = (Bvh_HullGroup) {0};
		
		hg->bvh[0].tris.arr = TrisFromBspModel(&sect.bsp_data, &hg->bvh[0].tris.count, i);
		hg->bvh[0].tris.ids = calloc(hg->bvh[0].tris.count, sizeof(u16));
		for(int k = 0; k < hg->bvh[0].tris.count; k++) hg->bvh[0].tris.ids[k] = k;

		BvhConstruct(&sect, &hg->bvh[0], Vector3Zero(), &hg->bvh[0].tris);
		if(hg->bvh[0].tris.count == 0) {
			free(hg->bvh->tris.arr);
			free(hg->bvh->tris.ids);

			hg->flags = 0;

			continue;
		}

		/*
		for(int j = 1; j < 3; j++) {
			hg->bvh[j].tris.arr = TrisFromBrushPoolFiltered(&brush_pools[j], &hg->bvh[j].tris.count, i);
			hg->bvh[j].tris.ids = calloc(hg->bvh[j].tris.count, sizeof(u16));
			for(int k = j; k < hg->bvh[j].tris.count; k++) hg->bvh[j].tris.ids[k] = k;
			
			Vector3 volume = Vector3Zero();
			if(j == 1)
				volume = BODY_VOLUME_MEDIUM;
			if(j == 2)
				volume = BODY_VOLUME_SMALL;

			printf("hg[%d] bvh[%d] tri count: %d\n", i, j, hg->bvh[j].tris.count);
			BvhConstruct(&sect, &hg->bvh[j], volume, &hg->bvh[j].tris);
		}
		*/

		for(int j = 1; j < 3; j++) {
			if(i == 0) break;

			// Build pool of brushes for this model only
			BrushPool model_pool = {0};
			model_pool.brushes = calloc(brush_pools[0].count, sizeof(Brush));
			for(int b = 0; b < brush_pools[0].count; b++) {
				if(brush_pools[0].brushes[b].model_id == i)
					model_pool.brushes[model_pool.count++] = brush_pools[0].brushes[b];
			}

			if(model_pool.count == 0) {
				free(model_pool.brushes);
				printf("model empty\n");
				continue;
			}

			Vector3 volume = (j == 1) ? BODY_VOLUME_MEDIUM : BODY_VOLUME_SMALL;
			BrushPool exp = ExpandBrushes(&model_pool, volume);

			hg->bvh[j].tris.arr = TrisFromBrushPool(&exp, &hg->bvh[j].tris.count);
			hg->bvh[j].tris.ids = calloc(hg->bvh[j].tris.count, sizeof(u16));
			for(int k = 0; k < hg->bvh[j].tris.count; k++) hg->bvh[j].tris.ids[k] = k;
			BvhConstruct(&sect, &hg->bvh[j], Vector3Zero(), &hg->bvh[j].tris);

			free(exp.brushes);
			free(model_pool.brushes);
		}

		hg->flags |= HULLGROUP_ACTIVE;
	}

	for(short i = 0; i < 3; i++) {
		BrushPool *bp = &brush_pools[i];
		free(bp->brushes);
	}

	return sect;
}

void SectFreeBrushData(MapSection *sect) {
	

	for(int i = 0; i < rbrush_list.count; i++) {
		UnloadModel(rbrush_list.render_brushes[i].model);
	}

	for(int i = 0; i < translucent_rbrush_list.count; i++)
		UnloadModel(translucent_rbrush_list.render_brushes[i].model);
}

// This function basically just constructs edges between nodes that already exist
void BuildNavGraph(MapSection *sect) {
	NavGraph *navgraph = &sect->base_navgraph;
	navgraph->edge_count = 0;

	BuildNavEdges(navgraph, sect);
	SubdivideNavGraph(sect, navgraph);
}

#define MAX_EDGE_LENGTH (368.0f*368.0f)
#define MAX_EDGE_ANGLE (95.0f*DEG2RAD)
void BuildNavEdges(NavGraph *navgraph, MapSection *sect) {
	MessageDiag("BuildNavEdges()", NULL, ANSI_BLUE);

	Bsp_Hull *hull = &sect->bsp_data.hull_groups[0].hulls[1];

	navgraph->edge_count = 0;
	navgraph->edge_cap = 128;
	if(navgraph->edges) 
		navgraph->edges = realloc(navgraph->edges, sizeof(NavEdge) * navgraph->edge_cap);
	else {
		//printf("edges null\n");
		navgraph->edges = calloc(navgraph->edge_cap, sizeof(NavEdge));
	}

	for(u16 i = 0; i < navgraph->node_count; i++) {
		NavNode *node_A = &navgraph->nodes[i];

		for(u16 j = 0; j < navgraph->node_count; j++) {
			NavNode *node_B = &navgraph->nodes[j];

			if(j == i)
				continue;

			// Using vector subtraction to get distance,
			// doing this in case I want to integrate actual level geometry later 
			Vector3 v = Vector3Subtract(node_B->position, node_A->position);
			float length = Vector3LengthSqr(v);	

			// Don't build edges if nodes are too far apart
			if(length > MAX_EDGE_LENGTH)
				continue;

			float angle = Vector3Angle(node_A->position, node_B->position);
			if(angle > MAX_EDGE_ANGLE || angle < -MAX_EDGE_ANGLE)
				continue;

			if(fabsf(node_A->position.z - node_B->position.z) >= 64.0f)
				continue;
			
			Bsp_TraceData tr = Bsp_TraceDataEmpty();
			Bsp_RecursiveTraceEx(hull, hull->first_node, 0, 1, node_A->position, node_B->position, &tr);

			if(tr.fraction < 1.0f) {
				continue;
			}

			BvhTraceData bvh_tr = TraceDataEmpty(); 
			Ray ray = (Ray) { .position = node_A->position, .direction = Vector3Normalize(v) };
			BvhTracePointEx(ray, sect, &sect->bvh[0], 0, &bvh_tr, Vector3Distance(node_A->position, node_B->position));
			
			if(bvh_tr.hit)
				continue;

			// Don't build edges if line between nodes are obstructed by level geometry

			// All checks passed, create edge
			NavEdge edge = (NavEdge) { .id_A = i, .id_B = j };

			// Resize edge array if needed
			if(navgraph->edge_count >= navgraph->edge_cap) {
				navgraph->edge_cap = navgraph->edge_cap << 1;
				navgraph->edges = realloc(navgraph->edges, sizeof(NavEdge) * navgraph->edge_cap);	
			}

			// Skip creating edge if the reverse of it already exists 
			bool duplicate = false;
			for(u16 k = 0; k < navgraph->edge_count; k++) {
				NavEdge *edge = &navgraph->edges[k];
				if((edge->id_A == node_A->id && edge->id_B == node_B->id) || (edge->id_B == node_A->id && edge->id_A == node_B->id)) {
					duplicate = true;
					break;
				}
			}
			if(duplicate)
				continue;

			// Copy to array	
			navgraph->edges[navgraph->edge_count++] = edge;
		}
	}

	for(u16 i = 0; i < navgraph->edge_count; i++) {
		NavEdge *edge = &navgraph->edges[i];

		NavNode *node_A = &navgraph->nodes[edge->id_A];
		NavNode *node_B = &navgraph->nodes[edge->id_B];

		node_A->edges[node_A->edge_count++] = i;
		node_B->edges[node_B->edge_count++] = i;
	}

	navgraph->edges = realloc(navgraph->edges, sizeof(NavEdge) * navgraph->edge_count);
	navgraph->nodes = realloc(navgraph->nodes, sizeof(NavNode) * navgraph->node_count);

	navgraph->bounds = (BoundingBox) {
		Vector3Scale(Vector3One(),  FLT_MAX),
		Vector3Scale(Vector3One(), -FLT_MAX)
	};

	for(u16 i = 0; i < navgraph->node_count; i++) {
		NavNode *node = &navgraph->nodes[i]; 

		navgraph->bounds.min = Vector3Min(navgraph->bounds.min, node->position);
		navgraph->bounds.max = Vector3Max(navgraph->bounds.max, node->position);
	}

	navgraph->bounds.min = Vector3Add(navgraph->bounds.min, Vector3Scale( (Vector3) { 1, 1, 0.1f }, -256)); 
	navgraph->bounds.max = Vector3Add(navgraph->bounds.max, Vector3Scale( (Vector3) { 1, 1, 0.1f },  256)); 
}

void GetConnectedNodes(NavNode *node, u16 connected[MAX_EDGES_PER_NODE], u8 *count, NavGraph *navgraph) {
	for(u16 i = 0; i < node->edge_count; i++) {
		NavEdge *edge = &navgraph->edges[node->edges[i]];

		NavNode *node_A = &navgraph->nodes[edge->id_A];
		NavNode *node_B = &navgraph->nodes[edge->id_B];

		u16 next_node = (node_A->id == node->id) ? node_B->id : node_A->id;
		
		for(u8 i = 0; i < *count; i++) {
			if(connected[i] == next_node)
				return;
		}
		
		connected[(*count)++] = next_node; 
	}	
}

void WalkNavGraph(NavGraph *navgraph, u16 start_node, u16 *walked, u16 *count) {
	NavNode *node = &navgraph->nodes[start_node];	

	walked[(*count)++] = start_node;
	
	u16 next_nodes[MAX_EDGES_PER_NODE];
	u8 next_count = 0;

	GetConnectedNodes(node, next_nodes, &next_count, navgraph);
	
	for(u8 i = 0; i < next_count; i++) {
		bool duplicate = false;
		for(u16 j = 0; j < *count; j++) {
			if(walked[j] == next_nodes[i]) {
				duplicate = true;
			}
		}

		if(duplicate) 
			continue;

		WalkNavGraph(navgraph, next_nodes[i], walked, count);
	}
}

// Split navigation graphs so the spatial separation is reflected in data
// Only having one graph would break pathfinding, 
// graph/edge construction is distance based
void SubdivideNavGraph(MapSection *sect, NavGraph *navgraph) {
	u16 walked_total = 0;
	bool *traveled = calloc(navgraph->node_count, sizeof(bool));

	u16 splits[128];
	u16 split_count = 0;

	u16 next = 0;

	while(walked_total < navgraph->node_count) {
		while(next < navgraph->node_count && traveled[next])
			next++;

		if(next >= navgraph->node_count)
			break;

		u16 walk_count = 0;
		u16 temp[navgraph->node_count];
		WalkNavGraph(navgraph, next, temp, &walk_count);

		for(u16 i = 0; i < walk_count; i++) {
			u16 j = temp[i];
			if(!traveled[j]) {
				traveled[j] = true;
				walked_total++;
			}
		}

		splits[split_count++] = next;

		//for(u16 i = 0; i < walk_count; i++) printf("[%d]: -> %d\n", i, temp[i]);
	}

	/*
	printf("split count: %d\n", split_count);
	for(short i = 0; i < split_count; i++) {
		printf("split[%d] = %d \n", i, splits[i]);
	}
	*/

	for(short i = 0; i < split_count; i++) {
		u16 split_id = splits[i];

		u16 walk_count = 0;
		u16 node_ids[navgraph->node_count];

		WalkNavGraph(navgraph, split_id, node_ids, &walk_count);

		NavGraph graph = (NavGraph) {0};
		graph.node_count = walk_count;
		graph.nodes = calloc(graph.node_count, sizeof(NavNode));
		for(u16 j = 0; j < walk_count; j++) {
			graph.nodes[j] = navgraph->nodes[node_ids[j]];
			graph.nodes[j].id = j;
			graph.nodes[j].edge_count = 0;
		}
		BuildNavEdges(&graph, sect);
		for(u16 j = 0; j < graph.edge_count; j++) {
			NavEdge *edge = &graph.edges[j];

			NavNode *a = &graph.nodes[edge->id_A];
			NavNode *b = &graph.nodes[edge->id_B];

			a->edges[a->edge_count++] = j;
			b->edges[b->edge_count++] = j;
		}

		sect->navgraphs[sect->navgraph_count++] = graph;

		/*
		NavGraph *graph = &sect->navgraphs[sect->navgraph_count++];
		graph->node_count = walk_count;
		graph->nodes = calloc(graph->node_count, sizeof(NavNode));
		for(u16 j = 0; j < walk_count; j++) {
			graph->nodes[j] = navgraph->nodes[node_ids[j]];
			graph->nodes[j].id = j;
		}

		graph->edge_cap = 128;
		graph->edges = calloc(graph->edge_cap, sizeof(NavEdge));
		BuildNavEdges(graph);
		*/
	}

	//printf("graph count: %d\n", sect->navgraph_count);

	free(traveled);
}

bool IsNodeInGraph(NavGraph *graph, NavNode *node) {
	/*
	u16 walked[graph->node_count];
	u16 walk_count = 0;
	WalkNavGraph(graph, 0, walked, &walk_count);

	for(u16 i = walk_count; i > 0; i--) {
		if(walked[i] == node->id) {
			if(fabsf(Vector3LengthSqr(Vector3Subtract(node->position, graph->nodes[i].position))) < EPSILON*EPSILON)
				return true;
		}
	}
	*/
	
	if(node->id > graph->node_count)
		return false;

	for(u16 i = 0; i < graph->node_count; i++) {
		float diff = Vector3LengthSqr(Vector3Subtract(node->position, graph->nodes[i].position));	
		diff*=diff;

		if(diff <= EPSILON)
			return true;
	}

	return false;
}

void DebugDrawNavGraphs(MapSection *sect, Model model) {
	/*
	NavGraph *navgraph = &sect->base_navgraph;

	for(u16 e = 0; e < navgraph->edge_count; e++) {
		NavEdge *edge = &navgraph->edges[e];

		NavNode *node_A = &navgraph->nodes[edge->id_A];
		NavNode *node_B = &navgraph->nodes[edge->id_B];

		DrawModel(model, node_A->position, 1, BLUE);
		DrawModel(model, node_B->position, 1, BLUE);

		//Color line_color = (e % 2 == 0) ? MAGENTA : GREEN; 
		Color line_color = MAGENTA;
		DrawLine3D(node_A->position, node_B->position, line_color);
	}
	*/

	for(u16 i = 0; i < sect->navgraph_count; i++) {
		NavGraph *navgraph = &sect->navgraphs[i];

		for(u16 n = 0; n < navgraph->node_count; n++) {
			NavNode *node = &navgraph->nodes[n];
			DrawModel(model, node->position, 1, BLUE);
		}

		for(u16 e = 0; e < navgraph->edge_count; e++) {
			NavEdge *edge = &navgraph->edges[e];

			NavNode *node_A = &navgraph->nodes[edge->id_A];
			NavNode *node_B = &navgraph->nodes[edge->id_B];

			Color node_color[2] = { BLUE };
			if(node_A->flags & IS_COVER) node_color[0] = YELLOW;
			if(node_B->flags & IS_COVER) node_color[1] = YELLOW;

			DrawModel(model, node_A->position, 1, node_color[0]);
			DrawModel(model, node_B->position, 1, node_color[1]);

			Color line_color = (i % 2 == 0) ? MAGENTA : GREEN; 
			DrawLine3D(node_A->position, node_B->position, line_color);
		}

		DrawBoundingBox(navgraph->bounds, (i % 2 == 0) ? MAGENTA : GREEN);
	}
}

void DebugDrawNavGraphsText(MapSection *sect, Camera3D cam, Vector2 window_size) {
	Vector3 cam_dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));

	for(u16 i = 0; i < sect->navgraph_count; i++) {
		NavGraph *navgraph = &sect->navgraphs[i];

		for(u16 n = 0; n < navgraph->node_count; n++) {
			NavNode *node = &navgraph->nodes[n];

			Vector3 to_cam = Vector3Normalize(Vector3Subtract(cam.position, node->position));
			if(Vector3DotProduct(to_cam, cam_dir) > 0) continue;

			float dist = Vector3Distance(node->position, cam.position);
			float text_size = (30);

			Vector2 pos = GetWorldToScreen(node->position, cam);

			for(u16 e = 0; e < navgraph->edge_count; e++) {
				NavEdge *edge = &navgraph->edges[e];

				NavNode *node_A = &navgraph->nodes[edge->id_A];
				NavNode *node_B = &navgraph->nodes[edge->id_B];

				Vector3 mid = Vector3Scale(Vector3Add(node_A->position, node_B->position), 0.5f);

				Vector3 to_cam = Vector3Normalize(Vector3Subtract(cam.position, mid));
				if(Vector3DotProduct(to_cam, cam_dir) > 0) continue;

				Vector2 pos = GetWorldToScreen(mid, cam);

				float text_size = (30);

				DrawText(TextFormat("%d -> %d", edge->id_A, edge->id_B), pos.x, pos.y, text_size, GRAY);
				DrawText(TextFormat("%d", e), pos.x, pos.y - 32, text_size, LIGHTGRAY);
			}

			DrawText(TextFormat("%d", node->id), pos.x, pos.y, text_size, YELLOW);
			//DrawText(TextFormat("ec: %d", node->edge_count), pos.x, pos.y - 32, text_size, SKYBLUE);
			/*
			for(u8 k = 0; k < navgraph->nodes[n].edge_count; k++) {
				DrawText(TextFormat("%d", navgraph->nodes[n].edges[k]), pos.x, pos.y - (32 * (k+1)), text_size, SKYBLUE);
			}
			*/
		}
	}
}

#define TRANSLUCENT_MAX	128
int translucent_count = 0;
int translucent_ids[TRANSLUCENT_MAX] = {0};
void DrawMap(MapSection *sect, Vector3 pos) {
	UpdateBspShaders(&sect->bsp_data);

	translucent_count = 0;
	
	int curr_leaf = Bsp_FindLeaf(&sect->bsp_data, pos);
	for(int i = 0; i < rbrush_list.count; i++) {
		if(!Bsp_LeafVisible(&sect->bsp_data, curr_leaf, rbrush_list.ids[i])) 
			continue;

		if(rbrush_list.render_brushes[i].flags & RBRUSH_TRANSLUCENT) {
			continue;
		}

		DrawModel(rbrush_list.render_brushes[i].model, Vector3Zero(), 1, WHITE);
	}
}

void DrawMapTranslucent(MapSection *sect, Camera3D cam) {
	Vector3 pos = cam.position;
	Vector3 dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));

	// Measure approximate depth
	float depth[translucent_rbrush_list.count];
	for(int i = 0; i < translucent_rbrush_list.count; i++) {
		RenderBrush *rbrush = &translucent_rbrush_list.render_brushes[i];
		BoundingBox aabb = GetModelBoundingBox(rbrush->model);

		Vector3 n = (Vector3) {
			Clamp(pos.x, aabb.min.x, aabb.max.x),
			Clamp(pos.y, aabb.min.y, aabb.max.y),
			Clamp(pos.z, aabb.min.z, aabb.max.z)
		};
		depth[i] = Vector3DotProduct(dir, Vector3Subtract(n, pos));
	}

	// Sort brushes by depth 
	int sorted_ids[translucent_rbrush_list.count];
	for(int i = 0; i < translucent_rbrush_list.count; i++) 
		sorted_ids[i] = i;

	for(int i = 1; i < translucent_rbrush_list.count; i++) {
		float d = depth[i];
		int id = sorted_ids[i];
		int j = i - 1;
		while(j >= 0 && depth[j] < d) {
			depth[j+1] = depth[j];
			sorted_ids[j+1] = sorted_ids[j];
			j--;
		}
		depth[j+1] = d;
		sorted_ids[j+1] = id;
	}

	// Draw brushes
	rlDisableDepthMask();
	for(int i = 0; i < translucent_rbrush_list.count; i++) {
		int idx = sorted_ids[i];

		RenderBrush *rbrush = &translucent_rbrush_list.render_brushes[idx];
		if(rbrush->flags & RBRUSH_FORCEFIELD) 
			continue;

		Color light = lit_SampleLightGrid(&sect->bsp_data, BoxCenter(GetModelBoundingBox(rbrush->model)));
		DrawModel(rbrush->model, Vector3Zero(), 1, light);
	}
	rlEnableDepthMask();
}

// Set up DSP preset nodes for each leaf in BSP 
void DSP_AudioSetup(Bsp_Data *bsp, AudioPlayer *ap, SpawnList *spawn_list) {
	// Unload existing preset nodes (if existing)
	if(ap->ref_presets) 
		free(ap->ref_presets);

	// Allocate memory for nodes with zero values (DSP_DEFAULT) 
	ap->num_ref_presets = bsp->num_leaves;	
	ap->ref_presets = calloc(ap->num_ref_presets, 1);

	// Iterate through spawn list, search for DSP nodes
	for(int i = 0; i < spawn_list->count; i++) {
		EntSpawn *spawn = &spawn_list->arr[i];

		if(strcmp(spawn->classname, "dsp_node"))	
			continue;

		int leaf = Bsp_FindLeaf(bsp, spawn->position);

		u8 type = ap->ref_presets[i];
		switch(spawn->ent_type) {
			case ENT_DSP_DEFAULT:
				type = DSP_DEFAULT;
				break;

			case ENT_DSP_SMALL_ROOM:
				type = DSP_SMALL_ROOM;
				break;

			case ENT_DSP_OPEN:
				type = DSP_OPEN;
				break;
		}

		ap->ref_presets[leaf] = type;

		if(type > 0) {
			BoundingBox sound_box = (BoundingBox) {
				.min = Vector3Scale(Vector3One(), -32),
				.max = Vector3Scale(Vector3One(),  32)
			};
			sound_box = BoxTranslate(sound_box, spawn->position);

			BoxPoints points = BoxGetPoints(sound_box);

			for(short j = 0; j < 8; j++) {
				int subleaf = Bsp_FindLeaf(bsp, points.v[j]);
				if(!Bsp_LeafVisible(bsp, leaf, subleaf))
					continue;

				ap->ref_presets[subleaf] = type;
			}
		}
	}
}

void DSP_UpdateBlend(MapSection *sect, AudioPlayer *ap, Vector3 pos, float dt) {
	//int curr_leaf = Bsp_FindLeaf(&sect->bsp_data, pos);
	//AP_BlendDsp(ap, dt, 1, ap->ref_presets[curr_leaf]);

	int base_leaf = Bsp_FindLeaf(&sect->bsp_data, pos);
	//AP_BlendDsp(ap, dt, 1, ap->ref_presets[base_leaf]);

	BoundingBox sound_box = (BoundingBox) {
		.min = Vector3Scale(Vector3One(), -32),
		.max = Vector3Scale(Vector3One(),  32)
	};
	sound_box = BoxTranslate(sound_box, pos);

	BoxPoints points = BoxGetPoints(sound_box);
	
	float weights_0 = 0.0f;
	float weights_1 = 0.0f;
	float weights_2 = 0.0f;

	for(short i = 0; i < 8; i++) {
		int curr_leaf = Bsp_FindLeaf(&sect->bsp_data, points.v[i]);
		short vis = (Bsp_LeafVisible(&sect->bsp_data, base_leaf, curr_leaf)) ? 1 : -1;

		if(vis < -1 && ap->ref_presets[curr_leaf])
			continue;

		//AP_BlendDsp(ap, dt, 1, ap->ref_presets[curr_leaf]);
		if(ap->ref_presets[curr_leaf] == 0)
			weights_0 += 0.5f * vis; 

		if(ap->ref_presets[curr_leaf] == 1)
			weights_1 += 1.0f * vis;

		if(ap->ref_presets[curr_leaf] == 2)
			weights_2 += 1.0f;
	}

	if(ap->ref_presets[base_leaf] == 0)
		weights_0 += 1.0f; 

	if(ap->ref_presets[base_leaf] == 1)
		weights_1 += 1.0f;

	if(ap->ref_presets[base_leaf] == 2)
		weights_2 += 1.0f;

	u8 preset_id = DSP_DEFAULT; 
	if(weights_1 > weights_0)
		preset_id = 1;
	if(weights_2 > weights_1)
		preset_id = 2;

	AP_BlendDsp(ap, dt, 1, preset_id);
}

void DebugDrawDSP(MapSection *sect, AudioPlayer *ap, Vector3 pos) {
	int curr_leaf = Bsp_FindLeaf(&sect->bsp_data, pos);
	for(int i = 0; i < rbrush_list.count; i++) {
		if(!Bsp_LeafVisible(&sect->bsp_data, curr_leaf, rbrush_list.ids[i])) 
			continue;

		if(!ap->ref_presets[curr_leaf])
			continue;

		DrawModel(rbrush_list.render_brushes[i].model, Vector3Zero(), 1, ColorAlpha(RED, 0.25f));
	}
}

void MapUpdateBvhOffsets(MapSection *sect) {
	for(int i = 0; i < sect->bsp_data.num_models; i++) {
		Bvh_HullGroup *hg = &sect->bvh_hullgroups[i];
		for(int j = 0; j < 3; j++) { 
			hg->bvh[j].origin = hg->origin;
		}
	}
}

void BvhTraceHullGroups(Ray ray, MapSection *sect, BvhTraceData *data, float max_dist, u8 ignore_flags, u8 hull_id) {
	Vector3 ray_pos = ray.position;

	BvhTraceData tr = TraceDataEmpty();
	BvhTracePointPro(ray, sect, &sect->bvh[hull_id], 0, &tr, max_dist, ignore_flags);

	for(int i = 0; i < sect->bvh_hullgroup_count; i++) {
		Bvh_HullGroup *hg = &sect->bvh_hullgroups[i];
		if(!(hg->flags & HULLGROUP_ACTIVE))
			continue;

		BvhTree *bvh = &hg->bvh[hull_id];

		ray.position = ray_pos;
		ray.position = Vector3Subtract(ray.position, hg->origin);
		
		BvhTraceData temp_tr = TraceDataEmpty();
		BvhTracePointPro(ray, sect, bvh, 0, &temp_tr, max_dist, ignore_flags);

		if(temp_tr.distance < tr.distance)
			tr = temp_tr;	
	}

	*data = tr;
}


