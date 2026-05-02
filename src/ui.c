#include <stdlib.h>
#include "raylib.h"
#include "ui.h"

UiHandler *ui_self_ptr = NULL;

void ui_Toggle() {
	// Toggle active flag
	ui_self_ptr->flags ^= UI_ACTIVE;

	if(ui_self_ptr->flags & UI_ACTIVE) {
		// Enable and hide OS cursor on activation
		EnableCursor();
		HideCursor();
	} else {
		// Disable OS cursor on deactivation
		DisableCursor();
	}
}

void ui_Update(UiHandler *ui, float dt) {
	// Bind ui self pointer
	if(!ui_self_ptr)
		ui_self_ptr = ui;

	// Do nothing if ui is inactive
	if(!(ui->flags & UI_ACTIVE))	
		return;

	ui->cursor_pos = GetMousePosition();
	ui_DisplayCursor(ui);
}

void ui_DisplayCursor(UiHandler *ui) {
	
}

u8 ui_Button(UiHandler *ui, Rectangle rect) {
	bool hover = CheckCollisionPointRec(ui->cursor_pos, rect);

	if(hover) {

		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			return 1;
	}	

	return 0;
}

