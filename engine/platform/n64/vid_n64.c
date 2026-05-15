/*
vid_n64.c
*/
#include "platform/platform.h"
#include "client/vid_common.h"
#include "../../client/ref_common.h"
#include "../../ref_api.h"
#include <libdragon.h>

static surface_t *n64_surface = NULL;
qboolean n64_display_initialized = false;

qboolean VID_SetMode( void )
{
	int iScreenWidth = 320;
	int iScreenHeight = 240;
	qboolean fullscreen = false;

	if( !FBitSet( vid_fullscreen->flags, FCVAR_CHANGED ) )
		Cvar_SetValue( "fullscreen", 0 );
	else
		ClearBits( vid_fullscreen->flags, FCVAR_CHANGED );

	// force the N64 minimum display size
	if( R_ChangeDisplaySettings( iScreenWidth, iScreenHeight, fullscreen ) != rserr_ok )
		return false;

	return true;
}

qboolean R_Init_Video( const int type )
{
	refState.desktopBitsPixel = 16;

	VID_StartupGamma();

	switch( type )
	{
	case REF_SOFTWARE:
		glw_state.software = true;
		Con_Reportf( "R_Init_Video: Initializing N64 software renderer\n" );
		break;
	case REF_GL:
		Host_Error( "N64 platform does not support GL render backend\n" );
		return false;
	default:
		Host_Error( "Unknown N64 render backend %d\n", type );
		return false;
	}

	if( !n64_display_initialized )
	{
		Con_Reportf( "R_Init_Video: Calling display_init(320x240 @ 16bpp)\n" );
		display_init( RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE );
		n64_display_initialized = true;
		Con_Reportf( "R_Init_Video: Display initialized\n" );
	}

	if( !VID_SetMode() )
	{
		Con_Printf( S_ERROR "R_Init_Video: VID_SetMode failed\n" );
		return false;
	}

	host.renderinfo_changed = false;

	Con_Reportf( "R_Init_Video: Video initialized successfully\n" );
	return true;
}

void R_Free_Video( void )
{
	if( n64_surface )
	{
		display_show( n64_surface );
		n64_surface = NULL;
	}

	if( n64_display_initialized )
	{
		display_close();
		n64_display_initialized = false;
	}
}

rserr_t R_ChangeDisplaySettings( int width, int height, qboolean fullscreen )
{
	Con_Reportf( "R_ChangeDisplaySettings: N64 forced mode %dx%d %s\n", width, height, fullscreen ? "fullscreen" : "windowed" );

	if( width != 320 || height != 240 )
	{
		Con_Reportf( "N64 display backend forcing 320x240 mode instead of %dx%d\n", width, height );
		width = 320;
		height = 240;
	}

	R_SaveVideoMode( width, height, width, height );

	return rserr_ok;
}

int R_MaxVideoModes( void )
{
	return 1;
}

struct vidmode_s *R_GetVideoMode( int num )
{
	static struct vidmode_s dummy_mode = { "320x240", 320, 240 };
	return &dummy_mode;
}

void* GL_GetProcAddress( const char *name )
{
	return NULL;
}

void GL_UpdateSwapInterval( void )
{
}

int GL_SetAttribute( int attr, int val )
{
	return 0;
}

int GL_GetAttribute( int attr, int *val )
{
	if( val ) *val = 0;
	return 0;
}

void GL_SwapBuffers( void )
{
}

static unsigned short dummy_buffer[4]; // Completely unused after display_init, downsized to save 150KB.

void *SW_LockBuffer( void )
{
	if( !n64_display_initialized )
		return dummy_buffer;

	if( !n64_surface )
		n64_surface = display_get();

	if( n64_surface )
	{
		// Return a cached pointer (KSEG0) for much faster software rendering
		return (void *)(((uint32_t)n64_surface->buffer) & ~0x20000000);
	}

	return dummy_buffer;
}

void SW_UnlockBuffer( void )
{
	if( n64_surface )
	{
		// Use the cached pointer to set the alpha bit
		uint32_t *pixels32 = (uint32_t *)(((uint32_t)n64_surface->buffer) & ~0x20000000);
		int count32 = (n64_surface->stride * n64_surface->height) / 4;
		for( int i = 0; i < count32; i++ )
			pixels32[i] |= 0x00010001; // Set alpha bit for all pixels so the VI displays them

		// Writeback the cache to physical RAM so the VI can see it
		data_cache_hit_writeback( pixels32, n64_surface->stride * n64_surface->height );
		display_show( n64_surface );
		n64_surface = NULL;
	}
}

qboolean SW_CreateBuffer( int width, int height, uint *stride, uint *bpp, uint *r, uint *g, uint *b )
{
	// The display was already initialized in launcher.c main().
	n64_display_initialized = true;

	if( width != 320 || height != 240 )
	{
		extern void Sys_Error( const char *error, ... );
		Sys_Error( "FATAL: engine requested %dx%d but N64 VI is 320x240.", width, height );
		return false;
	}

	// For 16 BPP: stride is in bytes, so stride_pixels = stride / 2
	int bytes_per_pixel = 2;  // 16 BPP (DEPTH_16_BPP)
	
	if( stride )
		*stride = 320;  // Always force N64 hardware stride in pixels for 320x240
	if( bpp )
		*bpp = bytes_per_pixel;
	if( r )
		*r = 0xF800;  // R5 (5-bit red at bits 15-11)
	if( g )
		*g = 0x07C0;  // G5 (5-bit green at bits 10-6)
	if( b )
		*b = 0x003E;  // B5 (5-bit blue at bits 5-1)

	return true;
}
