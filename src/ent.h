#include "raylib.h"
#include "raymath.h"
#include "../include/num_redefs.h"
#include "input_handler.h"
#include "geo.h"
#include "map.h"
#include "ai.h"
#include "v_effect.h"
#include "anim.h"
#include "audioplayer.h"

#ifndef ENT_H_
#define ENT_H_

// ----------------------------------------------------------------------------------------------------------------------------
void EntDebugText();

typedef struct {
	i16 c;	// x, column
	i16 r;	// y, row
	i16 t;	// z, tab

} Coords;

#define ENT_GRID_CELL_EXTENTS (Vector3) { 380, 380, 380 } 
#define MAX_ENTS_PER_CELL	32
typedef struct {
	BoundingBox aabb;

	i16 ents[MAX_ENTS_PER_CELL];
	u8 ent_count;

} EntGridCell;

typedef struct {
	EntGridCell *cells;

	char sect_next[16];
	char sect_prev[16];

	Vector3 origin;
	Coords size;

	i16 cell_count;	

	i16 level_end;		// Cell that triggers loading next section
	i16 level_back;		// Cell that triggers loading previous section

} EntGrid;

int16_t CellCoordsToId(Coords coords, EntGrid *grid);
Coords CellIdToCoords(int16_t id, EntGrid *grid);

Coords Vec3ToCoords(Vector3 v, EntGrid *grid);
Vector3 CoordsToVec3(Coords coords, EntGrid *grid);

bool CoordsInBounds(Coords coords, EntGrid *grid);

typedef struct {
	BoundingBox bounds;

	Quaternion qrot;

	Vector3 position;
	Vector3 velocity;
	
	Vector3 forward;
	Vector3 start_forward;
	Vector3 targ_look;

	Vector3 ground_normal;
	Vector3 prev_pos;

	Vector3 last_safe_pos;

	float start_angle;
	float radius;

	float pitch, yaw, roll;

	float air_time;

	i32 last_ground_surface;

	short on_ground;

} comp_Transform;

#define BUG_POINT_TURRET (Vector3) { 0, 0, 20 }
#define BUG_POINT_MAINTAINER (Vector3) { 0, 0, 25 }

typedef struct {
	BoundingBox hit_box;
	BoundingBox crit_box;

	// Hitbox for bug 
	BoundingBox bug_box;

	// Point where bug will be if succesful hit
	Vector3 bug_point;

	float damage_cooldown;
	short amount;

	// Hit function id
	short on_hit;

	// Destroy function id
	short on_destroy;
	
	bool component_valid;

} comp_Health;

void ApplyDamage(comp_Health *comp_health, short amount);

#define WEAPON_TRAVEL_HITSCAN		0
#define WEAPON_TRAVEL_PROJECTILE	1	

enum weapon_types : u8 {
	/*	
	WEAP_PISTOL,
	WEAP_SHOTGUN,
	WEAP_REVOLVER,
	WEAP_DISRUPTOR,
	WEAP_TURRET
	*/
	WEAP_DISRUPTOR,
	WEAP_REVOLVER,
	WEAP_PISTOL,
	WEAP_SHOTGUN,
	WEAP_TURRET,
};

typedef struct {
	float cooldown;

	float reload_time_amnt;
	float reload_timer;

	short travel_type;
	short damage;

	int clip_size;
	int in_clip;
	int ammo;

	u8 ammo_type;
	u8 id;

} comp_Weapon;

#define ENT_ACTIVE		0x01
#define ENT_COLLIDERS	0x02
#define ENT_IS_PICKUP	0x04

enum ENT_BEHAVIORS : i8 {
	ENT_BEHAVIOR_NONE 		= -1,
	ENT_BEHAVIOR_PLAYER		=  0,
};

