#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"
#include "kbsp.h"
#include "../include/log_message.h"
#include "geo.h"
#include "hash.h"

Material *materials;
Texture2D *textures;
HashMap material_hashmap = (HashMap) { 0 };

Bsp_Data LoadBsp(char *path, bool print_output) {
	Bsp_Data data = (Bsp_Data) {0};


	FILE *pF = fopen(path, "rb");

	if(!pF) {
		MessageError("ERROR: Could not load file ", path);
		return data;
	}

	// ---------------------------------------------------------------------------------------
	// Header
	Bsp_Header header = {0};
	fread(&header, sizeof(header), 1, pF);

	/*
	if(header.version != BSP_VERSION) {
		MessageError("ERROR: BSP version mismatch", NULL);
		return data;
	}
	*/

	if(print_output)
		printf("%d\n", header.version);
	// ---------------------------------------------------------------------------------------
	// Entities
	Bsp_Lump ents_lump = header.lumps[LUMP_ENTS];
	
	fseek(pF, ents_lump.file_offset, SEEK_SET);
	data.ent_str = (char*)malloc(ents_lump.file_size);
	fread(data.ent_str, ents_lump.file_size, 1, pF);

	if(print_output) {
		Message("-------- ENTITY DATA --------", ANSI_GREEN);
		MessageDiagInt("bytes", ents_lump.file_size, ANSI_YELLOW);
		//printf("%s\n", data.ent_str);
	}

	// ---------------------------------------------------------------------------------------
	// Planes
	Bsp_Lump planes_lump = header.lumps[LUMP_PLANES];

	fseek(pF, planes_lump.file_offset, SEEK_SET);
	i32 plane_count = planes_lump.file_size / sizeof(Bsp_Plane);  
	Bsp_Plane *planes = malloc(sizeof(Bsp_Plane) * plane_count);
	fread(planes, sizeof(Bsp_Plane) * plane_count, 1, pF);

	data.num_planes = plane_count;
	data.planes = planes;
	// ---------------------------------------------------------------------------------------
	// Miptex
	Bsp_Lump miptex_lump = header.lumps[LUMP_MIPTEX];
	fseek(pF, miptex_lump.file_offset, SEEK_SET);

	i32 miptex_count;
	fread(&miptex_count, sizeof(i32), 1, pF);

	i32 *mip_offsets = malloc(sizeof(i32) * miptex_count);
	fread(mip_offsets, sizeof(i32) * miptex_count, 1, pF);

	data.num_miptex = miptex_count;
	data.miptex = malloc(sizeof(Bsp_Miptex) * miptex_count);

	for(int i = 0; i < miptex_count; i++) {
		fseek(pF, miptex_lump.file_offset + mip_offsets[i], SEEK_SET);
		fread(&data.miptex[i], sizeof(Bsp_Miptex), 1, pF);
	}
	// ---------------------------------------------------------------------------------------
	// Vertices
	Bsp_Lump vert_lump = header.lumps[LUMP_VERTICES];

	fseek(pF, vert_lump.file_offset, SEEK_SET);
	i32 vert_count = vert_lump.file_size / sizeof(Vector3);  
	Vector3 *verts = malloc(sizeof(Vector3) * vert_count);
	fread(verts, sizeof(Vector3) * vert_count, 1, pF);

	data.num_verts = vert_count;
	data.verts = verts;
	// ---------------------------------------------------------------------------------------
	// Vis
	Bsp_Lump vis_lump = header.lumps[LUMP_VIS];
	fseek(pF, vis_lump.file_offset, SEEK_SET);
	data.vis = malloc(vis_lump.file_size);
	fread(data.vis, vis_lump.file_size, 1, pF);
	// ---------------------------------------------------------------------------------------
	// Nodes
	Bsp_Lump nodes_lump = header.lumps[LUMP_NODES];

	fseek(pF, nodes_lump.file_offset, SEEK_SET);
	i32 node_count = nodes_lump.file_size / sizeof(Bsp_Node);
	Bsp_Node *nodes = malloc(sizeof(Bsp_Node) * node_count);
	fread(nodes, sizeof(Bsp_Node) * node_count, 1, pF);

	data.num_nodes = node_count;
	data.nodes = nodes;
	// ---------------------------------------------------------------------------------------
	// Tex info
	Bsp_Lump texinfo_lump = header.lumps[LUMP_TEXINFO];

	fseek(pF, texinfo_lump.file_offset, SEEK_SET);
	i32 texinfo_count = texinfo_lump.file_size / sizeof(Bsp_Surface);
	Bsp_Surface *texinfos = malloc(sizeof(Bsp_Surface) * texinfo_count);
	fread(texinfos, sizeof(Bsp_Surface) * texinfo_count, 1, pF);

	data.num_surfaces = texinfo_count;
	data.surfaces = texinfos;
	// ---------------------------------------------------------------------------------------
	// Faces 
	Bsp_Lump faces_lump = header.lumps[LUMP_FACES];

	fseek(pF, faces_lump.file_offset, SEEK_SET);
	i32 face_count = faces_lump.file_size / sizeof(Bsp_Face);
	Bsp_Face *faces = malloc(sizeof(Bsp_Face) * face_count);
	fread(faces, sizeof(Bsp_Face) * face_count, 1, pF);

	data.num_faces = face_count;
	data.faces = faces; 
	// ---------------------------------------------------------------------------------------
	// Lightmaps 
	Bsp_Lump lightmap_lump = header.lumps[LUMP_LIGHTMAPS];
	fseek(pF, lightmap_lump.file_offset, SEEK_SET);
	data.lightmap.num_lightmap = lightmap_lump.file_size;
	data.lightmap.lightmap = malloc(lightmap_lump.file_size);
	fread(data.lightmap.lightmap, lightmap_lump.file_size, 1, pF);
	// ---------------------------------------------------------------------------------------
	// Clip nodes
	Bsp_Lump clipnodes_lump = header.lumps[LUMP_CLIPNODES];

	fseek(pF, clipnodes_lump.file_offset, SEEK_SET);
	i32 clipnode_count = clipnodes_lump.file_size / sizeof(Bsp_ClipNode);
	Bsp_ClipNode *clipnodes = malloc(sizeof(Bsp_ClipNode) * clipnode_count);
	fread(clipnodes, sizeof(Bsp_ClipNode) * clipnode_count, 1, pF);

	data.num_clipnodes = clipnode_count;
	data.clipnodes = clipnodes; 
	// ---------------------------------------------------------------------------------------
	// Leaves 
	Bsp_Lump leaves_lump = header.lumps[LUMP_LEAVES];

	fseek(pF, leaves_lump.file_offset, SEEK_SET);
	i32 leaf_count = leaves_lump.file_size / sizeof(Bsp_Leaf);
	Bsp_Leaf *leaves = malloc(sizeof(Bsp_Leaf) * leaf_count);
	fread(leaves, sizeof(Bsp_Leaf) * leaf_count, 1, pF);

	data.num_leaves = leaf_count;
	data.leaves = leaves;
	// ---------------------------------------------------------------------------------------
	// L_faces 
	Bsp_Lump lfaces_lump = header.lumps[LUMP_LFACES];	
	
	fseek(pF, lfaces_lump.file_offset, SEEK_SET);
	i32 lfaces_count = lfaces_lump.file_size / sizeof(u16);
	u16 *lfaces = malloc(sizeof(u16) * lfaces_count);
	fread(lfaces, sizeof(u16) * lfaces_count, 1, pF);

	data.num_lfaces = lfaces_count;
	data.lfaces = lfaces;
	// ---------------------------------------------------------------------------------------
	// Edges 
	Bsp_Lump edges_lump = header.lumps[LUMP_EDGES];

	fseek(pF, edges_lump.file_offset, SEEK_SET);
	i32 edge_count = edges_lump.file_size / sizeof(Bsp_Edge);
	Bsp_Edge *edges = malloc(sizeof(Bsp_Edge) * edge_count);
	fread(edges, sizeof(Bsp_Edge) * edge_count, 1, pF);

	data.num_edges = edge_count;
	data.edges = edges;
	// ---------------------------------------------------------------------------------------
	// L_edges 
	Bsp_Lump ledges_lump = header.lumps[LUMP_L_EDGES];

	fseek(pF, ledges_lump.file_offset, SEEK_SET);
	i32 ledge_count = ledges_lump.file_size / sizeof(i32);
	i32 *ledges = malloc(sizeof(i32) * ledge_count);
	fread(ledges, sizeof(i32) * ledge_count, 1, pF);

	data.num_ledges = ledge_count;
	data.ledges = ledges;

	// ---------------------------------------------------------------------------------------
	// Models
	Bsp_Lump models_lump = header.lumps[LUMP_MODELS];
	fseek(pF, models_lump.file_offset, SEEK_SET);

	i32 model_count = models_lump.file_size / sizeof(Bsp_Model);
	Bsp_Model *models = malloc(sizeof(Bsp_Model) * model_count);
	fread(models, sizeof(Bsp_Model) * model_count, 1, pF);

	data.num_models = model_count;
	data.models = models;
	// ---------------------------------------------------------------------------------------
	// Miptex
	/*
	data.miptex_lump_offset = miptex_lump.file_offset;
	data.textures = malloc(sizeof(Texture) * data.num_miptex);

	for(int i = 0; i < data.num_miptex; i++) {
		Bsp_Miptex *mip = &data.miptex[i];

		if(mip->offset1 == 0 || mip->offset1 >= 256) {
			data.textures[i] = (Texture2D) {0};
			continue;
		}

		int px_count = mip->width * mip->height;
		u8 *indexed = malloc(px_count);

		fseek(pF, data.miptex_lump_offset + mip_offsets[i] + mip->offset1, SEEK_SET);
		fread(indexed, px_count, 1, pF);

		u8 *rgba = calloc(px_count * 4, 1);
		for(int j = 0; j < px_count; j++) {
			rgba[j*4+0] = qPalette[indexed[j]][0];   
			rgba[j*4+1] = qPalette[indexed[j]][1];   
			rgba[j*4+2] = qPalette[indexed[j]][2];   
			rgba[j*4+3] = 255;   
		}

		Image img = (Image) {
			.data = rgba,
			.width = mip->width,
			.height = mip->height,
  			.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
			.mipmaps = 1
		};

		data.textures[i] = LoadTextureFromImage(img);
		free(indexed);
		free(rgba);
	}
	free(mip_offsets);
	*/
	// ---------------------------------------------------------------------------------------
	// BSPX
	int last_offset = 0;
	for(int i = 0; i < 15; i++) {
		int end = header.lumps[i].file_offset + header.lumps[i].file_size;
		if(end > last_offset)
			last_offset = end;
	}
	last_offset = (last_offset + 3) & ~3;

	fseek(pF, last_offset, SEEK_SET);
	char magic[4];
	fread(magic, 4, 1, pF);

	if(memcmp(magic, "BSPX", 4) != 0) {
		MessageError("NO BSPX DATA", NULL);

	} else {
		int num_lumps = 0;
		fread(&num_lumps, 4, 1, pF);
		MessageDiagInt("BSPX num_lumps", num_lumps, ANSI_GREEN);

		char names[num_lumps][24];
		int offsets[num_lumps];
		int sizes[num_lumps];

		int lm_shift_id = -1; 
		int lm_rgb_id = -1; 
		int lm_decoupled_id = -1;
		int lm_grid_id = -1;

		for(int i = 0; i < num_lumps; i++) {
			char name[24];
			int fileofs, filelen;
			fread(name, 24, 1, pF);
			fread(&fileofs, 4, 1, pF);
			fread(&filelen, 4, 1, pF);

			memcpy(names[i], name, 24);
			offsets[i] = fileofs;
			sizes[i] = filelen;

			if(memcmp(name, "RGBLIGHTING", strlen("RGBLIGHTING")) == 0)
				lm_rgb_id = i;
			if(memcmp(name, "DECOUPLED_LM", strlen("DECOUPLED_LM")) == 0)
				lm_decoupled_id = i;
			if(memcmp(name, "LIGHTGRID_OCTREE", strlen("LIGHTGRID_OCTREE")) == 0)
				lm_grid_id = i;
		}

		if(GetLogState()) {
			for(int i = 0; i < num_lumps; i++) {
				printf("  bspx lump: %s ofs = %d, len = %d\n", names[i], offsets[i], sizes[i]);
			}
		}

		// Read lightmap RGB data (if available)
		if(lm_rgb_id != -1) {
			data.lm_rgb = malloc(sizes[lm_rgb_id]);

			fseek(pF, offsets[lm_rgb_id], SEEK_SET);
			fread(data.lm_rgb, 1, sizes[lm_rgb_id], pF);
		}

		// Read structural lightmap data (if available)
		if(lm_decoupled_id != -1) {
			int num_entries = sizes[lm_decoupled_id] / sizeof(Lm_Decoupled);
			data.decouple_lm = malloc(sizes[lm_decoupled_id]);
			fseek(pF, offsets[lm_decoupled_id], SEEK_SET);
			fread(data.decouple_lm, 1, sizes[lm_decoupled_id], pF);
		}

		// Read lightgrid octree data (if available)
		if(lm_grid_id != -1) {
			int num_entries = sizes[lm_grid_id] / sizeof(lm_OctreeNode);
		}
	}
	// ---------------------------------------------------------------------------------------

	// Close and return data
	fclose(pF);

	BspRenderSetup(&data);

	if(GetLogState())
		printf("bsp submodel count: %d\n", data.num_models);

	data.hull_groups = malloc(sizeof(Bsp_HullGroup) * data.num_models);
	for(int i = 0; i < data.num_models; i++) {
		data.hull_groups[i] = Bsp_BuildHullGroup(&data, i);
	}

	return data;
}

