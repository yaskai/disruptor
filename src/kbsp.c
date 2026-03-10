#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"
#include "kbsp.h"
#include "../include/log_message.h"

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

	if(header.version != BSP_VERSION) {
		MessageError("ERROR: BSP version mismatch", NULL);
		return data;
	}

	if(print_output)
		printf("%d\n", header.version);
	// ---------------------------------------------------------------------------------------
	// Planes
	Bsp_Lump planes_lump = header.lumps[LUMP_PLANES];

	fseek(pF, planes_lump.file_offset, SEEK_SET);
	i32 plane_count = planes_lump.file_size / sizeof(Bsp_Plane);  
	Bsp_Plane planes[plane_count];
	fread(&planes, sizeof(planes), 1, pF);

	data.num_planes = plane_count;
	data.planes = malloc(sizeof(Bsp_Plane) * data.num_planes);
	memcpy(data.planes, planes, sizeof(planes));
	// ---------------------------------------------------------------------------------------
	// Miptex
	Bsp_Lump miptex_lump = header.lumps[LUMP_MIPTEX];

	fseek(pF, miptex_lump.file_offset, SEEK_SET);
	i32 miptex_count = miptex_lump.file_size / sizeof(Bsp_Miptex);  
	Bsp_Miptex miptexes[miptex_count];
	fread(&miptexes, sizeof(miptexes), 1, pF);

	data.num_miptex = miptex_count;
	data.miptex = malloc(sizeof(Bsp_Miptex) * data.num_miptex);
	memcpy(data.miptex, miptexes, sizeof(miptexes));
	// ---------------------------------------------------------------------------------------
	// Vertices
	Bsp_Lump vert_lump = header.lumps[LUMP_VERTICES];

	fseek(pF, vert_lump.file_offset, SEEK_SET);
	i32 vert_count = vert_lump.file_size / sizeof(Vector3);  
	Vector3 verts[vert_count];
	fread(&verts, sizeof(verts), 1, pF);

	data.num_verts = vert_count;
	data.verts = malloc(sizeof(Vector3) * data.num_verts);
	memcpy(data.verts, verts, sizeof(verts));
	// ---------------------------------------------------------------------------------------
	// Vis

	// ---------------------------------------------------------------------------------------
	// Nodes
	Bsp_Lump nodes_lump = header.lumps[LUMP_NODES];

	fseek(pF, nodes_lump.file_offset, SEEK_SET);
	i32 node_count = nodes_lump.file_size / sizeof(Bsp_Node);
	Bsp_Node nodes[node_count];
	fread(&nodes, sizeof(nodes), 1, pF);

	data.num_nodes = node_count;
	data.nodes = malloc(sizeof(Bsp_Node) * data.num_nodes);
	memcpy(data.nodes, nodes, sizeof(nodes));
	// ---------------------------------------------------------------------------------------
	// Tex info
	Bsp_Lump texinfo_lump = header.lumps[LUMP_TEXINFO];

	fseek(pF, texinfo_lump.file_offset, SEEK_SET);
	i32 texinfo_count = texinfo_lump.file_size / sizeof(Bsp_Surface);
	Bsp_Surface texinfos[texinfo_count];
	fread(&texinfos, sizeof(texinfos), 1, pF);

	data.num_surfaces = texinfo_count;
	data.surfaces = malloc(sizeof(Bsp_Surface) * data.num_surfaces);
	memcpy(data.surfaces, texinfos, sizeof(texinfos));	
	// ---------------------------------------------------------------------------------------
	// Faces 
	Bsp_Lump faces_lump = header.lumps[LUMP_FACES];

	fseek(pF, faces_lump.file_offset, SEEK_SET);
	i32 face_count = faces_lump.file_size / sizeof(Bsp_Face);
	Bsp_Face faces[face_count];
	fread(&faces, sizeof(faces), 1, pF);

	data.num_faces = face_count;
	data.faces = malloc(sizeof(Bsp_Face) * data.num_faces);
	memcpy(data.faces, faces, sizeof(faces));
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
	Bsp_ClipNode clipnodes[clipnode_count];
	fread(&clipnodes, sizeof(clipnodes), 1, pF);

	data.num_clipnodes = clipnode_count;
	data.clipnodes = malloc(sizeof(Bsp_ClipNode) * data.num_clipnodes);
	memcpy(data.clipnodes, clipnodes, sizeof(clipnodes));
	// ---------------------------------------------------------------------------------------
	// Leaves 
	Bsp_Lump leaves_lump = header.lumps[LUMP_LEAVES];

	fseek(pF, leaves_lump.file_offset, SEEK_SET);
	i32 leaf_count = leaves_lump.file_size / sizeof(Bsp_Leaf);
	Bsp_Leaf leaves[leaf_count];
	fread(&leaves, sizeof(leaves), 1, pF);

	data.num_leaves = leaf_count;
	data.leaves = malloc(sizeof(Bsp_Leaf) * data.num_leaves);
	memcpy(data.leaves, leaves, sizeof(leaves));
	// ---------------------------------------------------------------------------------------
	// L_faces 
	Bsp_Lump lfaces_lump = header.lumps[LUMP_LFACES];	
	
	fseek(pF, lfaces_lump.file_offset, SEEK_SET);
	i32 lfaces_count = lfaces_lump.file_size / sizeof(Bsp_LFaces);
	Bsp_LFaces lfaces[lfaces_count];
	fread(&lfaces, sizeof(lfaces), 1, pF);

	data.lfaces.num_lface = lfaces_count;
	data.lfaces.faces = malloc(sizeof(Bsp_LFaces) * data.lfaces.num_lface);
	memcpy(data.lfaces.faces, lfaces, sizeof(lfaces));
	// ---------------------------------------------------------------------------------------
	// Edges 
	Bsp_Lump edges_lump = header.lumps[LUMP_EDGES];

	fseek(pF, edges_lump.file_offset, SEEK_SET);
	i32 edge_count = edges_lump.file_size / sizeof(Bsp_Edge);
	Bsp_Edge edges[edge_count];
	fread(&edges, sizeof(edges), 1, pF);

	data.num_edges = edge_count;
	data.edges = malloc(sizeof(Bsp_Edge) * data.num_edges);
	memcpy(data.edges, edges, sizeof(edges));	
	// ---------------------------------------------------------------------------------------
	// L_edges 
	Bsp_Lump ledges_lump = header.lumps[LUMP_L_EDGES];

	fseek(pF, ledges_lump.file_offset, SEEK_SET);
	i32 ledge_count = ledges_lump.file_size / sizeof(Bsp_LEdges);
	Bsp_LEdges ledges[ledge_count];
	fread(&ledges, sizeof(ledges), 1, pF);

	data.num_ledges = ledge_count;
	data.ledges.edge = malloc(sizeof(Bsp_LEdges) * data.num_ledges);
	memcpy(data.ledges.edge, ledges, sizeof(ledges));	
	// ---------------------------------------------------------------------------------------
	// Models
	Bsp_Lump models_lump = header.lumps[LUMP_MODELS];
	fseek(pF, models_lump.file_offset, SEEK_SET);

	i32 model_count = models_lump.file_size / sizeof(Bsp_Model);
	Bsp_Model models[model_count];
	fread(&models, sizeof(models), 1, pF);

	data.num_models = model_count;
	data.models = malloc(sizeof(Bsp_Model) * data.num_models);
	memcpy(data.models, models, sizeof(models));
	// ---------------------------------------------------------------------------------------

	// Close and return data
	fclose(pF);
	return data;
}