enum ENT_TYPES : u8 {
	ENT_PLAYER 		 		= 	0,
	ENT_TURRET 		 		= 	1,
	ENT_MAINTAINER 	 		= 	2,
	ENT_REGULATOR	 		= 	3,
	ENT_DRONE 		 		= 	4,
	ENT_HEALTHPACK	 		= 	5,
	ENT_AMMO_PISTOL	 		= 	6,
	ENT_AMMO_SHOTGUN 		= 	7,
	ENT_AMMO_REVOLVER		=	8,
	ENT_DISRUPTOR	 		= 	9,
	ENT_SWITCH				=  10,
	ENT_BRUSH				=  11,		// * Unused (probably not useful...)
	ENT_FORCEFIELD			=  12,
	ENT_BULLSEYE			=  13,
	ENT_DSP_DEFAULT 		=  14,
	ENT_DSP_SMALL_ROOM		=  15,
	ENT_DSP_OPEN			=  16,
	ENT_TEXT_OBJECT			=  17,
	ENT_DOOR				=  18,
	ENT_LADDER				=  19,
	ENT_GLASS				=  20,
};

enum ON_TRIGGER_EVENT_TYPES : u8 {
	TRIGGER_NONE 			=  0,
	TRIGGER_TOGGLE 			=  1,
	TRIGGER_TURN_ON			=  2,
	TRIGGER_TURN_OFF		=  3,
	TRIGGER_OPEN			=  4,
	TRIGGER_CLOSE			=  5,
};

enum TRIGGER_CONDITION_TYPES : u8 {
	TRIGGER_COND_HIT		=  0,	// Hit with bullet
	TRIGGER_COND_COLL_ENT	=  1,	// Colliding with entity (any)
	TRIGGER_COND_COLL_PLR	=  2,	// Colliding with player
	TRIGGER_COND_COLL_BUG	=  3,	// Colliding with bug
	TRIGGER_COND_INTERACT   =  4,
};

typedef struct {
	Model model;

	ModelAnimation *animations;
	int num_anims;

	AnimState anim_state;

	comp_Ai comp_ai;
	comp_Transform comp_transform;
	comp_Health comp_health;
	comp_Weapon comp_weapon;

	int bsp_model;

	u16 id;
	i16 cell_id;

	u16 trigger_id;
	u8 on_trigger;
	u8 trigger_condition;
	u8 trigger_state;

	i8 type;
	u8 flags;

} Entity;

enum projectile_states : u8 {
	TRAVELLING,
	LANDED
};

typedef struct {
	comp_Transform ct;
	comp_Health health;

	short base_damage;	

	u16 sender;
	
	u8 state;
	u8 type;

	bool active;

} Projectile;

enum ENT_SHADER_LOCS {
	LC_LIGHT_POS	= 0,
	LC_LIGHT_CLR	= 1,
	LC_MODEL_MAT	= 2,
	LC_MODEL_TEX	= 3,
	LC_VIEW_POS		= 4,
};

typedef struct {
	int locs[8];

} EntShaderLocs;

// Entity handler state flags
#define AT_LEVEL_END 		0x01
#define AT_LEVEL_BACK		0x02
#define AUTOSAVE_REQUEST	0x04
typedef struct {
	Model base_ent_models[16];
	Model weap_models[16];

	Entity *ents;
	Projectile *projectiles;

	EntGrid grid;
	SpawnList spawn_list;
	CheckPointList checkpoint_list;

	vEffect_Manager *effect_manager;
	AudioPlayer *ap;

	Shader ent_shader;
	EntShaderLocs ent_shader_locs;

	Vector3 player_start;
	Vector3 player_start_fwd;

	Vector3 player_ret;
	Vector3 player_ret_fwd;
	float player_death_timer;

	float ai_tick;
	float autosave_tick;

	int brush_ents_offset;

	u16 count;
	u16 capacity;

	u16 projectile_count;
	u16 projectile_capacity;

	u16 player_id;
	u16 bug_id;

	u8 flags;

} EntityHandler;

void LoadEntityBaseModels(EntityHandler *handler);

void EntHandlerInit(EntityHandler *handler, vEffect_Manager *effect_manager, AudioPlayer *ap);
void EntHandlerClose(EntityHandler *handler);