void UnloadBsp(Bsp_Data *data) {
	if(data->ent_str)		free(data->ent_str);
	if(data->planes)		free(data->planes);
	if(data->miptex)		free(data->miptex);
	if(data->verts)			free(data->verts);
	if(data->vis)			free(data->vis);
	if(data->nodes)			free(data->nodes);
	if(data->clipnodes)		free(data->clipnodes);
	if(data->edges)			free(data->edges);
	if(data->ledges)		free(data->ledges);
	if(data->faces)			free(data->faces);
	if(data->surfaces)		free(data->surfaces);
	if(data->models)		free(data->models);

	if(data->textures) {
		for(int i = 0; i < data->num_miptex; i++)
			UnloadTexture(data->textures[i]);

		free(data->textures);
	}

	UnloadShader(data->lm_shader);

	if(data->lm_rgb)
		free(data->lm_rgb);

	if(data->lm.uvs)
		free(data->lm.uvs);

	if(data->lm_oct_nodes)
		free(data->lm_oct_nodes);
}

Bsp_Hull Bsp_BuildHull(Bsp_Data *data, int hull_index) {
	Bsp_Hull hull = (Bsp_Hull) {0};

	hull.nodes = data->clipnodes;
	hull.first_node = data->models[0].head_nodes[hull_index];
	hull.last_node = data->num_clipnodes - 1;

	hull.planes = data->planes;

	return hull;
}