void UnloadBsp(Bsp_Data *data) {
	if(data->planes)		free(data->planes);
	if(data->miptex)		free(data->miptex);
	if(data->verts)			free(data->verts);
	if(data->vis)			free(data->vis);
	if(data->nodes)			free(data->nodes);
	if(data->clipnodes)		free(data->clipnodes);
	if(data->edges)			free(data->edges);
	if(data->ledges.edge)	free(data->ledges.edge);
	if(data->faces)			free(data->faces);
	if(data->surfaces)		free(data->surfaces);
	if(data->models)		free(data->models);
}

Bsp_Hull Bsp_BuildHull(Bsp_Data *data, int hull_index) {
	Bsp_Hull hull = (Bsp_Hull) {0};

	hull.nodes = data->clipnodes;
	hull.first_node = data->models[0].head_nodes[hull_index];
	hull.last_node = data->num_clipnodes - 1;

	hull.nodes = data->clipnodes;
	hull.planes = data->planes;

	return hull;
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

	Vector3 mid = (Vector3) { m.v[0], m.v[1], m.v[2] };

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
		printf("node_num: %d\n", node_num);
		return true;
	}

	node = &hull->nodes[node_num];
	plane = &hull->planes[node->planenum];

	Vector3 norm = (Vector3) { plane->normal[0], plane->normal[1], plane->normal[2] };
	if(plane->type < 3) {
		float3 p1_f3 = Vector3ToFloatV(p1);
		float3 p2_f3 = Vector3ToFloatV(p2);

		t1 =  p1_f3.v[plane->type] - plane->dist;
		t2 =  p2_f3.v[plane->type] - plane->dist;

		p1 = (Vector3) { p1_f3.v[0], p1_f3.v[1], p1_f3.v[2] };
		p2 = (Vector3) { p2_f3.v[0], p2_f3.v[1], p2_f3.v[2] };

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

	mid = (Vector3) { m.v[0], m.v[1], m.v[2] };

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

		mid = (Vector3) { m.v[0], m.v[1], m.v[2] };
	}

	trace->fraction = mid_frac;
	trace->point = mid;

	return false;
}

