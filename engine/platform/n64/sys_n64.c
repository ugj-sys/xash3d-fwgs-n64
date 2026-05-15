/*
 sys_n64.c - n64 platform initialization for HL1 Port (LibDragon)
 Copyright (C) 2024 Custom N64 Port

 This file provides the platform abstraction layer for running the
 Xash3D Source engine on Nintendo 64 with LibDragon SDK.

 The critical fix for the "black screen" issue:
 - display_init() from LibDragon handles SDRAM initialization AND RSP SYSRESET
 - After display_init() completes, RSP PC is at SYS_RESTART_VECTOR (0x80000000)
 - Our entry stub jumps to user main() which continues execution
*/

#include "platform/platform.h"
#include <libdragon.h>

/* =====================================================
   PLATFORM INITIALIZATION
 ===================================================== */

void Platform_Init( void )
{
    debugf("Platform_Init called.\n");
    joypad_init(); // Initialize libdragon controller support
}

/* =====================================================
   PLATFORM SHUTDOWN
 ===================================================== */

void Platform_Shutdown( void )
{
    /* Clean up resources before halt */
    display_close();
}

/* =====================================================
   TIME FUNCTIONS
 ===================================================== */

double Platform_DoubleTime( void )
{
    return (double)timer_ticks() / (double)TICKS_PER_SECOND;
}

void Platform_Sleep( int msec )
{
    unsigned long long ticks = (unsigned long long)msec * (TICKS_PER_SECOND / 1000);
    unsigned long long start = timer_ticks();
    while( timer_ticks() - start < ticks )
    {
        /* Wait for timer */
    }
}

/* =====================================================
   CLIPBOARD FUNCTIONS (not supported on N64)
 ===================================================== */

void Platform_GetClipboardText( char *buffer, size_t size )
{
    if( size > 0 ) buffer[0] = '\0';
}

void Platform_SetClipboardText( const char *buffer, size_t size )
{
    /* No-op on N64 */
}

/* =====================================================
   NATIVE OBJECTS (not supported on N64)
 ===================================================== */

void *Platform_GetNativeObject( const char *name )
{
    return NULL;
}

/* =====================================================
   VIBRATION (not supported on N64)
 ===================================================== */

void Platform_Vibrate(float life, char flags)
{
    /* No-op on N64 */
}

/* =====================================================
   SHELL EXECUTION (not supported on N64)
 ===================================================== */

void Platform_ShellExecute( const char *path, const char *parms )
{
    /* No-op on N64 */
}

/* =====================================================
   MESSAGE BOX (simplified for N64)
 ===================================================== */

void Platform_MessageBox( const char *title, const char *message, qboolean parentMainWindow )
{
    printf("MessageBox: %s\n%s\n", title, message);
    fflush(stdout);

    // Strip Xash color codes (^N) and copy into clean buffer
    static char clean[512];
    const char *src = message;
    char *dst = clean;
    int remaining = (int)sizeof(clean) - 1;
    while( *src && remaining > 0 )
    {
        if( src[0] == '^' && src[1] >= '0' && src[1] <= '9' )
        {
            src += 2;  // skip color code
            continue;
        }
        *dst++ = *src++;
        remaining--;
    }
    *dst = '\0';

    graphics_set_default_font();
    graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);

    while(1) {
        surface_t* display = display_get();
        if (display) {
            graphics_fill_screen(display, graphics_make_color(180, 0, 0, 255));
            graphics_draw_text(display, 8, 8,  "FATAL BOOT ERROR:");
            graphics_draw_text(display, 8, 20, (char*)title);

            // Word-wrap at 38 chars per line
            int y = 36;
            const char *p = clean;
            char line_buf[40];
            while( *p && y < 230 )
            {
                // Copy up to 38 chars or until newline
                int i = 0;
                while( i < 38 && p[i] && p[i] != '\n' )
                    i++;
                memcpy(line_buf, p, i);
                line_buf[i] = '\0';
                graphics_draw_text(display, 8, y, line_buf);
                y += 12;
                p += i;
                if( *p == '\n' ) p++;
            }
            display_show(display);
        }
    }
}

/* =====================================================
   END OF PLATFORM IMPLEMENTATION
 ===================================================== */