Bsp_HullGroup Bsp_BuildHullGroup(Bsp_Data *data, int model_id) {
	Bsp_HullGroup group = (Bsp_HullGroup) {0};	
	group.flags |= HULLGROUP_ACTIVE;

	for(int i = 0; i < 4; i++) {
		Bsp_Hull hull = (Bsp_Hull) {0};

		hull.nodes = data->clipnodes;
		hull.planes = data->planes;

		hull.first_node = data->models[model_id].head_nodes[i];
		hull.last_node = data->num_clipnodes - 1;

		group.hulls[i] = hull;
	}	

	return group;
}

int Bsp_PointContents(Bsp_Hull *hull, int num, Vector3 point) {
	float d;
	Bsp_ClipNode *node;
	Bsp_Plane *plane;

	while(num >= 0) {
		if(num < hull->first_node || num > hull->last_node) {
			MessageError("Bsp_PointContents()", "bad node number");
			printf("node_num: %d\n", num);
			return 0;
		}

		node = &hull->nodes[num];
		plane = &hull->planes[node->planenum];

		Vector3 normal = (Vector3) { plane->normal[0], plane->normal[1], plane->normal[2] };
		d = Vector3DotProduct(normal, point) - plane->dist;

		if(d < 0)
			num = node->children[1];
		else 
			num = node->children[0];
	}

	return num;
}

