#include "../include/num_redefs.h"
#include "raylib.h"
#include "ent.h"
#include "v_effect.h"
#include "config.h"
#include "audioplayer.h"
#include "rw_save.h"

#ifndef PLAYER_GUN_H_
#define PLAYER_GUN_H_

typedef struct {
	Model model;

	Camera3D cam;

	u16 current_gun;

} PlayerGun;

void PlayerGunInit(
	PlayerGun *player_gun,
	Entity *player,
	EntityHandler *handler,
	MapSection *sect,
	vEffect_Manager *effect_manager,
	Config *conf,
	Camera3D *world_cam,
	AudioPlayer *ap,
	InputHandler *input
);

void PlayerGunUpdate(PlayerGun *player_gun, float dt);
void PlayerGunDraw(PlayerGun *player_gun);
void PlayerGunDraw2d(PlayerGun *player_gun);
void PlayerShoot(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect);

void PlayerShootPistol(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect);
void PlayerShootShotgun(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect);
void PlayerShootRevolver(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect);
void PlayerShootDisruptor(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect);
void PlayerShootSMG(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect);

void PlayerGunUpdatePistol(PlayerGun *player_gun, float dt);
void PlayerGunUpdateShotgun(PlayerGun *player_gun, float dt);
void PlayerGunUpdateRevolver(PlayerGun *player_gun, float dt);
void PlayerGunUpdateDisruptor(PlayerGun *player_gun, float dt);
void PlayerGunUpdateSMG(PlayerGun *player_gun, float dt);

void PlayerGunReload(PlayerGun *player_gun, float dt);

void SendAmmoPickupEvent(int pickup_type);

void PlayerGunOnSave(rw_GlobalData *data, PlayerGun *player_gun);
void PlayerGunOnLoad(rw_GlobalData *data, PlayerGun *player_gun);

void HandlerSetPtrGun(PlayerGun *player_gun);
void PlayerGunSetMouseDelta(Vector2 md);

#endif