void UpdateEntities(EntityHandler *handler, MapSection *sect, float dt);
void RenderEntities(EntityHandler *handler, float dt);

void RenderBrushEntities(EntityHandler *handler);

void UpdateRenderList(EntityHandler *handler, MapSection *sect);

void EntGridInit(EntityHandler *handler);
void UpdateGrid(EntityHandler *handler);

void DrawEntsDebugInfo();

void SpawnPlayer(Entity *ent, Vector3 position, Vector3 fwd);

// ----------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------
// *** Player defs *** 
typedef struct {
	BoundingBox sweep_box;

	Vector3 view_dir, view_dest;
	Vector3 move_dir, move_dest;

	float view_length;
	float move_length;

	float accel;

} PlayerDebugData;

void PlayerInit(Camera3D *camera, InputHandler *input, MapSection *test_section, PlayerDebugData *debug_data, EntityHandler *ent_handler);

void PlayerUpdate(Entity *player, float dt);
void PlayerDraw(Entity *player);

void PlayerDamage(Entity *player, short amount);
void PlayerDie(Entity *player);

void PlayerDisplayDebugInfo(Entity *player);
void PlayerDebugText(Entity *player);

void PlayerMove(Entity *player, float dt);

void PlayerSetRecoilInput(Entity *player, float recoil_input);

// ----------------------------------------------------------------------------------------------------------------------------

void ProcessEntity(EntSpawn *spawn_point, EntityHandler *handler, NavGraph *nav_graph, Bsp_Data *bsp);
Entity SpawnEntity(EntSpawn *spawn_point, EntityHandler *handler, Bsp_Data *bsp);

// ----------------------------------------------------------------------------------------------------------------------------
// **** Enemies **** 
void TurretUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt);
void TurretDraw(Entity *ent);
void TurretShoot(Entity *ent, EntityHandler *handler, MapSection *sect, float dt);

void MaintainerUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt);
void MaintainerDraw(Entity *ent, EntityHandler *handler, float dt);

void RegulatorUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt);
void RegulatorDraw(Entity *ent, EntityHandler *handler, float dt);

// ----------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------
// **** AI ****

void AiSystemUpdate(EntityHandler *handler, MapSection *sect, float dt);
void AiRunTimers(EntityHandler *handler, float dt);

void AiComponentUpdate(Entity *ent, EntityHandler *handler, comp_Ai *ai, MapSection *sect, float dt);

void AiCheckInputs(Entity *ent, EntityHandler *handler, MapSection *sect);

void AiDoSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, comp_Ai *ai, float dt);
u8 AiDoTask(Entity *ent, EntityHandler *handler, MapSection *sect, comp_Ai *ai, u8 task_id, float dt);

void AiDoState(Entity *ent, comp_Ai *ai, float dt);

// * NAVIGATION
// Definitions found in nav.h
int FindClosestNavNode(Vector3 position, MapSection *sect);
void AiNavSetup(EntityHandler *handler, MapSection *sect);

int FindClosestNavNodeInGraph(Vector3 position, NavGraph *graph);
bool MakeNavPath(Entity *ent, NavGraph *graph, i16 target_id);

int FindNavNodeTo(Vector3 pos_A, Vector3 pos_B, NavGraph *graph, MapSection *sect);

bool AiMoveToNode(Entity *ent, NavGraph *graph, u16 path_id);
// ----------------------------------------------------------------------------------------------------------------------------
// **** Bullets ****

typedef struct {
	Vector3 point;	
	Vector3 normal;

	float dist;

	i16 hit_ent;

} EntTraceData;

EntTraceData EntTraceDataEmpty();

Vector3 TraceEntities(Ray ray, EntityHandler *handler, float max_dist, u16 sender, EntTraceData *trace_data);

Vector3 TraceBullet(EntityHandler *handler, MapSection *sect, Vector3 origin, Vector3 dir, u16 sender, bool *hit, bool dummy);

void DebugDrawEntText(EntityHandler *handler, Camera3D cam); 