bool Bsp_RecursiveTrace(Bsp_Hull *hull, int node_num, Vector3 point_A, Vector3 point_B, Vector3 *intersection) {
	Bsp_ClipNode *node;
	Bsp_Plane *plane;
	float tA, tB;
	float fraction;
	
	// Handle leaves	
	if(node_num < 0) {
		if(node_num == CONTENTS_SOLID) {
			*intersection = point_A;
			return true;
		}
		return false;
	}

	node = &hull->nodes[node_num];
	plane = &hull->planes[node->planenum];

	tA = Vector3DotProduct( (Vector3) { plane->normal[0], plane->normal[1], plane->normal[2] }, point_A ) - plane->dist;	
	tB = Vector3DotProduct( (Vector3) { plane->normal[0], plane->normal[1], plane->normal[2] }, point_B ) - plane->dist;	

	// Handle cases where the line falls entirely within a single child
	if(tA >= 0 && tB >= 0)
		return Bsp_RecursiveTrace(hull, node->children[0], point_A, point_B, intersection);

	if(tA < 0 && tB < 0)
		return Bsp_RecursiveTrace(hull, node->children[1], point_A, point_B, intersection);

	// Find point of intersection with split plane
	fraction = tA / (tA - tB);
	fraction = Clamp(fraction, 0.0f, 1.0f);

	float3 m = {0};
	float3 a = Vector3ToFloatV(point_A);
	float3 b = Vector3ToFloatV(point_B);

	for(short i = 0; i < 3; i++)
		m.v[i] = a.v[i] + fraction*(b.v[i] - a.v[i]);

	Vector3 mid = *(Vector3 *) m.v;

	short side = (tA >= 0) ? 0 : 1;
	if(Bsp_RecursiveTrace(hull, node->children[side], point_A, mid, intersection))
		return true;

	return Bsp_RecursiveTrace(hull, node->children[1 - side], mid, point_B, intersection);
}

#define	DIST_EPSILON	(0.03125)
Bsp_TraceData Bsp_TraceDataEmpty() {
	Bsp_TraceData data = {0};
	data.all_solid = true;
	data.fraction = 1;
	return data;
}

bool Bsp_RecursiveTraceEx(Bsp_Hull *hull, int node_num, float p1_frac, float p2_frac, Vector3 p1, Vector3 p2, Bsp_TraceData *trace) {
	Bsp_ClipNode *node;
	Bsp_Plane *plane;
	float t1, t2;
	Vector3 mid;
	int side;
	float mid_frac;
	float frac;

	// Check for empty
	if(node_num < 0) {
		if(node_num != CONTENTS_SOLID) {
			trace->all_solid = false;

			if(node_num == CONTENTS_EMPTY)
				trace->in_open = true;
			else 	
				trace->in_water = true;
		} else 
			trace->start_solid = true;

		return true;	// Empty
	}

	if(node_num < hull->first_node || node_num > hull->last_node) {
		MessageError("Bsp_RecursiveTraceEx", "bad node number");
		if(GetLogState()) printf("node_num: %d\n", node_num);
		return true;
	}

	node = &hull->nodes[node_num];
	plane = &hull->planes[node->planenum];

	Vector3 norm = *(Vector3 *) plane->normal;
	if(plane->type < 3) {
		float3 p1_f3 = Vector3ToFloatV(p1);
		float3 p2_f3 = Vector3ToFloatV(p2);

		t1 =  p1_f3.v[plane->type] - plane->dist;
		t2 =  p2_f3.v[plane->type] - plane->dist;

		p1 = *(Vector3 *) p1_f3.v;
		p2 = *(Vector3 *) p2_f3.v;

	} else {

		t1 = Vector3DotProduct(norm, p1) - plane->dist;
		t2 = Vector3DotProduct(norm, p2) - plane->dist;
	}

	if(t1 >= 0 && t2 >= 0)
		return Bsp_RecursiveTraceEx(hull, node->children[0], p1_frac, p2_frac, p1, p2, trace);
	if(t1 < 0 && t2 < 0)
		return Bsp_RecursiveTraceEx(hull, node->children[1], p1_frac, p2_frac, p1, p2, trace);

	if(t1 < 0)
		frac = (t1 + DIST_EPSILON) / (t1 - t2);
	else 
		frac = (t1 - DIST_EPSILON) / (t1 - t2);

	if(frac < 0)
		frac = 0;

	if(frac > 1)
		frac = 1;

	mid_frac = p1_frac + (p2_frac - p1_frac) * frac; 

	float3 m = {0};
	float3 p1_f3 = Vector3ToFloatV(p1);
	float3 p2_f3 = Vector3ToFloatV(p2);
	for(short i = 0; i < 3; i++) 
		m.v[i] = p1_f3.v[i] + frac*(p2_f3.v[i] - p1_f3.v[i]);

	mid = *(Vector3 *) m.v;

	side = (t1 < 0);

	// Move up to node
	if(!Bsp_RecursiveTraceEx(hull, node->children[side], p1_frac, mid_frac, p1, mid, trace))
		return false;

	// Go past node
	if(Bsp_PointContents(hull, node->children[side^1], mid) != CONTENTS_SOLID)
		return Bsp_RecursiveTraceEx(hull, node->children[side^1], mid_frac, p2_frac, mid, p2, trace);

	// Never go out of solid area
	if(trace->all_solid)
		return false;

	if(!side) {
		memcpy(trace->plane.normal, plane->normal, sizeof(float) * 3);
		trace->plane.dist = plane->dist;

	} else {
		for(short i = 0; i < 3; i++)
			trace->plane.normal[i] = -plane->normal[i];
		
		trace->plane.dist = -plane->dist;
	}

	// Shouldn't happen but does sometimes
	while(Bsp_PointContents(hull, node_num, mid) == CONTENTS_SOLID) {
		frac -= 0.1f;

		if(frac < 0) {
			trace->fraction = mid_frac;
			trace->point = mid;
			return false;
		}

		mid_frac = p1_frac + (p2_frac - p1_frac) * frac;

		float3 m = {0};
		float3 p1_f3 = Vector3ToFloatV(p1);
		float3 p2_f3 = Vector3ToFloatV(p2);
		for(short i = 0; i < 3; i++) 
			m.v[i] = p1_f3.v[i] + frac*(p2_f3.v[i] - p1_f3.v[i]);

		mid = *(Vector3 *) m.v;
	}

	trace->fraction = mid_frac;
	trace->point = mid;

	return false;
}

