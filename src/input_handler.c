#include "raylib.h"
#include "input_handler.h"

void InputInit(InputHandler *handler) {
	handler->input_method = INPUT_DEVICE_KEYBOARD;	
}

void PollInput(InputHandler *handler) {
	handler->mouse_position = GetMousePosition();
	handler->mouse_delta = GetMouseDelta();

	for(u8 i = 0; i < INPUT_ACTION_COUNT; i++) {
		InputAction *action = &handler->actions[i];

		i32 key = action->key;
		if(!key) continue;

		if(IsKeyUp(key))
			action->state = INPUT_ACTION_UP;

		if(IsKeyDown(key)) 
			action->state = INPUT_ACTION_DOWN;

		if(IsKeyPressed(key))
			action->state = INPUT_ACTION_PRESSED;

		if(IsKeyReleased(key))
			action->state = INPUT_ACTION_RELEASED;
	}
}
