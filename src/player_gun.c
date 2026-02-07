#include "raylib.h"
#include "raymath.h"
#include "player_gun.h"
#include "geo.h"

Vector3 gun_pos = {0};
float gun_rot = 0;
float p, y, r;
Matrix mat = {0};

#define REVOLVER_REST (Vector3) { -1.05f, -2.05f, 3.5f }
#define REVOLVER_ANGLE_REST 2.5f

float recoil = 0;

void PlayerGunInit(PlayerGun *player_gun) {
	player_gun->cam = (Camera3D) {
		.position = (Vector3) { 0, 0, -1 },
		.target = (Vector3) { 0, 0, 1 },
		.up = (Vector3) {0, 1, 0},
		.fovy = 70,
		.projection = CAMERA_PERSPECTIVE
	};

	player_gun->model = LoadModel("resources/rev_00.glb");
	mat = player_gun->model.transform;

	gun_pos = REVOLVER_REST;
	gun_rot = REVOLVER_ANGLE_REST;
}

void PlayerGunUpdate(PlayerGun *player_gun, float dt) {
	player_gun->model.transform = mat;
	mat = MatrixRotateX(-(recoil + gun_rot) * DEG2RAD);

	recoil -= ((recoil * 0.99f) * 10 * dt); 
	if(recoil <= 0.1f) recoil = 0;

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && recoil <= 5.0f) {
		recoil += 120 + (GetRandomValue(10, 30) * 0.1f);
	}

	gun_pos.z = REVOLVER_REST.z - (recoil * 0.025f);
}

void PlayerGunDraw(PlayerGun *player_gun) {
	BeginMode3D(player_gun->cam);
	
	DrawModel(player_gun->model, gun_pos, 1.0f, WHITE);
	//DrawModelEx(player_gun->model, gun_pos, UP, gun_rot, Vector3Scale(Vector3One(), 1.0f), WHITE);

	EndMode3D();
}