int Bsp_FindLeaf(Bsp_Data *bsp, Vector3 point) {
	int node_num = bsp->models[0].head_nodes[0];

	while(node_num >= 0) {
		Bsp_Node *node = &bsp->nodes[node_num];
		Bsp_Plane *plane = &bsp->planes[node->planenum];

		Vector3 normal = *(Vector3 *) plane->normal;
		float d = Vector3DotProduct(normal, point) - plane->dist;

		node_num = (d >= 0) ? node->children[0] : node->children[1];
	}
	return ~node_num;
}

bool Bsp_LeafVisible(Bsp_Data *bsp, int curr_leaf, int test_leaf) {
	if(curr_leaf == test_leaf)
		return true;

	Bsp_Leaf *leaf = &bsp->leaves[curr_leaf];

	// No vis data, default to drawing
	if(leaf->visofs < 0)
		return true;

	u8 *vis = bsp->vis + leaf->visofs;

	// Decompress and test bitmask
	int leafnum = 1;
	while(leafnum < bsp->num_leaves) {
		if(*vis == 0) {
			// Skip
			vis++;
			leafnum += *vis * 8;
			vis++;

		} else {
			// Test each bit in byte
			for(int bit = 0; bit < 8; bit++) {
				if(leafnum == test_leaf) {
					return (*vis >> bit) & 1;
				}

				leafnum++;
			}

			vis++;
		}
	}

	return false;
}

void Bsp_PrintStructSizes() {
	printf("box: %zu bytes\n", sizeof(Bsp_Box32));
	printf("face: %zu bytes\n", sizeof(Bsp_Face));
	printf("leaf: %zu bytes\n", sizeof(Bsp_Leaf));
	printf("miptex: %zu bytes\n", sizeof(Bsp_Miptex));
}

