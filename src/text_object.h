#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef TEXT_OBJECT_H_
#define TEXT_OBJECT_H_

#define TXT_OBJ_ACTIVE	0x01
#define TXT_OBJ_SHOW	0x02
typedef struct {
	char text[64];
	
	Vector3 position;
		
	u8 flags;

} TextObject;

#endif

