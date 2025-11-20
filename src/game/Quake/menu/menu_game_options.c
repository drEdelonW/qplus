#include "menu.h"
#include "menu_prv.h"
#include "server.h"
#include "cvar_q1.h"
#include "host.h"
#include "screen.h"
#include "cbuf.h"
#include "gamedefs.h"

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct {
    cString name;
    cString description;
} level_t;

typedef struct {
    int     firstLevel;
    int     levels;
    cString description;
} episode_t;


episode_t episodes[] = {
    {0,  1, "Welcome to Quake"},
    {1,  8, "Doomed Dimension"},
    {9,  7, "Realm of Black Magic"},
    {16, 7, "Netherworld"},
    {23, 8, "The Elder World"},
    {31, 1, "Final Level"},
    {32, 6, "Deathmatch Arena"}
};

level_t levels[] = {
    {"start", "Entrance"},              // 0

    {"e1m1", "Slipgate Complex"},       // 1
    {"e1m2", "Castle of the Damned"},
    {"e1m3", "The Necropolis"},
    {"e1m4", "The Grisly Grotto"},
    {"e1m5", "Gloom Keep"},
    {"e1m6", "The Door To Chthon"},
    {"e1m7", "The House of Chthon"},
    {"e1m8", "Ziggurat Vertigo"},

    {"e2m1", "The Installation"},       // 9
    {"e2m2", "Ogre Citadel"},
    {"e2m3", "Crypt of Decay"},
    {"e2m4", "The Ebon Fortress"},
    {"e2m5", "The Wizard's Manse"},
    {"e2m6", "The Dismal Oubliette"},
    {"e2m7", "Underearth"},

    {"e3m1", "Termination Central"},    // 16
    {"e3m2", "The Vaults of Zin"},
    {"e3m3", "The Tomb of Terror"},
    {"e3m4", "Satan's Dark Delight"},
    {"e3m5", "Wind Tunnels"},
    {"e3m6", "Chambers of Torment"},
    {"e3m7", "The Haunted Halls"},

    {"e4m1", "The Sewage System"},      // 23
    {"e4m2", "The Tower of Despair"},
    {"e4m3", "The Elder God Shrine"},
    {"e4m4", "The Palace of Hate"},
    {"e4m5", "Hell's Atrium"},
    {"e4m6", "The Pain Maze"},
    {"e4m7", "Azure Agony"},
    {"e4m8", "The Nameless City"},

    {"end", "Shub-Niggurath's Pit"},    // 31

    {"dm1", "Place of Two Deaths"},     // 32
    {"dm2", "Claustrophobopolis"},
    {"dm3", "The Abandoned Base"},
    {"dm4", "The Bad Place"},
    {"dm5", "The Cistern"},
    {"dm6", "The Dark Zone"}
};



//MED 01/06/97  added hipnotic episodes
episode_t   hipnoticepisodes[] = {
    {0,  1, "Scourge of Armagon"},
    {1,  5, "Fortress of the Dead"},
    {6,  6, "Dominion of Darkness"},
    {12, 4, "The Rift"},
    {16, 1, "Final Level"},
    {17, 1, "Deathmatch Arena"}
};

//MED 01/06/97 added hipnotic levels
level_t hipnoticlevels[] = {
    {"start", "Command HQ"},            // 0

    {"hip1m1", "The Pumping Station"},  // 1
    {"hip1m2", "Storage Facility"},
    {"hip1m3", "The Lost Mine"},
    {"hip1m4", "Research Facility"},
    {"hip1m5", "Military Complex"},

    {"hip2m1", "Ancient Realms"},       // 6
    {"hip2m2", "The Black Cathedral"},
    {"hip2m3", "The Catacombs"},
    {"hip2m4", "The Crypt"},
    {"hip2m5", "Mortum's Keep"},
    {"hip2m6", "The Gremlin's Domain"},

    {"hip3m1", "Tur Torment"},          // 12
    {"hip3m2", "Pandemonium"},
    {"hip3m3", "Limbo"},
    {"hip3m4", "The Gauntlet"},

    {"hipend", "Armagon's Lair"},       // 16

    {"hipdm1", "The Edge of Oblivion"}  // 17
};