Model *BspLeafToModels(Bsp_Data *bsp, Bsp_Leaf *leaf, int *out_count) {
	int max_tex_id = 0; 
	for(int i = 0; i < leaf->num_faces; i++) {
		int face_id = bsp->lfaces[leaf->first_face + i];	
		Bsp_Face *face = &bsp->faces[face_id];

		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		if(surface->texture_id > max_tex_id)
			max_tex_id = surface->texture_id;
	}

	int tex_count = max_tex_id + 1;
	int *tri_counts = calloc(tex_count, sizeof(int));

	for(int i = 0; i < leaf->num_faces; i++) {
		int face_id = bsp->lfaces[leaf->first_face + i];	
		Bsp_Face *face = &bsp->faces[face_id];

		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		tri_counts[surface->texture_id] += face->edge_count - 2;
	}

	int used = 0;	
	for(int i = 0; i < tex_count; i++) {
		if(tri_counts[i] > 0)
			used++;
	}

	*out_count = used;
	if(used == 0) {
		free(tri_counts);
		return NULL;
	}

	int *tex_to_slot = malloc(sizeof(int) * tex_count);
	for(int i = 0; i < tex_count; i++)
		tex_to_slot[i] = -1;

	Mesh *meshes = calloc(used, sizeof(Mesh));
	int *tex_slot_ids = malloc(used * sizeof(int));
	int slot = 0;

	for(int i = 0; i < tex_count; i++) {
		if(tri_counts[i] <= 0)
			continue;

		tex_to_slot[i] = slot;
		tex_slot_ids[slot] = i;

		Mesh *mesh = &meshes[slot];

		mesh->triangleCount = tri_counts[i];
		mesh->vertexCount 	= tri_counts[i]*3;

		mesh->vertices 		= MemAlloc(sizeof(float) * mesh->vertexCount * 3);
		mesh->normals		= MemAlloc(sizeof(float) * mesh->vertexCount * 3);
		mesh->texcoords 	= MemAlloc(sizeof(float) * mesh->vertexCount * 2);
		mesh->texcoords2 	= MemAlloc(sizeof(float) * mesh->vertexCount * 2);

		slot++;
	}

	int *cursors = calloc(used, sizeof(int));

	for(int i = 0; i < leaf->num_faces; i++) {
		int face_id = bsp->lfaces[leaf->first_face + i];
		Bsp_Face *face = &bsp->faces[face_id];

		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		Bsp_Miptex *mip = &bsp->miptex[surface->texture_id];

		Lm_Decoupled *lmd = &bsp->decouple_lm[face_id];
		Rectangle *uv_rec = &bsp->lm.uvs[face_id];

		int _slot = tex_to_slot[surface->texture_id];
		Mesh *mesh = &meshes[_slot];
		int *vert_id = &cursors[_slot]; 

		Vector3 face_verts[face->edge_count]; 
		for(int j = 0; j < face->edge_count; j++) {
			i32 list_edge = bsp->ledges[face->first_edge + j];
			face_verts[j] = (list_edge >= 0) ? bsp->verts[bsp->edges[list_edge].v[0]] : bsp->verts[bsp->edges[-list_edge].v[1]]; 
		}

		Bsp_Plane *plane = &bsp->planes[face->plane];
		Vector3 normal = *(Vector3 *) plane->normal;
		if(face->side)
			normal = Vector3Negate(normal);

		for(int j = 1; j < face->edge_count - 1; j++) {
			Tri tri = (Tri) { .normal = normal, .vertices = { face_verts[0], face_verts[j+1], face_verts[j] } };

			for(int k = 0; k < 3; k++) {
				// Vertex
				mesh->vertices[*vert_id*3+0] = tri.vertices[k].x;
				mesh->vertices[*vert_id*3+1] = tri.vertices[k].y;
				mesh->vertices[*vert_id*3+2] = tri.vertices[k].z;

				// Normal
				mesh->normals[*vert_id*3+0]	= tri.normal.x;
				mesh->normals[*vert_id*3+1]	= tri.normal.y;
				mesh->normals[*vert_id*3+2]	= tri.normal.z;

				// Texture UV
				mesh->texcoords[*vert_id*2+0] = (Vector3DotProduct(tri.vertices[k], surface->vector_s) + surface->dist_s) / mip->width; 
				mesh->texcoords[*vert_id*2+1] = (Vector3DotProduct(tri.vertices[k], surface->vector_t) + surface->dist_t) / mip->height;

				// Lighmap UV
				if(lmd->w <= 0 || lmd->h <= 0) {
					(*vert_id)++;
					continue;
				}


				float lm_u = Vector3DotProduct(tri.vertices[k], lmd->vs) + lmd->dist_s;
				float lm_v = Vector3DotProduct(tri.vertices[k], lmd->vt) + lmd->dist_t;
				mesh->texcoords2[*vert_id*2+0] = (uv_rec->x + lm_u + 0.5f) / bsp->lm.tex.width;
				mesh->texcoords2[*vert_id*2+1] = (uv_rec->y + lm_v + 0.5f) / bsp->lm.tex.height;

				(*vert_id)++;
			}
		}
	}

	Model *models = malloc(sizeof(Model) * used);
	for(int i = 0; i < used; i++) {
		int tex_id = tex_slot_ids[i];

		if(tex_id >= bsp->num_miptex) 
			continue;

		UploadMesh(&meshes[i], false);
		models[i] = LoadModelFromMesh(meshes[i]);

		Texture2D mat_texture = materials[HashFetch(&material_hashmap, bsp->miptex[tex_id].name)].maps->texture;
		models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = mat_texture;
		char pref[3];
		memcpy(pref, bsp->miptex[tex_id].name, sizeof(pref));

		if(strcmp(pref, "sky") == 0) {
			continue;
		}

		if(strcmp(pref, "{ff") == 0) {
			models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTextureFromImage(GenImageColor(512, 512, ColorAlpha(BLUE, 0.5)));
			continue;
		}

		models[i].materials[0].maps[MATERIAL_MAP_METALNESS].texture = bsp->lm.tex;
		models[i].materials[0].shader = bsp->lm_shader;
	}

	free(tri_counts);
	free(tex_to_slot);
	free(tex_slot_ids);
	free(cursors);
	free(meshes);

	return models;
}

