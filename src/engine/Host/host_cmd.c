/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "cmd.h"
#include "cbuf.h"
#include <string.h>
#include <stdlib.h>
#include "gamedefs.h"
#include "server.h"
#undef SERVER   // TODO: remove this workaround
#include "client.h"
#include "console.h"
#include "keys.h"
#include "host.h"
#include "sys.h"
#include "protocol.h"
#include "common.h"
#include "world.h"
#include "versions.h"
#include "cvar_q1.h"
#include "Alias.h"
#include "screen.h"
#include "q_tools.h"
#include "msg.h"
#include "menu_prv.h"
#include "progs.h"

/*
==================
Host_Quit_f
==================
*/


void Host_Quit_f() {
    if ((key.dest != key_console) &&
        (cls.state != ca_dedicated)) {
        M_Menu_Quit_f();
        return;
    }
    CL_Disconnect();
    Host_ShutdownServer(false);

    Sys_Quit();
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f() {
    void (*print) (cStringRO fmt, ...);

    if (cmd_source == src_command) {
        if (!sv.active) { Cmd_ForwardToServer();    return; }
        print = Con_Printf;
    }
    else    print = SV_ClientPrintf;

    print("host:    %s\n", Cvar_VariableString("hostname"));
    print("version: %4.2f\n", VERSION);
    if (tcpipAvailable)     print("tcp/ip:  %s\n", my_tcpip_address);
    if (ipxAvailable)       print("ipx:     %s\n", my_ipx_address);
    print("map:     %s\n", sv.name);
    print("players: %i active (%i max)\n\n", net_activeconnections, svs.maxClients);

    RmtClient_p rClient = svs.clients;
    for (int32_t j = 0; j < svs.maxClients; j++, rClient++) {
        if (!rClient->active)    continue;

        int seconds = (int)(net_time - rClient->netconnection->connecttime);
        int minutes = seconds / 60;
        int hours = 0;

        if (minutes) {
            seconds -= (minutes * 60);
            hours = minutes / 60;
            if (hours)
                minutes -= (hours * 60);
        }
        else    hours = 0;

        print("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, rClient->name, (int32_t)rClient->edict->v.frags, hours, minutes, seconds);
        print("   %s\n", rClient->netconnection->address);
    }
}




/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f() {
    if (cmd_source == src_command) { Cmd_ForwardToServer(); return; }

    SV_ClientPrintf("Client ping times:\n");
    RmtClient_p rClient = svs.clients;
    for (int32_t i = 0; i < svs.maxClients; i++, rClient++) {
        if (!rClient->active)    continue;

        float total = 0;
        for (int j = 0; j < NUM_PING_TIMES; j++)
            total += rClient->ping_times[j];
        total /= NUM_PING_TIMES;
        SV_ClientPrintf("%4i %s\n", (int)(total * 1000), rClient->name);
    }
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f() {
    if (cmd_source != src_command)  return;

    cls.demonum = -1;  // stop demo loop in case this fails

    CL_Disconnect();
    Host_ShutdownServer(false);

    key.dest = key_game;   // remove console or menu
    SCR_BeginLoadingPlaque();

    cls.mapstring[0] = 0;
    for (int i = 0; i < Cmd_Argc(); i++) {
        strcat(cls.mapstring, Cmd_Argv(i));
        strcat(cls.mapstring, " ");
    }
    strcat(cls.mapstring, "\n");

    svs.serverflags = 0;   // haven't completed an episode yet

    char name[MAX_QPATH];
    strcpy(name, Cmd_Argv(1));
    SV_SpawnServer(name
#ifdef QUAKE2
        , NULL
#endif
    );

    if (!sv.active)     return;

    if (cls.state != ca_dedicated) {
        strcpy(cls.spawnparms, "");

        for (int i = 2; i < Cmd_Argc(); i++) {
            strcat(cls.spawnparms, Cmd_Argv(i));
            strcat(cls.spawnparms, " ");
        }

        Cmd_ExecuteString("connect local", src_command);
    }
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f() {
#ifdef QUAKE2
    if (Cmd_Argc() < 2) { ;                 Con_Printf("changelevel <levelname> : continue game on a new level\n"); return; }
    if (!sv.active || cls.demoplayback) { ; Con_Printf("Only the server may changelevel\n");                        return; }

    strcpy(level, Cmd_Argv(1));
    cString startspot;
    if (Cmd_Argc() == 2) { startspot = NULL; }
    else {
        char _startspot[MAX_QPATH];
        strcpy(_startspot, Cmd_Argv(2));
        startspot = _startspot;
    }

    SV_SaveSpawnparms();
    char level[MAX_QPATH];
    SV_SpawnServer(level, startspot);
#else

    if (Cmd_Argc() != 2) { ;                Con_Printf("changelevel <levelname> : continue game on a new level\n"); return; }
    if (!sv.active || cls.demoplayback) { ; Con_Printf("Only the server may changelevel\n");                        return; }
    SV_SaveSpawnparms();

    char level[MAX_QPATH];
    strcpy(level, Cmd_Argv(1));
    SV_SpawnServer(level);
#endif
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f() {
    if (cls.demoplayback ||
        !sv.active ||
        cmd_source != src_command
        )
        return;

    char mapname[MAX_QPATH];
    strcpy(mapname, sv.name); // must copy out, because it gets cleared
    // in sv_spawnserver
#ifdef QUAKE2
    char startspot[MAX_QPATH];
    strcpy(startspot, sv.startspot);
    SV_SpawnServer(mapname, startspot);
#else
    SV_SpawnServer(mapname);
#endif
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f() {
    SCR_BeginLoadingPlaque();
    cls.signon = 0;  // need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f() {
    cls.demonum = -1;  // stop demo loop in case this fails
    if (cls.demoplayback) {
        CL_StopPlayback();
        CL_Disconnect();
    }
    char name[MAX_QPATH];   strcpy(name, Cmd_Argv(1));
    CL_EstablishConnection(name);
    Host_Reconnect_f();
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define SAVEGAME_VERSION 5

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment(cString text) {
    for (int i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
        text[i] = ' ';

    memcpy(text, cl.levelname, strlen(cl.levelname));

    char kills[20];
    snprintf(kills, sizeof(kills), "kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
    memcpy(text + 22, kills, strlen(kills));
    // convert space to _ to make stdio happy
    for (int i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
        if (text[i] == ' ')
            text[i] = '_';

    text[SAVEGAME_COMMENT_LENGTH] = '\0';
}


/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f() {
    if (cmd_source != src_command)      return;
    if (!sv.active) { ;                 Con_Printf("Not playing a local game.\n");              return; }
    if (cl.intermission != IM_NONE) { ; Con_Printf("Can't save in intermission.\n");            return; }
    if (svs.maxClients != 1) { ;        Con_Printf("Can't save multiplayer games.\n");          return; }
    if (Cmd_Argc() != 2) { ;            Con_Printf("save <savename> : save a game\n");          return; }
    if (strstr(Cmd_Argv(1), "..")) { ;  Con_Printf("Relative pathnames are not allowed.\n");    return; }

    for (int32_t i = 0; i < svs.maxClients; i++) {
        if (svs.clients[i].active &&
            (svs.clients[i].edict->v.health <= 0)
            ) {
            Con_Printf("Can't savegame with a dead player\n");  return;
        }
    }

    char name[256];
    snprintf(name, sizeof(name), "%s/%s", com.gamedir, Cmd_Argv(1));
    COM_DefaultExtension(name, ".sav");

    Con_Printf("Saving game to %s...\n", name);
    FILE* saveFile = fopen(name, "w");
    if (!saveFile) { ;     Con_Printf("ERROR: couldn't open[w].\n"); return; }

    fprintf(saveFile, "%i\n", SAVEGAME_VERSION);
    char comment[SAVEGAME_COMMENT_LENGTH + 1];
    Host_SavegameComment(comment);
    fprintf(saveFile, "%s\n", comment);
    for (int i = 0; i < NUM_SPAWN_PARMS; i++)
        fprintf(saveFile, "%f\n", svs.clients->spawn_parms[i]);

    fprintf(saveFile, "%d\n", current_skill);
    fprintf(saveFile, "%s\n", sv.name);
    fprintf(saveFile, "%f\n", sv.time);

    // write the light styles

    for (int i = 0; i < MAX_LIGHTSTYLES; i++) {
        if (sv.lightstyles[i])  fprintf(saveFile, "%s\n", sv.lightstyles[i]);
        else                    fprintf(saveFile, "m\n");
    }


    ED_WriteGlobals(saveFile);
    for (uint32_t i = 0; i < sv.num_edicts; i++) {
        ED_Write(saveFile, ED_GetEDictByIdx(i));
        fflush(saveFile);
    }
    fclose(saveFile);
    Con_Printf("done.\n");
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f() {
    if (cmd_source != src_command)  return;
    if (Cmd_Argc() != 2) { ;    Con_Printf("load <savename> : load a game\n"); return; }

    cls.demonum = -1;  // stop demo loop in case this fails

    char name[MAX_OSPATH];
    snprintf(name, sizeof(name), "%s/%s", com.gamedir, Cmd_Argv(1));
    COM_DefaultExtension(name, ".sav");

    // we can't call SCR_BeginLoadingPlaque, because too much stack space has
    // been used.  The menu calls it before stuffing loadgame command
    // SCR_BeginLoadingPlaque ();

    Con_Printf("Loading game from %s...\n", name);
    FILE* loadFile = fopen(name, "r");
    if (!loadFile) { Con_Printf("ERROR: couldn't open[r].\n"); return; }

    int32_t version;
    fscanf(loadFile, "%i\n", &version);
    if (version != SAVEGAME_VERSION) { fclose(loadFile); Con_Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION); return; }

    char str[32768];
    fscanf(loadFile, "%s\n", str);

    float spawn_parms[NUM_SPAWN_PARMS];
    for (int i = 0; i < NUM_SPAWN_PARMS; i++)
        fscanf(loadFile, "%f\n", &spawn_parms[i]);
    // this silliness is so we can load 1.06 save files, which have float skill values

    float tfloat;
    fscanf(loadFile, "%f\n", &tfloat);
    current_skill = (int32_t)(tfloat + 0.1);
    Cvar_SetValue("skill", (float)current_skill);

#ifdef QUAKE2
    Cvar_SetValue("deathmatch", 0);
    Cvar_SetValue("coop", 0);
    Cvar_SetValue("teamplay", 0);
#endif

    char mapname[MAX_QPATH];
    fscanf(loadFile, "%s\n", mapname);
    float time;
    fscanf(loadFile, "%f\n", &time);

    CL_Disconnect_f();

    SV_SpawnServer(mapname
#ifdef QUAKE2
        , NULL
#endif
    );

    if (!sv.active) { Con_Printf("Couldn't load map\n");    return; }
    sv.paused = true;  // pause until all clients connect
    sv.loadgame = true;

    // load the light styles

    for (int i = 0; i < MAX_LIGHTSTYLES; i++) {
        fscanf(loadFile, "%s\n", str);
        sv.lightstyles[i] = Hunk_Alloc(strlen(str) + 1);
        strcpy(sv.lightstyles[i], str);
    }

    // load the edicts out of the savegame file
    int32_t entnum = -1;  // -1 is the globals
    while (!feof(loadFile)) {
        int32_t i = 0;
        for (; i < sizeof(str) - 1; i++) {
            int r = fgetc(loadFile);
            if ((r == EOF) || !r) break;

            str[i] = (char)r;
            if (r == '}') {
                i++;
                break;
            }
        }
        if (i == sizeof(str) - 1)   Host_SysError("Loadgame buffer overflow");
        str[i] = 0;
        cString start = str;
        start = COM_Parse(str);
        if (!com.token[0])  break;  // end of file
        if (strcmp(com.token, "{")) Host_SysError("First token isn't a brace");

        if (entnum == -1) { ED_ParseGlobals(start); }   // parse the global vars
        else { // parse an edict
            edict_p ent = ED_GetEDictByIdx((uint32_t)entnum);
            memset(&ent->v, 0, progs->entityfields * 4);
            ent->free = false;
            ED_ParseEdict(start, ent);

            // link it into the bsp tree
            if (!ent->free)
                SV_LinkEdict(ent, false);
        }

        entnum++;
    }

    sv.num_edicts = entnum;
    sv.time = time;

    fclose(loadFile);

    for (int i = 0; i < NUM_SPAWN_PARMS; i++)
        svs.clients->spawn_parms[i] = spawn_parms[i];

    if (cls.state != ca_dedicated) {
        CL_EstablishConnection("local");
        Host_Reconnect_f();
    }
}

#ifdef QUAKE2
void SaveGamestate() {
    char name[256];
    snprintf(name, sizeof(name), "%s/%s.gip", com.gamedir, sv.name);

    Con_Printf("Saving game to %s...\n", name);
    FILE* saveGStFile = fopen(name, "w");
    if (!saveGStFile) { Con_Printf("ERROR: couldn't open[w].\n");  return; }

    fprintf(saveGStFile, "%i\n", SAVEGAME_VERSION);

    char comment[SAVEGAME_COMMENT_LENGTH + 1];
    Host_SavegameComment(comment);
    fprintf(saveGStFile, "%s\n", comment);
    // for (int i = 0; i < NUM_SPAWN_PARMS; i++)
    //     fprintf(saveGStFile, "%f\n", svs.clients->spawn_parms[i]);
    fprintf(saveGStFile, "%f\n", skill.value);
    fprintf(saveGStFile, "%s\n", sv.name);
    fprintf(saveGStFile, "%f\n", sv.time);

    // write the light styles

    for (int i = 0; i < MAX_LIGHTSTYLES; i++) {
        if (sv.lightstyles[i])  fprintf(saveGStFile, "%s\n", sv.lightstyles[i]);
        else                    fprintf(saveGStFile, "m\n");
    }


    for (int i = svs.maxClients + 1; i < sv.num_edicts; i++) {
        edict_p ent = ED_GetEDictByIdx(i);
        if ((int32_t)ent->v.flags & FL_ARCHIVE_OVERRIDE)
            continue;
        fprintf(saveGStFile, "%i\n", i);
        ED_Write(saveGStFile, ent);
        fflush(saveGStFile);
    }
    fclose(saveGStFile);
    Con_Printf("done.\n");
}

int LoadGamestate(cString level, cString startspot) {
    char name[MAX_OSPATH];
    snprintf(name, sizeof(name), "%s/%s.gip", com.gamedir, level);

    Con_Printf("Loading game from %s...\n", name);
    FILE* loadGStFile = fopen(name, "r");
    if (!loadGStFile) { Con_Printf("ERROR: couldn't open[w].\n");  return -1; }

    int32_t version;    fscanf(loadGStFile, "%i\n", &version);
    if (version != SAVEGAME_VERSION) {
        fclose(loadGStFile);
        Con_Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
        return -1;
    }
    char str[32768];    fscanf(loadGStFile, "%s\n", str);
    // float spawn_parms[NUM_SPAWN_PARMS];
    // for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
    //  fscanf (loadGStFile, "%f\n", &spawn_parms[i]);
    float sk; fscanf(loadGStFile, "%f\n", &sk);
    Cvar_SetValue("skill", sk);

    char mapname[MAX_QPATH];    fscanf(loadGStFile, "%s\n", mapname);
    float time;                 fscanf(loadGStFile, "%f\n", &time);

    SV_SpawnServer(mapname, startspot);

    if (!sv.active) { Con_Printf("Couldn't load map\n"); return -1; }

    // load the light styles
    for (int i = 0; i < MAX_LIGHTSTYLES; i++) {
        fscanf(loadGStFile, "%s\n", str);
        sv.lightstyles[i] = Hunk_Alloc(strlen(str) + 1);
        strcpy(sv.lightstyles[i], str);
    }

    // load the edicts out of the savegame file
    int32_t  entnum;
    while (!feof(loadGStFile)) {
        fscanf(loadGStFile, "%i\n", &entnum);
        int32_t i = 0;
        for (; i < sizeof(str) - 1; i++) {
            int r = fgetc(loadGStFile);
            if (r == EOF || !r) break;
            str[i] = r;
            if (r == '}') { i++;    break; }
        }
        if (i == sizeof(str) - 1)   Host_SysError("Loadgame buffer overflow");
        str[i] = 0;
        cString start = str;
        start = COM_Parse(str);
        if (!com.token[0])  break;  // end of file
        if (strcmp(com.token, "{")) Host_SysError("First token isn't a brace");

        // parse an edict

        edict_p ent = ED_GetEDictByIdx(entnum);
        memset(&ent->v, 0, progs->entityfields * 4);
        ent->free = false;
        ED_ParseEdict(start, ent);

        // link it into the bsp tree
        if (!ent->free) SV_LinkEdict(ent, false);
    }

    // sv.num_edicts = entnum;
    sv.time = time;
    fclose(loadGStFile);

    // for (int i = 0; i < NUM_SPAWN_PARMS; i++)
    //     svs.clients->spawn_parms[i] = spawn_parms[i];

    return 0;
}

// changing levels within a unit
void Host_Changelevel2_f() {
    if (Cmd_Argc() < 2) { Con_Printf("changelevel2 <levelname> : continue game on a new level in the unit\n");  return; }
    if (!sv.active || cls.demoplayback) { Con_Printf("Only the server may changelevel\n");  return; }

    char level[MAX_QPATH];
    strcpy(level, Cmd_Argv(1));

    cString startspot;
    if (Cmd_Argc() == 2) { startspot = NULL; }
    else {
        char _startspot[MAX_QPATH];
        strcpy(_startspot, Cmd_Argv(2));
        startspot = _startspot;
    }

    SV_SaveSpawnparms();

    // save the current level's state
    SaveGamestate();

    // try to restore the new level
    if (LoadGamestate(level, startspot)) { SV_SpawnServer(level, startspot); }
}
#endif


//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f() {
    if (Cmd_Argc() == 1) { Con_Printf("\"name\" is \"%s\"\n", cl_name.string); return; }

    cString newName = (Cmd_Argc() == 2) ? Cmd_Argv(1) : Cmd_Args();
    newName[15] = 0;

    if (cmd_source == src_command) {
        if (Q_strcmp(cl_name.string, newName) == 0) return;

        Cvar_Set("_cl_name", newName);
        if (cls.state == ca_connected)
            Cmd_ForwardToServer();
        return;
    }
    if (remoteClient->name[0] &&
        strcmp(remoteClient->name, "unconnected") &&
        (Q_strcmp(remoteClient->name, newName) != 0)
        )
        Con_Printf("%s renamed to %s\n", remoteClient->name, newName);

    Q_strcpy(remoteClient->name, newName);
    remoteClient->edict->v.netname = PR_SetQString(remoteClient->name);

    // send notification to all clients
    sizebuf_p pBuf = &sv.reliable_datagram;
    MSG_WriteByte(pBuf, svc_updatename);    MSG_WriteByte(pBuf, (uint8_t)(remoteClient - svs.clients));    MSG_WriteString(pBuf, remoteClient->name);
}


void Host_Version_f() {
    Con_Printf("Version %4.2f\n", VERSION);
    Con_Printf("Exe: "__TIME__" "__DATE__"\n");
}

#ifdef IDGODS
void Host_Please_f() {
    if (cmd_source != src_command)  return;

    if ((Cmd_Argc() == 3) &&
        (Q_strcmp(Cmd_Argv(1), "#") == 0)) {
        int j = Q_atof(Cmd_Argv(2)) - 1;
        if ((j < 0) ||
            (j >= svs.maxClients) ||
            (!svs.clients[j].active)
            )
            return;

        RmtClient_p cl = &svs.clients[j];
        if (cl->privileged) {
            cl->privileged = false;
            cl->edict->v.flags = (int32_t)cl->edict->v.flags & ~(FL_GODMODE | FL_NOTARGET);
            cl->edict->v.movetype = MOVETYPE_WALK;
            noclip_anglehack = false;
        }
        else    cl->privileged = true;
    }

    if (Cmd_Argc() != 2)    return;

    {
        RmtClient_p cl = svs.clients;
        for (int j = 0; j < svs.maxClients; j++, cl++) {
            if (!cl->active)    continue;
            if (Q_strcasecmp(cl->name, Cmd_Argv(1)) == 0) {
                if (cl->privileged) {
                    cl->privileged = false;
                    cl->edict->v.flags = (int32_t)cl->edict->v.flags & ~(FL_GODMODE | FL_NOTARGET);
                    cl->edict->v.movetype = MOVETYPE_WALK;
                    noclip_anglehack = false;
                }
                else    cl->privileged = true;
                break;
            }
        }
    }
}
#endif


void Host_Say(bool teamonly) {
    bool  fromServer = false;
    if (cmd_source == src_command) {
        if (cls.state == ca_dedicated) {
            fromServer = true;
            teamonly = false;
        }
        else { Cmd_ForwardToServer();   return; }
    }

    if (Cmd_Argc() < 2)         return;

    RmtClient_p save = remoteClient;

    cString args = Cmd_Args();
    // remove quotes if present
    if (*args == '"') {
        args++;
        args[Q_strlen(args) - 1] = 0;
    }

    // turn on color set 1
    {
        char text[NAME_LENGTH];
        if (fromServer) snprintf(text, sizeof(text), "%c<%s> ", 1, hostname.string);
        else            snprintf(text, sizeof(text), "%c%s: ", 1, save->name);

        uint32_t j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
        if (Q_strlen(args) > j)
            args[j] = 0;
        strcat(text, args);
        strcat(text, "\n");

        RmtClient_p rClient = svs.clients;
        for (int32_t j = 0; j < svs.maxClients; j++, rClient++) {
            if (!rClient ||
                !rClient->active ||
                !rClient->spawned ||
                (
                    teamplay.value &&
                    teamonly &&
                    (rClient->edict->v.team != save->edict->v.team))
                )
                continue;
            remoteClient = rClient;
            SV_ClientPrintf("%s", text);
        }
        remoteClient = save;

        Host_Printf("%s", &text[1]);
    }
}


void Host_Say_f() { Host_Say(false); }
void Host_Say_Team_f() { Host_Say(true); }


void Host_Tell_f() {
    if (cmd_source == src_command) { Cmd_ForwardToServer(); return; }
    if (Cmd_Argc() < 3)     return;

    char text[NAME_LENGTH];
    Q_strcpy(text, remoteClient->name);
    Q_strcat(text, ": ");

    cString args = Cmd_Args();

    // remove quotes if present
    if (*args == '"') {
        args++;
        args[Q_strlen(args) - 1] = 0;
    }

    // check length & truncate if necessary
    uint32_t j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
    if (Q_strlen(args) > j)     args[j] = 0;

    strcat(text, args);
    strcat(text, "\n");

    RmtClient_p save = remoteClient;
    {
        RmtClient_p rClient = svs.clients;
        for (int32_t j = 0; j < svs.maxClients; j++, rClient++) {
            if (!rClient->active || !rClient->spawned)        continue;
            if (Q_strcasecmp(rClient->name, Cmd_Argv(1)))    continue;

            remoteClient = rClient;
            SV_ClientPrintf("%s", text);
            break;
        }
    }
    remoteClient = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f() {
    if (Cmd_Argc() == 1) {
        Con_Printf("\"color\" is \"%i %i\"\n", ((uint8_t)cl_color.value) >> 4, ((uint8_t)cl_color.value) & 0x0f);
        Con_Printf("color <0-13> [0-13]\n");
        return;
    }

    uint8_t top = (uint8_t)atoi(Cmd_Argv(1));
    uint8_t bottom = (uint8_t)atoi(
        Cmd_Argv(
            (Cmd_Argc() == 2) ?
            1 : 2
        )
    );

    top &= 15;
    if (top > 13)
        top = 13;
    bottom &= 15;
    if (bottom > 13)
        bottom = 13;

    uint8_t playercolor = (uint8_t)(((uint16_t)top << 4) + bottom);

    if (cmd_source == src_command) {
        Cvar_SetValue("_cl_color", playercolor);
        if (cls.state == ca_connected)  Cmd_ForwardToServer();
        return;
    }

    remoteClient->colors = playercolor;
    remoteClient->edict->v.team = bottom + 1;

    // send notification to all clients
    sizebuf_p pBuf = &sv.reliable_datagram;
    MSG_WriteByte(pBuf, svc_updatecolors);  MSG_WriteByte(pBuf, (uint8_t)(remoteClient - svs.clients));    MSG_WriteByte(pBuf, remoteClient->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f() {
    if (cmd_source == src_command) { ;  Cmd_ForwardToServer();                                  return; }
    if (sv_player->v.health <= 0) { ;   SV_ClientPrintf("Can't suicide -- allready dead!\n");   return; }

    pr_global_struct->time = (float)sv.time;
    pr_global_struct->self = ED_GetEDictOffs(sv_player);
    PR_ExecuteProgram(pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f() {
    if (cmd_source == src_command) { Cmd_ForwardToServer(); return; }
    if (!pausable.value)            SV_ClientPrintf("Pause not allowed.\n");
    else {
        sv.paused ^= 1;

        SV_BroadcastPrintf("%s %spaused the game\n", (sv.paused) ? "" : "un", PR_GetQString(sv_player->v.netname));

        // send notification to all clients
        sizebuf_p pBuf = &sv.reliable_datagram;
        MSG_WriteByte(pBuf, svc_setpause);  MSG_WriteByte(pBuf, sv.paused);
    }
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f() {
    if (cmd_source == src_command) { ;  Con_Printf("prespawn is not valid from the console\n"); return; }
    if (remoteClient->spawned) { ;       Con_Printf("prespawn not valid -- allready spawned\n"); return; }

    sizebuf_p pBuf = &remoteClient->message;
    SZ_Write(pBuf, sv.signon.data, (size_t)sv.signon.cursize);
    MSG_WriteByte(pBuf, svc_signonnum);    MSG_WriteByte(pBuf, 2);
    remoteClient->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f() {
    if (cmd_source == src_command) { ;  Con_Printf("spawn is not valid from the console\n");    return; }
    if (remoteClient->spawned) { ;      Con_Printf("Spawn not valid -- allready spawned\n");    return; }

    // run the entrance script
    if (sv.loadgame) { // loaded games are fully inited allready
        // if this is the last client to be connected, unpause
        sv.paused = false;
    }
    else {
        // set up the edict
        edict_p ent = remoteClient->edict;
        memset(&ent->v, 0, progs->entityfields * 4);
        ent->v.colormap = (float)ED_GetEDictIdx(ent);
        ent->v.team = (float)(remoteClient->colors & 15) + 1;
        ent->v.netname = PR_SetQString(remoteClient->name);

        // copy spawn parms out of the RmtClient_t

        for (int i = 0; i < NUM_SPAWN_PARMS; i++)
            (&pr_global_struct->parm1)[i] = remoteClient->spawn_parms[i];

        // call the spawn function

        pr_global_struct->time = (float)sv.time;
        pr_global_struct->self = ED_GetEDictOffs(sv_player);
        PR_ExecuteProgram(pr_global_struct->ClientConnect);

        if ((Host_FloatTime() - remoteClient->netconnection->connecttime) <= sv.time)
            Host_Printf("%s entered the game\n", remoteClient->name);

        PR_ExecuteProgram(pr_global_struct->PutClientInServer);
    }


    // send all current names, colors, and frag counts
    sizebuf_p pBuf = &remoteClient->message;    SZ_Clear(pBuf);

    // send time of update
    MSG_WriteByte(pBuf, svc_time);    MSG_WriteFloat(pBuf, (float)sv.time);
    {
        RmtClient_p rClient = svs.clients;
        for (uint8_t i = 0; i < svs.maxClients; i++, rClient++) {
            MSG_WriteByte(pBuf, svc_updatename);    MSG_WriteByte(pBuf, i); MSG_WriteString(pBuf, rClient->name);
            MSG_WriteByte(pBuf, svc_updatefrags);   MSG_WriteByte(pBuf, i); MSG_WriteShort(pBuf, rClient->old_frags);
            MSG_WriteByte(pBuf, svc_updatecolors);  MSG_WriteByte(pBuf, i); MSG_WriteByte(pBuf, rClient->colors);
        }
    }
    // send all current light styles
    for (int i = 0; i < MAX_LIGHTSTYLES; i++) {
        MSG_WriteByte(pBuf, svc_lightstyle); MSG_WriteByte(pBuf, (char)i); MSG_WriteString(pBuf, sv.lightstyles[i]);
    }

    //
    // send some stats
    //
    MSG_WriteByte(pBuf, svc_updatestat); MSG_WriteByte(pBuf, STAT_TOTALSECRETS);  MSG_WriteLong(pBuf, (int32_t)pr_global_struct->total_secrets);
    MSG_WriteByte(pBuf, svc_updatestat); MSG_WriteByte(pBuf, STAT_TOTALMONSTERS); MSG_WriteLong(pBuf, (int32_t)pr_global_struct->total_monsters);
    MSG_WriteByte(pBuf, svc_updatestat); MSG_WriteByte(pBuf, STAT_SECRETS);       MSG_WriteLong(pBuf, (int32_t)pr_global_struct->found_secrets);
    MSG_WriteByte(pBuf, svc_updatestat); MSG_WriteByte(pBuf, STAT_MONSTERS);      MSG_WriteLong(pBuf, (int32_t)pr_global_struct->killed_monsters);


    //
    // send a fixangle
    // Never send a roll angle, because savegames can catch the server
    // in a state where it is expecting the client to correct the angle
    // and it won't happen if the game was just loaded, so you wind up
    // with a permanent head tilt
    edict_p ent = ED_GetEDictByIdx(1 + (uint32_t)(remoteClient - svs.clients));
    MSG_WriteByte(pBuf, svc_setangle); MSG_WriteAngle(pBuf, ent->v.angles[0]); MSG_WriteAngle(pBuf, ent->v.angles[1]); MSG_WriteAngle(pBuf, 0);

    SV_WriteClientdataToMessage(sv_player, pBuf);

    MSG_WriteByte(pBuf, svc_signonnum);    MSG_WriteByte(pBuf, 3);
    remoteClient->sendsignon = true;
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f() {
    if (cmd_source == src_command) { Con_Printf("begin is not valid from the console\n"); return; }

    remoteClient->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f() {
    if (cmd_source == src_command) {
        if (!sv.active) {
            Cmd_ForwardToServer(); return;
        }
    }
    else
        if (pr_global_struct->deathmatch && !remoteClient->privileged)
            return;

    RmtClient_p save = remoteClient;
    int32_t i;
    bool byNumber = false;
    if ((Cmd_Argc() > 2) &&
        (Q_strcmp(Cmd_Argv(1), "#") == 0)
        ) {
        i = (int32_t)Q_atof(Cmd_Argv(2)) - 1;
        if ((i < 0) ||
            (i >= svs.maxClients) ||
            (!svs.clients[i].active)
            )
            return;
        remoteClient = &svs.clients[i];
        byNumber = true;
    }
    else {
        remoteClient = svs.clients;
        for (i = 0; i < svs.maxClients; i++, remoteClient++) {
            if (!remoteClient->active)                               continue;
            if (Q_strcasecmp(remoteClient->name, Cmd_Argv(1)) == 0)  break;
        }
    }

    if (i < svs.maxClients) {
        cString who;
        if (cmd_source == src_command)
            if (cls.state == ca_dedicated)  who = "Console";
            else                            who = cl_name.string;
        else                                who = save->name;

        // can't kick yourself!
        if (remoteClient == save)    return;

        cString message = NULL;
        if (Cmd_Argc() > 2) {
            message = COM_Parse(Cmd_Args());
            if (byNumber) {
                message++;       // skip the #
                while (*message == ' ')    // skip white space
                    message++;
                message += Q_strlen(Cmd_Argv(2)); // skip the number
            }
            while (*message && *message == ' ')
                message++;
        }
        if (message)    SV_ClientPrintf("Kicked by %s: %s\n", who, message);
        else            SV_ClientPrintf("Kicked by %s\n", who);
        SV_DropClient(false);
    }

    remoteClient = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/
void Host_Give_f() {
    if (cmd_source == src_command) { Cmd_ForwardToServer();        return; }
    if (pr_global_struct->deathmatch && !remoteClient->privileged)  return;

    cString t = Cmd_Argv(1);
    int cVal = atoi(Cmd_Argv(2));

    switch (t[0]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        // MED 01/04/97 added hipnotic give stuff
        if (hipnotic) {
            if (t[0] == '6') {
                if (t[1] == 'a')    sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
                else                sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
            }
            else if (t[0] == '9')   sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
            else if (t[0] == '0')   sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
            else if (t[0] >= '2')   sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
        }
        else {
            if (t[0] >= '2')    sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
        }
        break;

    case 's':
        if (rogue) {
            eval_p val = GetEdictFieldValue(sv_player, "ammo_shells1");
            if (val)
                val->_float = (float)cVal;
        }

        sv_player->v.ammo_shells = (float)cVal;
        break;
    case 'n':
        if (rogue) {
            eval_p val = GetEdictFieldValue(sv_player, "ammo_nails1");
            if (val) {
                val->_float = (float)cVal;
                if (sv_player->v.weapon <= IT_LIGHTNING)    sv_player->v.ammo_nails = (float)cVal;
            }
        }
        else {
            sv_player->v.ammo_nails = (float)cVal;
        }
        break;
    case 'l':
        if (rogue) {
            eval_p val = GetEdictFieldValue(sv_player, "ammo_lava_nails");
            if (val) {
                val->_float = (float)cVal;
                if (sv_player->v.weapon > IT_LIGHTNING)     sv_player->v.ammo_nails = (float)cVal;
            }
        }
        break;
    case 'r':
        if (rogue) {
            eval_p val = GetEdictFieldValue(sv_player, "ammo_rockets1");
            if (val) {
                val->_float = (float)cVal;
                if (sv_player->v.weapon <= IT_LIGHTNING)    sv_player->v.ammo_rockets = (float)cVal;
            }
        }
        else { sv_player->v.ammo_rockets = (float)cVal; }
        break;
    case 'm':
        if (rogue) {
            eval_p val = GetEdictFieldValue(sv_player, "ammo_multi_rockets");
            if (val) {
                val->_float = (float)cVal;
                if (sv_player->v.weapon > IT_LIGHTNING)     sv_player->v.ammo_rockets = (float)cVal;
            }
        }
        break;
    case 'h':   sv_player->v.health = (float)cVal; break;
    case 'c':
        if (rogue) {
            eval_p val = GetEdictFieldValue(sv_player, "ammo_cells1");
            if (val) {
                val->_float = (float)cVal;
                if (sv_player->v.weapon <= IT_LIGHTNING)    sv_player->v.ammo_cells = (float)cVal;
            }
        }
        else { sv_player->v.ammo_cells = (float)cVal; }
        break;
    case 'p':
        if (rogue) {
            eval_p val = GetEdictFieldValue(sv_player, "ammo_plasma");
            if (val) {
                val->_float = (float)cVal;
                if (sv_player->v.weapon > IT_LIGHTNING)     sv_player->v.ammo_cells = (float)cVal;
            }
        }
        break;
    }
}


/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f() {
    edict_p eDict = FindViewthing();
    if (!eDict) return;

    Model_p mdl = Mod_ForName(Cmd_Argv(1), false);
    if (!mdl) { Con_Printf("Can't load %s\n", Cmd_Argv(1)); return; }

    eDict->v.frame = 0;
    cl.model_precache[(int)eDict->v.modelindex] = mdl;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f() {
    edict_p eDict = FindViewthing();
    if (!eDict)     return;

    Model_p mdl = cl.model_precache[(int)eDict->v.modelindex];
    int frame = atoi(Cmd_Argv(1));
    if (frame >= mdl->numframes)
        frame = mdl->numframes - 1;

    eDict->v.frame = (float)frame;
}


void PrintFrameName(Model_p mdl, int frame) {
    AliasHdr_p hdr = (AliasHdr_p)Mod_Extradata(mdl);
    if (!hdr)   return;

    mAliasFrameDesc_p pframedesc = &hdr->frames[frame];
    Con_Printf("frame %i: %s\n", frame, pframedesc->name);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f() {
    edict_p eDict = FindViewthing();
    if (!eDict)     return;

    Model_p mdl = cl.model_precache[(int)eDict->v.modelindex];
    eDict->v.frame++;
    if (eDict->v.frame >= mdl->numframes)
        eDict->v.frame = (float)mdl->numframes - 1;

    PrintFrameName(mdl, (int)eDict->v.frame);
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f() {
    edict_p eDict = FindViewthing();
    if (!eDict)     return;

    Model_p mdl = cl.model_precache[(int)eDict->v.modelindex];
    eDict->v.frame--;
    if (eDict->v.frame < 0)
        eDict->v.frame = 0;

    PrintFrameName(mdl, (int)eDict->v.frame);
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f() {
    if (cls.state == ca_dedicated) {
        if (!sv.active)
            Cbuf_AddText("map start\n");
        return;
    }

    int demo = Cmd_Argc() - 1;
    if (demo > MAX_DEMOS) {
        Con_Printf("Max %i demos in demoloop\n", MAX_DEMOS);
        demo = MAX_DEMOS;
    }
    Con_Printf("%i demo(s) in loop\n", demo);

    for (int i = 1; i < demo + 1; i++)
        strncpy(cls.demos[i - 1], Cmd_Argv(i), sizeof(cls.demos[0]) - 1);

    if ((!sv.active) &&
        (cls.demonum != -1) &&
        (!cls.demoplayback)
        ) {
        cls.demonum = 0;
        CL_NextDemo();
    }
    else cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f() {
    if (cls.state == ca_dedicated)  return;
    if (cls.demonum == -1)  cls.demonum = 1;

    CL_Disconnect_f();
    CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f() {
    if ((cls.state == ca_dedicated) || (!cls.demoplayback)) return;

    CL_StopPlayback();
    CL_Disconnect();
}

//=============================================================================