//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t rogueepisodes[] = {
    {0,  1, "Introduction"},
    {1,  7, "Hell's Fortress"},
    {8,  8, "Corridors of Time"},
    {16, 1, "Deathmatch Arena"}
};
//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t  roguelevels[] = {
    {"start", "Split Decision"},    // 0

    {"r1m1", "Deviant's Domain"},   // 1
    {"r1m2", "Dread Portal"},
    {"r1m3", "Judgement Call"},
    {"r1m4", "Cave of Death"},
    {"r1m5", "Towers of Wrath"},
    {"r1m6", "Temple of Pain"},
    {"r1m7", "Tomb of the Overlord"},

    {"r2m1", "Tempus Fugit"},       // 8
    {"r2m2", "Elemental Fury I"},
    {"r2m3", "Elemental Fury II"},
    {"r2m4", "Curse of Osiris"},
    {"r2m5", "Wizard's Keep"},
    {"r2m6", "Blood Sacrifice"},
    {"r2m7", "Last Bastion"},
    {"r2m8", "Source of Evil"},

    {"ctf1", "Division of Change"}  // 16
};


static int  _startEpisode;
static int  _startLevel;
static int  _maxPlayers;
static bool _svInfoMsg = false;
static double   _svInfoMsgTime;

void M_Menu_GameOptions_f() {
    key.dest = key_menu;
    m_state = m_gameoptions;
    m_entersound = true;
    if (_maxPlayers == 0)    _maxPlayers = svs.maxClients;
    if (_maxPlayers < 2)     _maxPlayers = svs.maxClientsLimit;
}


static int _cur_ys[] = {
    40,     // begin game
    56,     // Max players
    64,     // Game Type
    72,     // Teamplay
    80,     // Skill
    88,     // Frag Limit
    96,     // Time Limit
    112,    // Episode
    120,    // Level
    128     // Map
};
typedef enum {
    go_force_signed = -1,
    go_FIRST = 0,

    go_BeginGame = go_FIRST,
    go_MaxPlayer,
    go_GameType,
    go_TeamPlay,
    go_Skill,
    go_FragLimit,
    go_TimeLimit,
    go_Episode,
    go_Level,

    go_LAST     //should be last
} GOm_e;
static GOm_e  _cursor;