RenderBrush *BspLeafToRenderBrushes(Bsp_Data *bsp, Bsp_Leaf *leaf, int *out_count) {
	int max_tex_id = 0; 
	for(int i = 0; i < leaf->num_faces; i++) {
		int face_id = bsp->lfaces[leaf->first_face + i];	
		Bsp_Face *face = &bsp->faces[face_id];

		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		if(surface->texture_id > max_tex_id)
			max_tex_id = surface->texture_id;
	}

	int tex_count = max_tex_id + 1;
	int *tri_counts = calloc(tex_count, sizeof(int));

	for(int i = 0; i < leaf->num_faces; i++) {
		int face_id = bsp->lfaces[leaf->first_face + i];	
		Bsp_Face *face = &bsp->faces[face_id];

		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		tri_counts[surface->texture_id] += face->edge_count - 2;
	}

	int used = 0;	
	for(int i = 0; i < tex_count; i++) {
		if(tri_counts[i] > 0)
			used++;
	}

	*out_count = used;
	if(used == 0) {
		free(tri_counts);
		return NULL;
	}

	int *tex_to_slot = malloc(sizeof(int) * tex_count);
	for(int i = 0; i < tex_count; i++)
		tex_to_slot[i] = -1;

	Mesh *meshes = calloc(used, sizeof(Mesh));
	int *tex_slot_ids = malloc(used * sizeof(int));
	int slot = 0;

	for(int i = 0; i < tex_count; i++) {
		if(tri_counts[i] <= 0)
			continue;

		tex_to_slot[i] = slot;
		tex_slot_ids[slot] = i;

		Mesh *mesh = &meshes[slot];

		mesh->triangleCount = tri_counts[i];
		mesh->vertexCount 	= tri_counts[i]*3;

		mesh->vertices 		= MemAlloc(sizeof(float) * mesh->vertexCount * 3);
		mesh->normals		= MemAlloc(sizeof(float) * mesh->vertexCount * 3);
		mesh->texcoords 	= MemAlloc(sizeof(float) * mesh->vertexCount * 2);
		mesh->texcoords2 	= MemAlloc(sizeof(float) * mesh->vertexCount * 2);

		slot++;
	}

	int *cursors = calloc(used, sizeof(int));

	for(int i = 0; i < leaf->num_faces; i++) {
		int face_id = bsp->lfaces[leaf->first_face + i];
		Bsp_Face *face = &bsp->faces[face_id];

		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		Bsp_Miptex *mip = &bsp->miptex[surface->texture_id];

		Lm_Decoupled *lmd = &bsp->decouple_lm[face_id];
		Rectangle *uv_rec = &bsp->lm.uvs[face_id];

		int _slot = tex_to_slot[surface->texture_id];
		Mesh *mesh = &meshes[_slot];
		int *vert_id = &cursors[_slot]; 

		Vector3 face_verts[face->edge_count]; 
		for(int j = 0; j < face->edge_count; j++) {
			i32 list_edge = bsp->ledges[face->first_edge + j];
			face_verts[j] = (list_edge >= 0) ? bsp->verts[bsp->edges[list_edge].v[0]] : bsp->verts[bsp->edges[-list_edge].v[1]]; 
		}

		Bsp_Plane *plane = &bsp->planes[face->plane];
		Vector3 normal = *(Vector3 *) plane->normal;
		if(face->side)
			normal = Vector3Negate(normal);

		for(int j = 1; j < face->edge_count - 1; j++) {
			Tri tri = (Tri) { .normal = normal, .vertices = { face_verts[0], face_verts[j+1], face_verts[j] } };

			for(int k = 0; k < 3; k++) {
				// Vertex
				mesh->vertices[*vert_id*3+0] = tri.vertices[k].x;
				mesh->vertices[*vert_id*3+1] = tri.vertices[k].y;
				mesh->vertices[*vert_id*3+2] = tri.vertices[k].z;

				// Normal
				mesh->normals[*vert_id*3+0]	= tri.normal.x;
				mesh->normals[*vert_id*3+1]	= tri.normal.y;
				mesh->normals[*vert_id*3+2]	= tri.normal.z;

				// Texture UV
				mesh->texcoords[*vert_id*2+0] = (Vector3DotProduct(tri.vertices[k], surface->vector_s) + surface->dist_s) / mip->width; 
				mesh->texcoords[*vert_id*2+1] = (Vector3DotProduct(tri.vertices[k], surface->vector_t) + surface->dist_t) / mip->height;

				// Lighmap UV
				if(lmd->w <= 0 || lmd->h <= 0) {
					(*vert_id)++;
					continue;
				}


				float lm_u = Vector3DotProduct(tri.vertices[k], lmd->vs) + lmd->dist_s;
				float lm_v = Vector3DotProduct(tri.vertices[k], lmd->vt) + lmd->dist_t;
				mesh->texcoords2[*vert_id*2+0] = (uv_rec->x + lm_u + 0.5f) / bsp->lm.tex.width;
				mesh->texcoords2[*vert_id*2+1] = (uv_rec->y + lm_v + 0.5f) / bsp->lm.tex.height;

				(*vert_id)++;
			}
		}
	}

	RenderBrush *render_brushes = malloc(sizeof(RenderBrush) * used);

	for(int i = 0; i < used; i++) {
		render_brushes[i] = (RenderBrush) {0};

		int tex_id = tex_slot_ids[i];

		if(tex_id >= bsp->num_miptex) 
			continue;

		UploadMesh(&meshes[i], false);
		render_brushes[i].model = LoadModelFromMesh(meshes[i]);

		char pref[3];
		memcpy(pref, bsp->miptex[tex_id].name, sizeof(pref));

		if(pref[0] == '{') {
			render_brushes[i].flags |= RBRUSH_TRANSLUCENT;
			//MessageDiag("Made translucent render brush", NULL, ANSI_YELLOW);

			//Texture2D tex = LoadTextureFromImage(GenImageColor(1, 1, ColorAlpha(WHITE, 0.0f)));
			//continue;
		}

		Texture2D mat_texture = materials[HashFetch(&material_hashmap, bsp->miptex[tex_id].name)].maps->texture;
		render_brushes[i].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = mat_texture;

		if(strcmp(pref, "sky") == 0) {
			continue;
		}

		/*
		if(strcmp(pref, "{ff") == 0) {
			render_brushes[i].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture =
				LoadTextureFromImage(GenImageColor(1, 1, ColorAlpha(BLUE, 0.5)));

			render_brushes[i].flags |= RBRUSH_FORCEFIELD;
			render_brushes[i].model.materials[0].shader = bsp->ff_shader; 

			MessageDiag("Made force field render brush", NULL, ANSI_YELLOW);

			continue;
		}
		*/

		render_brushes[i].model.materials[0].maps[1].texture = bsp->lm.tex;
		render_brushes[i].model.materials[0].shader = bsp->lm_shader;
	}

	free(tri_counts);
	free(tex_to_slot);
	free(tex_slot_ids);
	free(cursors);
	free(meshes);

	return render_brushes;
}

