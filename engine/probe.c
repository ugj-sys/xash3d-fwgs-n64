#include <stdio.h>
#include <stddef.h>
#include "common/const.h"
#include "common/protocol.h"
#include "client/client.h"

#define PRINT_SIZE(x) printf("SIZE OF " #x ": %d bytes\n", (int)sizeof(x));
#define PRINT_OFF(type, member) printf("OFFSET OF " #member ": %d bytes\n", (int)offsetof(type, member));

int main() {
    printf("--- CLIENT_T ANATOMY REPORT ---\n");
    PRINT_SIZE(client_t);
    
    PRINT_OFF(client_t, frames);
    PRINT_OFF(client_t, commands);
    PRINT_OFF(client_t, predicted_frames);
    PRINT_OFF(client_t, players);
    PRINT_OFF(client_t, instanced_baseline);
    PRINT_OFF(client_t, sound_precache);
    PRINT_OFF(client_t, event_precache);
    PRINT_OFF(client_t, files_precache);
    PRINT_OFF(client_t, lightstyles);
    PRINT_OFF(client_t, models);
    PRINT_OFF(client_t, consistency_list);
    PRINT_OFF(client_t, resourcesonhand);
    PRINT_OFF(client_t, resourcelist);
    PRINT_OFF(client_t, sound_index);
    PRINT_OFF(client_t, decal_index);
    
    printf("--- MEMBER SIZE DUMP ---\n");
    PRINT_SIZE(((client_t*)0)->frames);
    PRINT_SIZE(((client_t*)0)->commands);
    PRINT_SIZE(((client_t*)0)->predicted_frames);
    PRINT_SIZE(((client_t*)0)->players);
    PRINT_SIZE(((client_t*)0)->instanced_baseline);
    PRINT_SIZE(((client_t*)0)->sound_precache);
    PRINT_SIZE(((client_t*)0)->event_precache);
    PRINT_SIZE(((client_t*)0)->files_precache);
    PRINT_SIZE(((client_t*)0)->resourcelist);
    
    return 0;
}