void M_GameOptions_Draw() {
    static int _x2 = 160;
    int yIdx = 0;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    M_DrawPicHC(4, Draw_CachePic("gfx/p_multi.lmp"));

    M_DrawTextBox(_x2 - 8, _cur_ys[yIdx] - 8, 10, 1); {
        M_Print(_x2, _cur_ys[yIdx++], "begin game");
    }

    M_Print(0, _cur_ys[yIdx], "      Max players");     M_Print(_x2, _cur_ys[yIdx++], va("%i", _maxPlayers));
    M_Print(0, _cur_ys[yIdx], "        Game Type");     M_Print(_x2, _cur_ys[yIdx++], (coop.value) ? "Cooperative" : "Deathmatch");

    M_Print(0, _cur_ys[yIdx], "        Teamplay"); {
        if (rogue) {
            cString msg;

            switch ((int)teamplay.value) {
            case 1:     msg = "No Friendly Fire";   break;
            case 2:     msg = "Friendly Fire";      break;
            case 3:     msg = "Tag";                break;
            case 4:     msg = "Capture the Flag";   break;
            case 5:     msg = "One Flag CTF";       break;
            case 6:     msg = "Three Team CTF";     break;
            default:    msg = "Off";                break;
            }
            M_Print(_x2, _cur_ys[yIdx++], msg);
        }
        else {
            cString msg;

            switch ((int)teamplay.value) {
            case 1:     msg = "No Friendly Fire";   break;
            case 2:     msg = "Friendly Fire";      break;
            default:    msg = "Off";                break;
            }
            M_Print(_x2, _cur_ys[yIdx++], msg);
        }
    }

    M_Print(0, _cur_ys[yIdx], "            Skill"); {
        if (skill.value == 0)       M_Print(_x2, _cur_ys[yIdx++], "Easy difficulty");
        else if (skill.value == 1)  M_Print(_x2, _cur_ys[yIdx++], "Normal difficulty");
        else if (skill.value == 2)  M_Print(_x2, _cur_ys[yIdx++], "Hard difficulty");
        else                        M_Print(_x2, _cur_ys[yIdx++], "Nightmare difficulty");
    }
    M_Print(0, _cur_ys[yIdx], "       Frag Limit"); {
        if (fraglimit.value == 0)   M_Print(_x2, _cur_ys[yIdx++], "none");
        else                        M_Print(_x2, _cur_ys[yIdx++], va("%i frags", (int)fraglimit.value));
    }
    M_Print(0, _cur_ys[yIdx], "       Time Limit"); {
        if (timelimit.value == 0)   M_Print(_x2, _cur_ys[yIdx++], "none");
        else                        M_Print(_x2, _cur_ys[yIdx++], va("%i minutes", (int)timelimit.value));
    }
    M_Print(0, _cur_ys[yIdx], "         Episode"); {
        //MED 01/06/97 added hipnotic episodes
        if (hipnotic)               M_Print(_x2, _cur_ys[yIdx++], hipnoticepisodes[_startEpisode].description);
        //PGM 01/07/97 added rogue episodes
        else if (rogue)             M_Print(_x2, _cur_ys[yIdx++], rogueepisodes[_startEpisode].description);
        else                        M_Print(_x2, _cur_ys[yIdx++], episodes[_startEpisode].description);
    }
    M_Print(0, _cur_ys[yIdx], "           Level"); {
        //MED 01/06/97 added hipnotic episodes
        if (hipnotic) {
            M_Print(_x2, _cur_ys[yIdx++], hipnoticlevels[hipnoticepisodes[_startEpisode].firstLevel + _startLevel].description);
            M_Print(_x2, _cur_ys[yIdx++], hipnoticlevels[hipnoticepisodes[_startEpisode].firstLevel + _startLevel].name);
        }
        //PGM 01/07/97 added rogue episodes
        else if (rogue) {
            M_Print(_x2, _cur_ys[yIdx++], roguelevels[rogueepisodes[_startEpisode].firstLevel + _startLevel].description);
            M_Print(_x2, _cur_ys[yIdx++], roguelevels[rogueepisodes[_startEpisode].firstLevel + _startLevel].name);
        }
        else {
            M_Print(_x2, _cur_ys[yIdx++], levels[episodes[_startEpisode].firstLevel + _startLevel].description);
            M_Print(_x2, _cur_ys[yIdx++], levels[episodes[_startEpisode].firstLevel + _startLevel].name);
        }
    }

    // line cursor
    M_DrawCharacter(144, _cur_ys[_cursor], curSymb());

    if (_svInfoMsg) {
        if ((realtime - _svInfoMsgTime) < 5.0) {
            int x = (320 - 26 * 8) / 2;
            M_DrawTextBox(x, 138, 24, 4);
            x += 8;
            M_Print(x, 146, "  More than 4 players   ");
            M_Print(x, 154, " requires using command ");
            M_Print(x, 162, "line parameters; please ");
            M_Print(x, 170, "   see techinfo.txt.    ");
        }
        else {
            _svInfoMsg = false;
        }
    }
}


