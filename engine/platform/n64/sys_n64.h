/*
 sys_n64.h - n64 platform headers for HL1 Port (LibDragon)
 Copyright (C) 2024 Custom N64 Port
*/

#ifndef SYS_N64_H
#define SYS_N64_H

/* =====================================================
   Type definitions for Source engine compatibility
 ===================================================== */

typedef int qboolean;  /* Xash3D uses this instead of bool */

/* =====================================================
   N64 RSP RESET VECTOR CONSTANTS
   
   These constants are used by the LibDragon SDK's display_init()
   function to set up the RSP reset sequence and entry point.
 ===================================================== */

/* Standard LibDragon reset vector locations (from SDK source) */
#define SYS_RESET_VECTOR           0xFE000280
#define SYS_RESTART_VECTOR         0x80000000  /* Where RSP PC goes after SYSRESET */
#define SYS_USER_STACK_OFFSET      0x30        /* Offset of mode-0 Sysvar area */

/* =====================================================
   APPLICATION ENTRY POINT
   
   This is the address in ROM where main() or user entry code
   should be located. For a custom N64 port, this is typically
   at SYS_RESTART_VECTOR (0x80000000).
 ===================================================== */

#define USER_ENTRY_POINT           0x80000000

/* =====================================================
   PLATFORM FUNCTIONS
   
   These functions provide the Source engine with access to
   the underlying N64/LibDragon platform. They are declared
   here and defined in sys_n64.c.
 ===================================================== */

void Platform_Init( void );
void Platform_Shutdown( void );
double Platform_DoubleTime( void );
void Platform_Sleep( int msec );
void Platform_GetClipboardText( char *buffer, size_t size );
void Platform_SetClipboardText( const char *buffer, size_t size );
void *Platform_GetNativeObject( const char *name );
void Platform_Vibrate(float life, char flags);
void Platform_ShellExecute( const char *path, const char *parms );
void Platform_MessageBox( const char *title, const char *message, qboolean parentMainWindow );

#endif /* SYS_N64_H */