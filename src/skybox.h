#include "raylib.h"

#ifndef SKYBOX_H_
#define SKYBOX_H_

static TextureCubemap GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format);

typedef struct {
	Model model;
	TextureCubemap cubemap;
	Shader shader;

} SkyBox;

void skybox_Init(SkyBox skybox);
void skybox_Render(SkyBox skybox);

#endif