void M_NetStart_Change(int dir) {
    int count;

    switch (_cursor) {
    case go_MaxPlayer: {
        _maxPlayers += dir;
        if (_maxPlayers > svs.maxClientsLimit) {
            _maxPlayers = svs.maxClientsLimit;
            _svInfoMsg = true;
            _svInfoMsgTime = realtime;
        }
        if (_maxPlayers < 2)
            _maxPlayers = 2;
    } break;

    case go_GameType: Cvar_SetValue("coop", coop.value ? 0 : 1);  break;

    case go_TeamPlay: {
        if (rogue)  count = 6;
        else        count = 2;

        Cvar_SetValue("teamplay", teamplay.value + dir);
        if (teamplay.value > count)     Cvar_SetValue("teamplay", 0);
        else if (teamplay.value < 0)    Cvar_SetValue("teamplay", count);
    } break;

    case go_Skill: {
        Cvar_SetValue("skill", skill.value + dir);
        if (skill.value > 3)        Cvar_SetValue("skill", 0);
        else if (skill.value < 0)   Cvar_SetValue("skill", 3);
    } break;

    case go_FragLimit: {
        Cvar_SetValue("fraglimit", fraglimit.value + dir * 10);
        if (fraglimit.value > 100)      Cvar_SetValue("fraglimit", 0);
        else if (fraglimit.value < 0)   Cvar_SetValue("fraglimit", 100);
    } break;

    case go_TimeLimit: {
        Cvar_SetValue("timelimit", timelimit.value + dir * 5);
        if (timelimit.value > 60)       Cvar_SetValue("timelimit", 0);
        else if (timelimit.value < 0)   Cvar_SetValue("timelimit", 60);
    } break;

    case go_Episode: {
        _startEpisode += dir;
        //MED 01/06/97 added hipnotic count
        //PGM 01/07/97 added rogue count
        //PGM 03/02/97 added 1 for dmatch episode
        if (hipnotic)               count = 6;
        else if (rogue)             count = 4;
        else if (registered.value)  count = 7;
        else                        count = 2;

        if (_startEpisode < 0)              _startEpisode = count - 1;
        else if (_startEpisode >= count)    _startEpisode = 0;

        _startLevel = 0;
    } break;

    case go_Level: {
        _startLevel += dir;
        //MED 01/06/97 added hipnotic episodes
        if (hipnotic)   count = hipnoticepisodes[_startEpisode].levels;
        //PGM 01/06/97 added hipnotic episodes
        else if (rogue) count = rogueepisodes[_startEpisode].levels;
        else            count = episodes[_startEpisode].levels;

        if (_startLevel < 0)            _startLevel = count - 1;
        else if (_startLevel >= count)  _startLevel = 0;
    } break;
    default: break;

    }
}

void M_GameOptions_Key(keycode_t key) {
    switch (key) {
    case K_ESCAPE:  M_Menu_Net_f(); break;

    case K_UPARROW: {
        S_LocalSound("misc/menu1.wav");
        if (--_cursor < go_FIRST)
            _cursor = go_LAST - 1;
    } break;

    case K_DOWNARROW: {
        S_LocalSound("misc/menu1.wav");
        if (++_cursor >= go_LAST)
            _cursor = go_FIRST;
    } break;

    case K_LEFTARROW: {
        if (_cursor == go_BeginGame)    break;

        S_LocalSound("misc/menu3.wav");
        M_NetStart_Change(-1);
    } break;

    case K_RIGHTARROW: {
        if (_cursor == go_BeginGame)    break;
        S_LocalSound("misc/menu3.wav");
        M_NetStart_Change(1);
    } break;

    case K_ENTER: {
        S_LocalSound("misc/menu2.wav");
        if (_cursor == 0) {
            if (sv.active)
                Cbuf_AddText("disconnect\n");
            Cbuf_AddText("listen 0\n"); // so host_netport will be re-examined
            Cbuf_AddText(va("maxplayers %u\n", _maxPlayers));
            SCR_BeginLoadingPlaque();

            if (hipnotic)   Cbuf_AddText(va("map %s\n", hipnoticlevels[hipnoticepisodes[_startEpisode].firstLevel + _startLevel].name));
            else if (rogue) Cbuf_AddText(va("map %s\n", roguelevels[rogueepisodes[_startEpisode].firstLevel + _startLevel].name));
            else            Cbuf_AddText(va("map %s\n", levels[episodes[_startEpisode].firstLevel + _startLevel].name));

            return;
        }

        M_NetStart_Change(1);
    } break;
    default:    break;
    }
}
