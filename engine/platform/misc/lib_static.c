/*
lib_static.c - static linking support
Copyright (C) 2018 Flying With Gauss

This program is free software: you can redistribute it and/sor modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "platform/platform.h"
#include "library.h"
#include "server.h"
#include "eiface.h"


#if XASH_LIB == LIB_STATIC
#if defined(XASH_NO_LIBDL) && !defined(XASH_N64)

void *dlsym(void *handle, const char *symbol )
{
	Con_DPrintf( "dlsym( %p, \"%s\" ): stub\n", handle, symbol );
	return NULL;
}

void *dlopen(const char *name, int flag )
{
	Con_DPrintf( "dlopen( \"%s\", %d ): stub\n", name, flag );
	return NULL;
}

int dlclose(void *handle)
{
	Con_DPrintf( "dlsym( %p ): stub\n", handle );
	return 0;
}

char *dlerror( void )
{
	return "Loading ELF libraries not supported in this build!\n";
}

int dladdr( const void *addr, void *info )
{
	return 0;
}
#endif // XASH_NO_LIBDL


typedef struct table_s
{
	const char *name;
	void *pointer;
} table_t;

// DUMMY GAME SERVER PAYLOAD INJECTION 
static int Generic_Stub(void) { return 0; }
static int Success_Stub(void) { return 1; }
static const char *GameDesc_Stub(void) { return "N64 Engine Test"; }

static qboolean ClientConnect_Stub(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]) { 
	return true; 
}

// Teleport player to safety and initialize mandatory client flags so the 3D engine fires!
static void ClientPutInServer_Stub(edict_t *pEntity) {
	Con_Printf("[STUB] Player successfully put in server! INITIALIZING VIEWPORT...\n");
	
	// Position the player just in front of global origin inside map bounds
	pEntity->v.origin[0] = 0.0f;
	pEntity->v.origin[1] = 0.0f;
	pEntity->v.origin[2] = 64.0f;
	
	pEntity->v.fixangle = 1;
	pEntity->v.solid = 3;    // SOLID_SLIDEBOX
	pEntity->v.movetype = 3; // MOVETYPE_WALK
	pEntity->v.flags |= 8;   // FL_CLIENT
	
	// Make them visible and conscious
	pEntity->v.health = 100.0f;
	pEntity->v.takedamage = 2; // DAMAGE_YES
}

// Absolute default spawn logic executed automatically for EVERY entity found in the map file
static void Spawn_Stub(entvars_t *pev) {
	pev->solid = 0; // SOLID_NOT
}

static int GetHullBounds_Stub(int hullnumber, float *mins, float *maxs) {
	mins[0] = -16; mins[1] = -16; mins[2] = -36;
	maxs[0] = 16; maxs[1] = 16; maxs[2] = 36;
	return 1;
}

static int Dummy_GetEntityAPI(DLL_FUNCTIONS *pFunctionTable, int interfaceVersion) {
	int i;
	void **ptr = (void **)pFunctionTable;
	// Fill everything with generic stub
	for (i = 0; i < sizeof(DLL_FUNCTIONS) / sizeof(void *); i++) ptr[i] = (void*)Generic_Stub;
	
	// Apply mission-critical overrides
	pFunctionTable->pfnGetGameDescription = GameDesc_Stub;
	pFunctionTable->pfnSpawn = (void*)Success_Stub;
	pFunctionTable->pfnClientConnect = ClientConnect_Stub;
	pFunctionTable->pfnClientPutInServer = ClientPutInServer_Stub;
	pFunctionTable->pfnGetHullBounds = GetHullBounds_Stub;
	pFunctionTable->pfnAllowLagCompensation = (void*)Success_Stub;
	
	return 1;
}

static void Dummy_GiveFnptrsToDll(enginefuncs_t* engfuncs, globalvars_t *pGlobals) {
	// NO-OP
}

static table_t hl_exports[] = {
	{"GetEntityAPI", (void*)Dummy_GetEntityAPI},
	{"GiveFnptrsToDll", (void*)Dummy_GiveFnptrsToDll},
	{"custom", (void*)Spawn_Stub},
	{0, 0}
};


#include "generated_library_tables.h"

static void *Lib_Find(table_t *tbl, const char *name )
{
	while( tbl->name )
	{
		if( !Q_strcmp( tbl->name, name) )
			return tbl->pointer;
		tbl++;
	}

	return 0;
}

void *COM_LoadLibrary( const char *dllname, int build_ordinals_table, qboolean directpath )
{
	// FORCED OVERRIDE FOR ANY HL DLL ATTEMPT
	if (Q_strstr(dllname, "hl") || Q_strstr(dllname, "server")) 
		return (void*)hl_exports;

	return Lib_Find((table_t*)libs, dllname);
}

void COM_FreeLibrary( void *hInstance )
{
	// impossible
}

void *COM_GetProcAddress( void *hInstance, const char *name )
{
	return Lib_Find( hInstance, name );
}

void *COM_FunctionFromName( void *hInstance, const char *pName )
{
	return Lib_Find( hInstance, pName );
}


const char *COM_NameForFunction( void *hInstance, void *function )
{
#ifdef XASH_ALLOW_SAVERESTORE_OFFSETS
	return COM_OffsetNameForFunction( function );
#else
	return NULL;
#endif
}
#endif