Mesh BspLeafToMesh(Bsp_Data *bsp, Bsp_Leaf *leaf) {
	Mesh mesh = (Mesh) {0};

	u16 tri_count = 0;
	for(u16 i = 0; i < leaf->num_faces; i++) {
		u16 face_id = bsp->lfaces.faces[leaf->first_face + i];
		Bsp_Face *face = &bsp->faces[face_id];
		tri_count += face->edge_count - 2;
	}

	mesh.triangleCount = tri_count;
	mesh.vertexCount = tri_count * 3;
	mesh.vertices = malloc(sizeof(float) * mesh.vertexCount * 3);
	mesh.texcoords = malloc(sizeof(float) * mesh.vertexCount * 2);

	u16 vert_id = 0;
	for(u16 i = 0; i < leaf->num_faces; i++) {
		u16 face_id = bsp->lfaces.faces[leaf->first_face + i];
		Bsp_Face *face = &bsp->faces[face_id];

		Bsp_Surface *surface = &bsp->surfaces[face->styles];
		Bsp_Miptex *miptex = &bsp->miptex[surface->texture_id];

		// Get face vertices
		Vector3 face_verts[face->edge_count];
		for(i32 j = 0; j < face->edge_count; j++) {
			i32 ledge = bsp->ledges.edge[face->first_edge + j];
			if(ledge >= 0) 
				face_verts[j] = bsp->verts[bsp->edges[ledge][0]];
			else 
				face_verts[j] = bsp->verts[bsp->edges[-ledge][1]];
		}

		// Fan triangulate
		for(u16 j = 1; j < face->edge_count - 1; j++) {
			Vector3 tri[3] = { face_verts[0], face_verts[j], face_verts[j + 1] };
				
			for(u16 k = 0; k < 3; k++) {
				mesh.vertices[vert_id * 3 + 0] = tri[k].x;
				mesh.vertices[vert_id * 3 + 1] = tri[k].y;
				mesh.vertices[vert_id * 3 + 2] = tri[k].z;

				float u = (Vector3DotProduct(tri[k], surface->vector_s) + surface->dist_s) / miptex->width;
				float v = (Vector3DotProduct(tri[k], surface->vector_t) + surface->dist_t) / miptex->height;

				mesh.texcoords[vert_id * 2 + 0] = u;
				mesh.texcoords[vert_id * 2 + 1] = v;

				vert_id++;
			}
		}
	}

	UploadMesh(&mesh, false);
	return mesh;
} 