Model BspModelToRenderModel(Bsp_Data *bsp, int submodel_id) {
	Model model = (Model) {0};

	Bsp_Model *bsp_m = &bsp->models[submodel_id];

	int tri_count = 0;
	for(int i = 0; i < bsp_m->num_faces; i++) {
		Bsp_Face *face = &bsp->faces[bsp_m->first_face + i];
		tri_count += face->edge_count -2;
	}

	Mesh mesh = (Mesh) {0};

	mesh.triangleCount = tri_count;
	mesh.vertexCount = tri_count * 3;

	mesh.vertices 	= MemAlloc(sizeof(float) * mesh.vertexCount * 3);
	mesh.normals 	= MemAlloc(sizeof(float) * mesh.vertexCount * 3);
	mesh.texcoords 	= MemAlloc(sizeof(float) * mesh.vertexCount * 2);

	int vert_id = 0;
	for(int i = 0; i < bsp_m->num_faces; i++) {
		Bsp_Face *face = &bsp->faces[bsp_m->first_face + i];
		Bsp_Surface *surface = &bsp->surfaces[face->texinfo];
		Bsp_Miptex *mip = &bsp->miptex[surface->texture_id];

		Vector3 face_verts[face->edge_count];
		for(int j = 0; j < face->edge_count; j++) {
			i32 list_edge = bsp->ledges[face->first_edge + j];
			face_verts[j] = (list_edge >= 0) ? bsp->verts[bsp->edges[list_edge].v[0]] : bsp->verts[bsp->edges[-list_edge].v[1]]; 
		}

		Bsp_Plane *plane = &bsp->planes[face->plane];
		Vector3 normal = *(Vector3 *) plane->normal;
		if(face->side) normal = Vector3Negate(normal); 

		for(int j = 1; j < face->edge_count - 1; j++) {
			Vector3 tri[3] = { face_verts[0], face_verts[j + 1], face_verts[j] };

			for(short k = 0; k < 3; k++) {
				// Vertex
				mesh.vertices[vert_id*3+0] = tri[k].x;
				mesh.vertices[vert_id*3+1] = tri[k].y;
				mesh.vertices[vert_id*3+2] = tri[k].z;

				// Normal
				mesh.normals[vert_id*3+0]	= normal.x;
				mesh.normals[vert_id*3+1]	= normal.y;
				mesh.normals[vert_id*3+2]	= normal.z;

				// Texture UV
				mesh.texcoords[vert_id*2+0] = (Vector3DotProduct(tri[k], surface->vector_s) + surface->dist_s) / mip->width; 
				mesh.texcoords[vert_id*2+1] = (Vector3DotProduct(tri[k], surface->vector_t) + surface->dist_t) / mip->height;

				vert_id++;
			}
		}
	}

	UploadMesh(&mesh, false);
	model = LoadModelFromMesh(mesh);

	Bsp_Face *first_face = &bsp->faces[bsp_m->first_face];
	Bsp_Surface *surface = &bsp->surfaces[first_face->texinfo];
	Bsp_Miptex *mip = &bsp->miptex[surface->texture_id];
	int tex_id = HashFetch(&material_hashmap, mip->name);
	model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = materials[tex_id].maps[MATERIAL_MAP_DIFFUSE].texture;

	char pref[3];
	memcpy(pref, mip->name, sizeof(pref));
	
	if(strcmp(pref, "{ff") == 0) {
		model.materials[0].shader = bsp->ff_shader;
	}

	return model;
}

void BspRenderSetup(Bsp_Data *bsp) {
	bsp->lm_shader = LoadShader("resources/shaders/lit_v.glsl", "resources/shaders/lit_f.glsl");
	bsp->ff_shader = LoadShader("resources/shaders/lit_v.glsl", "resources/shaders/force_field_f.glsl");
	bsp->ff_locs[0] = GetShaderLocation(bsp->ff_shader, "time");

	FilePathList mat_list = LoadDirectoryFiles("tools/Disruptor/textures/custom");	

	materials = malloc(sizeof(Material) * mat_list.count); 
	textures = malloc(sizeof(Texture2D) * mat_list.count); 
	for(int i = 0; i < mat_list.count; i++) {
		char path[255] = {0};
		memcpy(path, mat_list.paths[i], strlen(mat_list.paths[i]));

		char *sep_f = strrchr(path, '/');
		char *sep_b = strrchr(path, '\\');
		char *sep = (sep_f > sep_b) ? sep_f : sep_b;

		*sep = '\0';

		char *format = sep + 1;
		char *dot = strrchr(format, '.');
		*dot = '\0';

		HashInsert(&material_hashmap, format, i);

		textures[i] = LoadTexture(mat_list.paths[i]);
		SetTextureFilter(textures[i], TEXTURE_FILTER_POINT);

		materials[i] = LoadMaterialDefault();
		materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = textures[i];
		materials[i].params[0] = 1;
	}

	//DisplayNodes(&material_hashmap);
}

void UpdateBspShaders(Bsp_Data *bsp) {
	float t = GetTime();
	SetShaderValue(bsp->ff_shader, bsp->ff_locs[0], &t, SHADER_UNIFORM_FLOAT);
}

