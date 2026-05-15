/*
in_n64.c
*/

#include "platform/platform.h"
#include <libdragon.h>
#include "keydefs.h"

extern void Key_Event( int key, int down );
extern void Cbuf_AddText( const char *text );

int Platform_JoyInit( int numjoy )
{
	return 0;
}

void Platform_EnableTextInput( qboolean enable )
{
}

void Platform_RunEvents( void )
{
	joypad_poll();

	if (!joypad_is_connected(JOYPAD_PORT_1))
		return;

	joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
	joypad_buttons_t released = joypad_get_buttons_released(JOYPAD_PORT_1);

	// Process Button Presses
	if (pressed.a)      Key_Event(K_ENTER, 1);
	if (pressed.b)      Key_Event(K_BACKSPACE, 1);
	if (pressed.start)  Key_Event(K_START_BUTTON, 1);
	if (pressed.l)      Key_Event(K_ESCAPE, 1);
	if (pressed.d_up)   Key_Event(K_UPARROW, 1);
	if (pressed.d_down) Key_Event(K_DOWNARROW, 1);
	if (pressed.d_left) Key_Event(K_LEFTARROW, 1);
	if (pressed.d_right)Key_Event(K_RIGHTARROW, 1);
	
	// N64 Gameplay Additions
	if (pressed.c_up)   Key_Event(K_UPARROW, 1);   // Move Forward
	if (pressed.c_down) Key_Event(K_DOWNARROW, 1); // Move Backward
	if (pressed.z)      Key_Event(K_MOUSE1, 1);    // Fire Weapon
	if (pressed.start) Cbuf_AddText("map stalkyard\n"); // Instant Map Load!

	// Process Button Releases
	if (released.a)      Key_Event(K_ENTER, 0);
	if (released.b)      Key_Event(K_BACKSPACE, 0);
	if (released.start)  Key_Event(K_START_BUTTON, 0);
	if (released.l)      Key_Event(K_ESCAPE, 0);
	if (released.d_up)   Key_Event(K_UPARROW, 0);
	if (released.d_down) Key_Event(K_DOWNARROW, 0);
	if (released.d_left) Key_Event(K_LEFTARROW, 0);
	if (released.d_right)Key_Event(K_RIGHTARROW, 0);
	
	if (released.c_up)   Key_Event(K_UPARROW, 0);
	if (released.c_down) Key_Event(K_DOWNARROW, 0);
	if (released.z)      Key_Event(K_MOUSE1, 0);
}

void Platform_GetMousePos( int *x, int *y )
{
	if( x ) *x = 0;
	if( y ) *y = 0;
}

void Platform_SetMousePos( int x, int y )
{
}

void Platform_PreCreateMove( void )
{
}

void Platform_MouseMove( float *x, float *y )
{
	// DIRECT ENGINE OVERRIDE: Zeroed out to prevent corrupting viewangles
	if( x ) *x = 0.0f;
	if( y ) *y = 0.0f;
}