// ----------------------------------------------------------------------------------------------------------------------------
// **** BUG ****

// Bug states
enum BUG_STATES : u8 {
	BUG_DEFAULT     = 0,		// Default state, attached to player
	BUG_LAUNCHED    = 1,		// In air
	BUG_LANDED		= 2			// On ground/enemy
};

// Bug flags
// 0x01 reserved for ENT_ACTIVE
// 0x02 reserved for ENT_COLLIDERS
// 0x04 reserved for ENT_IS_PICKUP
#define BUG_DISRUPTED_ENEMY 0x20
#define BUG_RECALL			0x40
#define BUG_ON_SWITCH		0x80

void BugInit(Entity *ent, EntityHandler *handler, MapSection *sect);
void BugUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt);
void BugDraw(Entity *ent, EntityHandler *handler);

void DisruptEntity(EntityHandler *handler, u16 ent_id, MapSection *sect);
void AlertMaintainers(EntityHandler *handler, u16 disrupted_id);

// ----------------------------------------------------------------------------------------------------------------------------

void OnHitEnt(Entity *ent, short damage, Vector3 bullet_pos);
void OnHitPlayer(Entity *ent, short damage, Vector3 bullet_pos);
void OnHitBug(Entity *ent, short damage, Vector3 bullet_pos);
void OnHitTurret(Entity *ent, short damage, Vector3 bullet_pos);
void OnHitMaintainer(Entity *ent, short damage, Vector3 bullet_pos);
void OnHitRegulator(Entity *ent, short damage, Vector3 bullet_pos);
void OnHitSwitch(Entity *ent, short damage, Vector3 bullet_pos);

void DoFix(Entity *ent);
void OnFixTurret(Entity *ent);
void OnFixMaintainer(Entity *ent);
void OnFixRegulator(Entity *ent);

// ----------------------------------------------------------------------------------------------------------------------------

void EntMove(Entity *ent, MapSection *sect, EntityHandler *handler, float dt);

// ----------------------------------------------------------------------------------------------------------------------------
// *** Projectiles ***

void ProjectileUpdate(Projectile *projectile, EntityHandler *handler, MapSection *sect, float dt);
void ProjectileDraw(Projectile *projectile);

void ProjectileThrow(Entity *ent, Vector3 pos, Vector3 dir, float force, u8 type, EntityHandler *handler);
void ProjectileImpact(Projectile *projectile, EntityHandler *handler, i16 ent_id);

void ManageProjectiles(EntityHandler *handler, MapSection *sect, float dt);
void RenderProjectiles(EntityHandler *handler);

// ----------------------------------------------------------------------------------------------------------------------------

void ReloadEntities(EntityHandler *handler, MapSection *sect, short with_states);
void ReloadEntitiesPartial(EntityHandler *handler, MapSection *sect);

SpawnList ParseBspEnts(EntityHandler *handler, Bsp_Data *bsp);
void SetEntityTriggers(EntityHandler *handler);

// ----------------------------------------------------------------------------------------------------------------------------
void SwitchSetup(EntityHandler *handler);
void SwitchUpdate(EntityHandler *handler, Entity *switch_ent, float dt);
void SwitchDraw(Entity *ent, EntityHandler *handker, float dt);
void DoTrigger(EntityHandler *handler, Entity *switch_ent);

// ----------------------------------------------------------------------------------------------------------------------------
void EntDrawLitModel(EntityHandler *handler, Entity *ent, float scale, short min_light);
void EntDrawLitModelEx(EntityHandler *handler, Entity *ent, Vector3 pos, float scale, Vector3 axis, float angle, short min_light);

// ----------------------------------------------------------------------------------------------------------------------------

#define DOOR_OPENING 		0x20
#define DOOR_CLOSING 		0x40
#define PLAYER_ON_PLATFORM	0x80
void DoorUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt);

bool CheckLOS(Entity *ent, i16 targ_id, EntityHandler *handler, MapSection *sect, u8 flags);

void lh_SetEntHandlerPtr(EntityHandler *handler);

#endif
