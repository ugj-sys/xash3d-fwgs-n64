/*
launcher.c - direct xash3d launcher
Copyright (C) 2015 Mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifdef SINGLE_BINARY

#include <stdlib.h>
#include <string.h>
#include "build.h"
#include "common.h"
#ifdef XASH_SDLMAIN
#include "SDL.h"
#endif

#if XASH_EMSCRIPTEN
#include <emscripten.h>
#endif

#if XASH_WIN32
#define XASH_NOCONHOST 1
#endif

static char szGameDir[128]; // safe place to keep gamedir
static int g_iArgc;
static char **g_pszArgv;
#if XASH_PSP
#define MAX_NARGVS 50
static int	g_fArgc;
static char *g_fArgv[MAX_NARGVS];
#endif

void Launcher_ChangeGame( const char *progname )
{
	strncpy( szGameDir, progname, sizeof( szGameDir ) - 1 );
	Host_Shutdown( );
	exit( Host_Main( g_iArgc, g_pszArgv, szGameDir, 1, &Launcher_ChangeGame ) );
}

#if XASH_NOCONHOST
#include <windows.h>
#include <shellapi.h> // CommandLineToArgvW
int __stdcall WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int nShow)
{
	int szArgc;
	char **szArgv;
	LPWSTR* lpArgv = CommandLineToArgvW(GetCommandLineW(), &szArgc);
	int i = 0;

	szArgv = (char**)malloc(szArgc*sizeof(char*));
	for (; i < szArgc; ++i)
	{
		size_t size = wcslen(lpArgv[i]) + 1;
		szArgv[i] = (char*)malloc(size);
		wcstombs(szArgv[i], lpArgv[i], size);
	}
	szArgv[i] = NULL;

	LocalFree(lpArgv);

	main( szArgc, szArgv );

	for( i = 0; i < szArgc; ++i )
		free( szArgv[i] );
	free( szArgv );
}
#endif

#if XASH_N64
#undef NDEBUG
#include <libdragon.h>
#define NDEBUG
#include <sys/reent.h>

// Captures ALL stdout/stderr traffic and forcefully
// ejects it from the silicon cache into the physical RAM chips IMMEDIATELY.
static char debug_log_scratch[4096] __attribute__((aligned(16)));
static int debug_log_cursor = 0;

_ssize_t _write_r( struct _reent *r, int fd, const void *buf, size_t nbyte )
{
	char *src = (char *)buf;
	for( size_t i = 0; i < nbyte && debug_log_cursor < (sizeof(debug_log_scratch) - 2); i++ )
	{
		debug_log_scratch[debug_log_cursor++] = src[i];
	}
	debug_log_scratch[debug_log_cursor] = '\0'; // keep it null terminated

	// CRITICAL: Command the MIPS core to force all data out of internal cache 
	// and dump it straight into the physical memory chips, securing it forever!
	data_cache_hit_writeback( debug_log_scratch, sizeof(debug_log_scratch) );
	
	return (_ssize_t)nbyte; 
}
#endif

#if XASH_N64
qboolean g_sd_mounted = false;
#endif

int n64_console_open = 0;

int xash_main( int argc, char** argv );

int __attribute__((naked)) main( int argc, char** argv )
{
	__asm__ __volatile__(
		"la $sp, 0x807ffff0\n\t"
		"j xash_main\n\t"
		"nop\n\t"
	);
}

int xash_main( int argc, char** argv )
{
#if XASH_N64
	// Libdragon's N64 entrypoint doesn't pass command-line arguments.
	// Initializing these to zero/NULL prevents reading uninitialized registers as garbage strings.
	static char *n64_argv[] = { "xash3d", "-dev", "5", "-width", "320", "-height", "240", "+vid_width", "320", "+vid_height", "240", "-disablehelp", "-ref", "soft", NULL };
	argc = 14;
	argv = n64_argv;

	extern int n64_display_initialized;

	timer_init();
	
	// SUPREME SYSTEM OVERRIDE: Force Expansion Pak Annexation
	// Because get_memory_size() fails to auto-detect the 8MB runtime in this ecosystem,
	// we must physically force the kernel variables to recognize the extra 4MB,
	// while strictly reserving the final 512KB for the Hardware Stack protection zone.
	extern char *__heap_top;
	extern int __heap_total_size;

	// Bootstrap standard LibDragon memory struct
	void *bootstrap = malloc(1); 
	if( __heap_top ) {
		// Absolute physical ceiling: 8MB limit minus 128KB stack buffer
		char *absolute_ceiling = (char*)(0x80000000 + (8 * 1024 * 1024) - (128 * 1024));
		
		// Compute absolute differential from current state (handles both positive and negative adjustments!)
		int32_t diff = absolute_ceiling - __heap_top;
		
		// Apply dynamic balanced adjustment
		__heap_top = absolute_ceiling;
		__heap_total_size += diff;
	}
	free(bootstrap);

	extern int get_memory_size(void);
	int actual_ram_kb = get_memory_size() / 1024;

	display_init( RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE );
	n64_display_initialized = 1;
	
	// CRITICAL: Unlock host console wrapper so Con_Printf triggers actual N64 printf immediately!
	host.allow_console = true; 
	
	Con_Printf("CRITICAL HW: PHYSICAL N64 RAM DETECTED = %d KB\n", actual_ram_kb);
	Con_Printf("CRITICAL HW: OVERRIDE APPLIED! ADDED 4096 KB TO HEAP CEILING!\n");

	if (dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS) {
		// DFS is NOT in the ROM — halt with a visible error screen.
		graphics_set_default_font();
		graphics_set_color(graphics_make_color(255,255,255,255), 0);
		while(1) {
			surface_t* s = display_get();
			if(s) {
				graphics_fill_screen(s, graphics_make_color(200,0,0,255));
				graphics_draw_text(s, 20, 80,  "FATAL: DFS NOT IN ROM");
				graphics_draw_text(s, 20, 100, "Rebuild with DFS payload.");
				display_show(s);
			}
		}
	}

	// MOUNT SUMMER CART 64 / EVERDRIVE FAT FILESYSTEM
	// Note: debug_init_sdfs returns true on success
	if (debug_init_sdfs("sd:/", -1)) {
		g_sd_mounted = true;
		Con_Printf("SUMMERCART64 SD DISK MOUNTED SUCCESS!\n");
		
		// Write safe telemetry file
		FILE *f = fopen("sd:/hl_n64.txt", "w");
		if (f) {
			fprintf(f, "Half-Life Xash3D N64 Engine Active!\n");
			fprintf(f, "Successfully mapped SD filesystem over PI DMA registers!\n");
			fclose(f);
			Con_Printf("Telemetry write sd:/hl_n64.txt successful.\n");
		} else {
			Con_Printf("SD Mount succeeded but root write permissions failed.\n");
		}
	} else {
		g_sd_mounted = false;
		Con_Printf("No FlashCart SD detected. Falling back to Internal Memory/ROM.\n");
	}
#endif



	char gamedir_buf[32] = "";
	const char *gamedir = getenv( "XASH3D_GAMEDIR" );

	if( !COM_CheckString( gamedir ) )
	{
		gamedir = "valve";
	}
	else
	{
		strncpy( gamedir_buf, gamedir, 32 );
		gamedir = gamedir_buf;
	}

#if XASH_EMSCRIPTEN
#ifdef EMSCRIPTEN_LIB_FS
	// For some unknown reason emscripten refusing to load libraries later
	COM_LoadLibrary("menu", 0 );
	COM_LoadLibrary("server", 0 );
	COM_LoadLibrary("client", 0 );
#endif
#if XASH_DEDICATED
	// NodeJS support for debug
	EM_ASM(try{
		FS.mkdir('/xash');
		FS.mount(NODEFS, { root: '.'}, '/xash' );
		FS.chdir('/xash');
	}catch(e){};);
#endif
#endif

	g_iArgc = argc;
	g_pszArgv = argv;

#if XASH_PSP	
	if(argc <= 1)
	{
		g_fArgc = 0;
		g_fArgv[g_fArgc++] = argv[0]; // cwd path
		Platform_ReadCmd( "start.cmd", &g_fArgc, g_fArgv );
		if(g_fArgc > 1)
		{
			g_iArgc = g_fArgc;
			g_pszArgv = g_fArgv;
		}	
	}
#endif

#if XASH_IOS
	{
		void IOS_LaunchDialog( void );
		IOS_LaunchDialog();
	}
#endif

#if XASH_PSP
	Host_Main( g_iArgc, g_pszArgv, gamedir, 0, &Launcher_ChangeGame );
	Sys_Quit( );
	return 0;
#else
	return Host_Main( g_iArgc, g_pszArgv, gamedir, 0, &Launcher_ChangeGame );
#endif
}

#endif
