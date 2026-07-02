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
// sbar.c -- status bar code

// #include "sbar.h"
#include "qPic.h"
#include "gamedefs.h"
#include "cvar_q1.h"
#include "draw.h"
// #include "common.h"
#include "VA.h"
#include "cmd.h"
#include "client.h"
#include "screen.h"
#include <string.h>

#define SBAR_HEIGHT (24)

int     sb_lines;   // scan lines to draw
#define STAT_MINUS  10 // num frame for '-' stats digit

typedef struct {
    int     updates;  // if >= vid.numpages, no update needed
    qPic_p  nums[2][11];
    qPic_p  colon;
    qPic_p  slash;
    qPic_p  ibar;
    qPic_p  sbar;
    qPic_p  scorebar;

    qPic_p  weapons[7][8];   // 0 is active, 1 is owned, 2-5 are flashes
    qPic_p  ammo[4];
    qPic_p  sigil[4];
    qPic_p  armor[3];
    qPic_p  items[32];

    qPic_p  faces[7][2];  // 0 is gibbed, 1 is dead, 2-6 are alive
    // 0 is static, 1 is temporary animation
    struct {
        qPic_p  invis;
        qPic_p  quad;
        qPic_p  invuln;
        qPic_p  invis_invuln;
    } face;
    bool    showscores;
} sBar_t;
static sBar_t _sb;

qPic_p  rsb_invbar[2];
qPic_p  rsb_weapons[5];
qPic_p  rsb_items[2];
qPic_p  rsb_ammo[3];
qPic_p  rsb_teambord;  // PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
qPic_p  hsb_weapons[7][5];   // 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
int hipweapons[4] = {
    HIT_LASER_CANNON_BIT,
    HIT_MJOLNIR_BIT,
    4,
    HIT_PROXIMITY_GUN_BIT
};
//MED 01/04/97 added hipnotic items array
qPic_p hsb_items[2];

void Sbar_MiniDeathmatchOverlay();
void Sbar_DeathmatchOverlay();
void M_DrawPic(int x, int y, qPic_p pic);

/*
    ===============
    Sbar_ShowScores

    Tab key down
    ===============
*/
void Sbar_ShowScores() {
    if (_sb.showscores)  return;
    _sb.showscores = true;
    _sb.updates = 0;
}

/*
    ===============
    Sbar_DontShowScores

    Tab key up
    ===============
*/
void Sbar_DontShowScores() {
    _sb.showscores = false;
    _sb.updates = 0;
}

/*
    ===============
    Sbar_Changed
    ===============
*/
void Sbar_Changed() {
    _sb.updates = 0; // update next frame
}

/*
    ===============
    Sbar_Init
    ===============
*/
void Sbar_Init() {
    for (int i = 0; i < 10; i++) {
        _sb.nums[0][i] = Draw_PicFromWad(va("num_%i", i));
        _sb.nums[1][i] = Draw_PicFromWad(va("anum_%i", i));
    }

    _sb.nums[0][10] = Draw_PicFromWad("num_minus");
    _sb.nums[1][10] = Draw_PicFromWad("anum_minus");

    _sb.colon = Draw_PicFromWad("num_colon");
    _sb.slash = Draw_PicFromWad("num_slash");

    _sb.weapons[0][0] = Draw_PicFromWad("inv_shotgun");
    _sb.weapons[0][1] = Draw_PicFromWad("inv_sshotgun");
    _sb.weapons[0][2] = Draw_PicFromWad("inv_nailgun");
    _sb.weapons[0][3] = Draw_PicFromWad("inv_snailgun");
    _sb.weapons[0][4] = Draw_PicFromWad("inv_rlaunch");
    _sb.weapons[0][5] = Draw_PicFromWad("inv_srlaunch");
    _sb.weapons[0][6] = Draw_PicFromWad("inv_lightng");

    _sb.weapons[1][0] = Draw_PicFromWad("inv2_shotgun");
    _sb.weapons[1][1] = Draw_PicFromWad("inv2_sshotgun");
    _sb.weapons[1][2] = Draw_PicFromWad("inv2_nailgun");
    _sb.weapons[1][3] = Draw_PicFromWad("inv2_snailgun");
    _sb.weapons[1][4] = Draw_PicFromWad("inv2_rlaunch");
    _sb.weapons[1][5] = Draw_PicFromWad("inv2_srlaunch");
    _sb.weapons[1][6] = Draw_PicFromWad("inv2_lightng");

    for (int i = 0; i < 5; i++) {
        _sb.weapons[2 + i][0] = Draw_PicFromWad(va("inva%i_shotgun", i + 1));
        _sb.weapons[2 + i][1] = Draw_PicFromWad(va("inva%i_sshotgun", i + 1));
        _sb.weapons[2 + i][2] = Draw_PicFromWad(va("inva%i_nailgun", i + 1));
        _sb.weapons[2 + i][3] = Draw_PicFromWad(va("inva%i_snailgun", i + 1));
        _sb.weapons[2 + i][4] = Draw_PicFromWad(va("inva%i_rlaunch", i + 1));
        _sb.weapons[2 + i][5] = Draw_PicFromWad(va("inva%i_srlaunch", i + 1));
        _sb.weapons[2 + i][6] = Draw_PicFromWad(va("inva%i_lightng", i + 1));
    }

    _sb.ammo[0] = Draw_PicFromWad("sb_shells");
    _sb.ammo[1] = Draw_PicFromWad("sb_nails");
    _sb.ammo[2] = Draw_PicFromWad("sb_rocket");
    _sb.ammo[3] = Draw_PicFromWad("sb_cells");

    _sb.armor[0] = Draw_PicFromWad("sb_armor1");
    _sb.armor[1] = Draw_PicFromWad("sb_armor2");
    _sb.armor[2] = Draw_PicFromWad("sb_armor3");

    _sb.items[0] = Draw_PicFromWad("sb_key1");
    _sb.items[1] = Draw_PicFromWad("sb_key2");
    _sb.items[2] = Draw_PicFromWad("sb_invis");
    _sb.items[3] = Draw_PicFromWad("sb_invuln");
    _sb.items[4] = Draw_PicFromWad("sb_suit");
    _sb.items[5] = Draw_PicFromWad("sb_quad");

    _sb.sigil[0] = Draw_PicFromWad("sb_sigil1");
    _sb.sigil[1] = Draw_PicFromWad("sb_sigil2");
    _sb.sigil[2] = Draw_PicFromWad("sb_sigil3");
    _sb.sigil[3] = Draw_PicFromWad("sb_sigil4");

    _sb.faces[4][0] = Draw_PicFromWad("face1");
    _sb.faces[4][1] = Draw_PicFromWad("face_p1");
    _sb.faces[3][0] = Draw_PicFromWad("face2");
    _sb.faces[3][1] = Draw_PicFromWad("face_p2");
    _sb.faces[2][0] = Draw_PicFromWad("face3");
    _sb.faces[2][1] = Draw_PicFromWad("face_p3");
    _sb.faces[1][0] = Draw_PicFromWad("face4");
    _sb.faces[1][1] = Draw_PicFromWad("face_p4");
    _sb.faces[0][0] = Draw_PicFromWad("face5");
    _sb.faces[0][1] = Draw_PicFromWad("face_p5");

    _sb.face.invis = Draw_PicFromWad("face_invis");
    _sb.face.invuln = Draw_PicFromWad("face_invul2");
    _sb.face.invis_invuln = Draw_PicFromWad("face_inv2");
    _sb.face.quad = Draw_PicFromWad("face_quad");

    Cmd_AddCommand("+showscores", Sbar_ShowScores);
    Cmd_AddCommand("-showscores", Sbar_DontShowScores);

    _sb.sbar = Draw_PicFromWad("sbar");
    _sb.ibar = Draw_PicFromWad("ibar");
    _sb.scorebar = Draw_PicFromWad("scorebar");

    //MED 01/04/97 added new hipnotic weapons
    if (hipnotic) {
        hsb_weapons[0][0] = Draw_PicFromWad("inv_laser");
        hsb_weapons[0][1] = Draw_PicFromWad("inv_mjolnir");
        hsb_weapons[0][2] = Draw_PicFromWad("inv_gren_prox");
        hsb_weapons[0][3] = Draw_PicFromWad("inv_prox_gren");
        hsb_weapons[0][4] = Draw_PicFromWad("inv_prox");

        hsb_weapons[1][0] = Draw_PicFromWad("inv2_laser");
        hsb_weapons[1][1] = Draw_PicFromWad("inv2_mjolnir");
        hsb_weapons[1][2] = Draw_PicFromWad("inv2_gren_prox");
        hsb_weapons[1][3] = Draw_PicFromWad("inv2_prox_gren");
        hsb_weapons[1][4] = Draw_PicFromWad("inv2_prox");

        for (int i = 0; i < 5; i++) {
            hsb_weapons[2 + i][0] = Draw_PicFromWad(va("inva%i_laser", i + 1));
            hsb_weapons[2 + i][1] = Draw_PicFromWad(va("inva%i_mjolnir", i + 1));
            hsb_weapons[2 + i][2] = Draw_PicFromWad(va("inva%i_gren_prox", i + 1));
            hsb_weapons[2 + i][3] = Draw_PicFromWad(va("inva%i_prox_gren", i + 1));
            hsb_weapons[2 + i][4] = Draw_PicFromWad(va("inva%i_prox", i + 1));
        }

        hsb_items[0] = Draw_PicFromWad("sb_wsuit");
        hsb_items[1] = Draw_PicFromWad("sb_eshld");
    }

    if (rogue) {
        rsb_invbar[0] = Draw_PicFromWad("r_invbar1");
        rsb_invbar[1] = Draw_PicFromWad("r_invbar2");

        rsb_weapons[0] = Draw_PicFromWad("r_lava");
        rsb_weapons[1] = Draw_PicFromWad("r_superlava");
        rsb_weapons[2] = Draw_PicFromWad("r_gren");
        rsb_weapons[3] = Draw_PicFromWad("r_multirock");
        rsb_weapons[4] = Draw_PicFromWad("r_plasma");

        rsb_items[0] = Draw_PicFromWad("r_shield1");
        rsb_items[1] = Draw_PicFromWad("r_agrav1");

        // PGM 01/19/97 - team color border
        rsb_teambord = Draw_PicFromWad("r_teambord");
        // PGM 01/19/97 - team color border

        rsb_ammo[0] = Draw_PicFromWad("r_ammolava");
        rsb_ammo[1] = Draw_PicFromWad("r_ammomulti");
        rsb_ammo[2] = Draw_PicFromWad("r_ammoplasma");
    }
}


//=============================================================================

// drawing routines are relative to the status bar location

/*
    =============
    Sbar_DrawPic
    =============
*/
void Sbar_DrawPic(int x, int y, qPic_p pic) {
    Draw_Pic(
        x +
        ((cl.gametype == GAME_DEATHMATCH) ?
            0 : HALF(vid.scr.width - 320)),
        y + (vid.scr.height - SBAR_HEIGHT),
        pic
    );
}

/*
    =============
    Sbar_DrawTransPic
    =============
*/
void Sbar_DrawTransPic(int x, int y, qPic_p pic) {
    Draw_TransPic(
        x +
        ((cl.gametype == GAME_DEATHMATCH) ?
            0 : HALF(vid.scr.width - 320)),
        y + (vid.scr.height - SBAR_HEIGHT),
        pic
    );
}

/*
    ================
    Sbar_DrawCharacter

    Draws one solid graphics character
    ================
*/
void Sbar_DrawCharacter(int x, int y, int num) {
    Draw_Character(
        x + 4 +
        ((cl.gametype == GAME_DEATHMATCH) ?
            0 : HALF(vid.scr.width - 320)),
        y + (vid.scr.height - SBAR_HEIGHT),
        num
    );
}

/*
    ================
    Sbar_DrawString
    ================
*/
void Sbar_DrawString(int x, int y, cString str) {
    Draw_String(
        x +
        ((cl.gametype == GAME_DEATHMATCH) ?
            0 : HALF(vid.scr.width - 320)),
        y + (vid.scr.height - SBAR_HEIGHT),
        str
    );
}

/*
    =============
    Sbar_itoa
    =============
*/
int Sbar_itoa(int num, cString buf) {
    cString str = buf;
    if (num < 0) {
        *str++ = '-';
        num = -num;
    }

    int pow10;
    for (pow10 = 10; num >= pow10; pow10 *= 10)
        ;

    do {
        pow10 /= 10;
        int dig = num / pow10;
        *str++ = '0' + dig;
        num -= dig * pow10;
    } while (pow10 != 1);

    *str = 0;

    return str - buf;
}


/*
    =============
    Sbar_DrawNum
    =============
*/
void Sbar_DrawNum(int x, int y, int num, int digits, int color) {
    char str[12];
    int l = Sbar_itoa(num, str);
    cString ptr = str;
    if (l > digits)     ptr += (l - digits);
    if (l < digits)     x += (digits - l) * 24;

    while (*ptr) {
        int frame;
        if (*ptr == '-')    frame = STAT_MINUS;
        else                frame = *ptr - '0';

        Sbar_DrawTransPic(x, y, _sb.nums[color][frame]);
        x += 24;
        ptr++;
    }
}

//=============================================================================

static int  _fragsort[MAX_SCOREBOARD];

static char _scoreboardtext[MAX_SCOREBOARD][20];
// static int  _scoreboardtop[MAX_SCOREBOARD]; // TODO: check is it useless?
// static int  _scoreboardbottom[MAX_SCOREBOARD];// TODO: check is it useless?
// int  scoreboardcount[MAX_SCOREBOARD];
static int  _scoreboardlines;

/*
    ===============
    Sbar_SortFrags
    ===============
*/
void Sbar_SortFrags() {
    // sort by frags
    _scoreboardlines = 0;
    for (int i = 0; i < cl.maxclients; i++)
        if (cl.scores[i].name[0]) {
            _fragsort[_scoreboardlines] = i;
            _scoreboardlines++;
        }

    for (int i = 0; i < _scoreboardlines; i++)
        for (int j = 0; j < (_scoreboardlines - 1 - i); j++)
            if (cl.scores[_fragsort[j]].frags < cl.scores[_fragsort[j + 1]].frags) {
                int k = _fragsort[j];
                _fragsort[j] = _fragsort[j + 1];
                _fragsort[j + 1] = k;
            }
}

int Sbar_ColorForMap(int m) {
    return
        (m < 128) ?
        (m + 8) : (m + 8); // &
}

/*
    ===============
    Sbar_UpdateScoreboard
    ===============
*/
void Sbar_UpdateScoreboard() {
    Sbar_SortFrags();

    // draw the text
    memset(_scoreboardtext, 0, sizeof(_scoreboardtext));

    for (int i = 0; i < _scoreboardlines; i++) {
        int k = _fragsort[i];
        ScoreBoard_p s = &cl.scores[k];
        snprintf(&_scoreboardtext[i][1], sizeof(_scoreboardtext[i][1]), "%3i %s", s->frags, s->name);

        // int top = s->colors & 0xf0;
        // int bottom = (s->colors & 15) << 4;
        // _scoreboardtop[i] = Sbar_ColorForMap(top);
        // _scoreboardbottom[i] = Sbar_ColorForMap(bottom);
    }
}



/*
===============
Sbar_SoloScoreboard
===============
*/
void Sbar_SoloScoreboard() {
    char str[80];
    snprintf(str, sizeof(str), "Monsters:%3i /%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
    Sbar_DrawString(8, 4, str);

    snprintf(str, sizeof(str), "Secrets :%3i /%3i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
    Sbar_DrawString(8, 12, str);

    // time
    int minutes = GetClSimTime() / 60;
    int seconds = GetClSimTime() - 60 * minutes;
    int tens = seconds / 10;
    int units = seconds - 10 * tens;
    snprintf(str, sizeof(str), "Time :%3i:%i%i", minutes, tens, units);
    Sbar_DrawString(184, 4, str);

    // draw level name
    Sbar_DrawString(232 - strlen(cl.levelname) * 4, 12, cl.levelname);
}

/*
===============
Sbar_DrawScoreboard
===============
*/
void Sbar_DrawScoreboard() {
    Sbar_SoloScoreboard();
    if (cl.gametype == GAME_DEATHMATCH)
        Sbar_DeathmatchOverlay();
#if 0

    if (cl.gametype != GAME_DEATHMATCH) {
        Sbar_SoloScoreboard();
        return;
    }

    Sbar_UpdateScoreboard();

    int l = (_scoreboardlines <= 6) ? _scoreboardlines : 6;

    for (int i = 0; i < l; i++) {
        int x = 20 * (i & 1);
        int y = i / 2 * 8;

        ScoreBoard_p s = &cl.scores[_fragsort[i]];
        if (!s->name[0])
            continue;

        // draw background
        int top = s->colors & 0xf0;
        int bottom = (s->colors & 15) << 4;
        top = Sbar_ColorForMap(top);
        bottom = Sbar_ColorForMap(bottom);

        Draw_Fill(x * 8 + 10 + HALF(vid.scr.width - 320), y + vid.scr.height - SBAR_HEIGHT, 28, 4, top);
        Draw_Fill(x * 8 + 10 + HALF(vid.scr.width - 320), y + 4 + vid.scr.height - SBAR_HEIGHT, 28, 4, bottom);

        // draw text
        for (int j = 0; j < 20; j++) {
            int c = _scoreboardtext[i][j];
            if ((c == 0) ||
                (c == ' '))
                continue;
            Sbar_DrawCharacter((x + j) * 8, y, c);
        }
    }
#endif
}

//=============================================================================

/*
===============
Sbar_DrawInventory
===============
*/
void Sbar_DrawInventory() {
    if (rogue) {
        if (cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN)
            Sbar_DrawPic(0, -24, rsb_invbar[0]);
        else
            Sbar_DrawPic(0, -24, rsb_invbar[1]);
    }
    else    Sbar_DrawPic(0, -24, _sb.ibar);


    // weapons
    for (int i = 0; i < 7; i++) {
        if (cl.items & (IT_SHOTGUN << i)) {
            LegDt_t time = cl.item_gettime[i];
            int flashon = (int)((GetClSimTime() - time) * 10);
            flashon = (flashon >= 10) ?
                (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN << i)) : (flashon % 5) + 2;

            Sbar_DrawPic(i * 24, -16, _sb.weapons[flashon][i]);

            if (flashon > 1)
                _sb.updates = 0;  // force update to remove flash
        }
    }

    // MED 01/04/97
    // hipnotic weapons
    if (hipnotic) {
        int grenadeflashing = 0;
        for (int i = 0; i < 4; i++) {
            if (cl.items & (1 << hipweapons[i])) {
                LegDt_t time = cl.item_gettime[hipweapons[i]];
                int flashon = (int)((GetClSimTime() - time) * 10);
                if (flashon >= 10) {
                    if (cl.stats[STAT_ACTIVEWEAPON] == (1 << hipweapons[i]))
                        flashon = 1;
                    else
                        flashon = 0;
                }
                else
                    flashon = (flashon % 5) + 2;

                // check grenade launcher
                if (i == 2) {
                    if ((cl.items & HIT_PROXIMITY_GUN) &&
                        (flashon)
                        ) {
                        grenadeflashing = 1;
                        Sbar_DrawPic(96, -16, hsb_weapons[flashon][2]);
                    }
                }
                else if (i == 3) {
                    if (cl.items & (IT_SHOTGUN << 4)) {
                        if (flashon && !grenadeflashing)    Sbar_DrawPic(96, -16, hsb_weapons[flashon][3]);
                        else if (!grenadeflashing)          Sbar_DrawPic(96, -16, hsb_weapons[0][3]);
                    }
                    else                                    Sbar_DrawPic(96, -16, hsb_weapons[flashon][4]);
                }
                else    Sbar_DrawPic(176 + (i * 24), -16, hsb_weapons[flashon][i]);
                if (flashon > 1)
                    _sb.updates = 0;      // force update to remove flash
            }
        }
    }

    // check for powered up weapon.
    if ((rogue) &&
        (cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN))
        for (int i = 0; i < 5; i++)
            if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
                Sbar_DrawPic((i + 2) * 24, -16, rsb_weapons[i]);

    // ammo counts
    for (int i = 0; i < 4; i++) {
        char num[6];
        snprintf(num, sizeof(num), "%3i", cl.stats[STAT_SHELLS + i]);
        if (num[0] != ' ')  Sbar_DrawCharacter((6 * i + 1) * 8 - 2, -24, 18 + num[0] - '0');
        if (num[1] != ' ')  Sbar_DrawCharacter((6 * i + 2) * 8 - 2, -24, 18 + num[1] - '0');
        if (num[2] != ' ')  Sbar_DrawCharacter((6 * i + 3) * 8 - 2, -24, 18 + num[2] - '0');
    }

    int flashon = 0;
    // items
    for (int i = 0; i < 6; i++)
        if (cl.items & (1 << (17 + i))) {
            LegDt_t time = cl.item_gettime[17 + i];
            if (time && (time > (GetClSimTime() - 2)) && flashon)   // flash frame
                _sb.updates = 0;
            else
                //MED 01/04/97 changed keys
                if (!hipnotic || (i > 1))   Sbar_DrawPic(192 + MUL16(i), -16, _sb.items[i]);


            if (time && (time > (GetClSimTime() - 2)))
                _sb.updates = 0;
        }
    //MED 01/04/97 added hipnotic items
    // hipnotic items
    if (hipnotic) {
        for (int i = 0; i < 2; i++)
            if (cl.items & (1 << (24 + i))) {
                LegDt_t time = cl.item_gettime[24 + i];
                if (time && (time > (GetClSimTime() - 2)) && flashon)   // flash frame
                    _sb.updates = 0;
                else    Sbar_DrawPic(288 + MUL16(i), -16, hsb_items[i]);

                if (time && (time > (GetClSimTime() - 2)))
                    _sb.updates = 0;
            }
    }

    if (rogue) {
        // new rogue items
        for (int i = 0; i < 2; i++) {
            if (cl.items & (1 << (29 + i))) {
                LegDt_t time = cl.item_gettime[29 + i];
                if (time && (time > (GetClSimTime() - 2)) && flashon)  // flash frame
                    _sb.updates = 0;
                else    Sbar_DrawPic(288 + MUL16(i), -16, rsb_items[i]);

                if (time && (time > (GetClSimTime() - 2)))
                    _sb.updates = 0;
            }
        }
    }
    else {
        // sigils
        for (int i = 0; i < 4; i++) {
            if (cl.items & (1 << (28 + i))) {
                LegDt_t time = cl.item_gettime[28 + i];
                if (time && (time > (GetClSimTime() - 2)) && flashon)  // flash frame
                    _sb.updates = 0;
                else    Sbar_DrawPic(320 - 32 + i * 8, -16, _sb.sigil[i]);

                if (time && (time > (GetClSimTime() - 2)))
                    _sb.updates = 0;
            }
        }
    }
}

//=============================================================================

/*
===============
Sbar_DrawFrags
===============
*/
void Sbar_DrawFrags() {
    Sbar_SortFrags();

    // draw the text
    int l = (_scoreboardlines <= 4) ? _scoreboardlines : 4;

    int x = 23;
    int xofs = (cl.gametype == GAME_DEATHMATCH) ?
        0 : HALF(vid.scr.width - 320);
    int y = vid.scr.height - SBAR_HEIGHT - 23;

    for (int i = 0; i < l; i++) {
        int k = _fragsort[i];
        ScoreBoard_p s = &cl.scores[k];
        if (!s->name[0])
            continue;

        // draw background
        int top = s->colors & 0xF0;
        int bottom = (s->colors & 15) << 4;
        top = Sbar_ColorForMap(top);
        bottom = Sbar_ColorForMap(bottom);

        Draw_Fill(xofs + x * 8 + 10, y, 28, 4, top);
        Draw_Fill(xofs + x * 8 + 10, y + 4, 28, 3, bottom);

        // draw number
        char num[12];
        snprintf(num, sizeof(num), "%3i", s->frags);

        Sbar_DrawCharacter((x + 1) * 8, -24, num[0]);
        Sbar_DrawCharacter((x + 2) * 8, -24, num[1]);
        Sbar_DrawCharacter((x + 3) * 8, -24, num[2]);

        if (k == cl.viewentity - 1) {
            Sbar_DrawCharacter(x * 8 + 2, -24, 16);
            Sbar_DrawCharacter((x + 4) * 8 - 4, -24, 17);
        }
        x += 4;
    }
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace() {

    // PGM 01/19/97 - team color drawing
    // PGM 03/02/97 - fixed so color swatch only appears in CTF modes
    if (rogue &&
        (cl.maxclients != 1) &&
        (teamplay.value > 3) &&
        (teamplay.value < 7)) {

        ScoreBoard_p s = &cl.scores[cl.viewentity - 1];
        // draw background
        int top = s->colors & 0xf0;
        int bottom = (s->colors & 15) << 4;
        top = Sbar_ColorForMap(top);
        bottom = Sbar_ColorForMap(bottom);

        int xofs = (cl.gametype == GAME_DEATHMATCH) ?
            113 : HALF(vid.scr.width - 320) + 113;

        Sbar_DrawPic(112, 0, rsb_teambord);
        Draw_Fill(xofs, vid.scr.height - SBAR_HEIGHT + 3, 22, 9, top);
        Draw_Fill(xofs, vid.scr.height - SBAR_HEIGHT + 12, 22, 9, bottom);

        // draw number
        char num[12];
        snprintf(num, sizeof(num), "%3i", s->frags);

        if (top == 8) {
            if (num[0] != ' ')  Sbar_DrawCharacter(109, 3, 18 + num[0] - '0');
            if (num[1] != ' ')  Sbar_DrawCharacter(116, 3, 18 + num[1] - '0');
            if (num[2] != ' ')  Sbar_DrawCharacter(123, 3, 18 + num[2] - '0');
        }
        else {
            Sbar_DrawCharacter(109, 3, num[0]);
            Sbar_DrawCharacter(116, 3, num[1]);
            Sbar_DrawCharacter(123, 3, num[2]);
        }

        return;
    }
    // PGM 01/19/97 - team color drawing

    if ((cl.items & (IT_INVISIBILITY | IT_INVULNERABILITY))
        == (IT_INVISIBILITY | IT_INVULNERABILITY)) {
        Sbar_DrawPic(112, 0, _sb.face.invis_invuln);
        return;
    }
    if (cl.items & IT_QUAD) { ;             Sbar_DrawPic(112, 0, _sb.face.quad);     return; }
    if (cl.items & IT_INVISIBILITY) { ;     Sbar_DrawPic(112, 0, _sb.face.invis);    return; }
    if (cl.items & IT_INVULNERABILITY) { ;  Sbar_DrawPic(112, 0, _sb.face.invuln);   return; }

    int f = (cl.stats[STAT_HEALTH] >= 100) ?
        4 : cl.stats[STAT_HEALTH] / 20;

    int anim;
    if (GetClSimTime() <= cl.faceanimtime) {
        anim = 1;
        _sb.updates = 0;  // make sure the anim gets drawn over
    }
    else
        anim = 0;
    Sbar_DrawPic(112, 0, _sb.faces[f][anim]);
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw() {
    if ((scr.con_current == vid.scr.height) || // console is full screen
        (_sb.updates >= vid.numpages))
        return;

    scr.copyeverything = true;

    _sb.updates++;

    if (sb_lines &&
        (vid.scr.width > 320)
        )
        Draw_TileClear(0, vid.scr.height - sb_lines, vid.scr.width, sb_lines);

    if (sb_lines > 24) {
        Sbar_DrawInventory();
        if (cl.maxclients != 1)
            Sbar_DrawFrags();
    }

    if (_sb.showscores ||
        (cl.stats[STAT_HEALTH] <= 0)) {
        Sbar_DrawPic(0, 0, _sb.scorebar);
        Sbar_DrawScoreboard();
        _sb.updates = 0;
    }
    else if (sb_lines) {
        Sbar_DrawPic(0, 0, _sb.sbar);

        // keys (hipnotic only)
        //MED 01/04/97 moved keys here so they would not be overwritten
        if (hipnotic) {
            if (cl.items & IT_KEY1) Sbar_DrawPic(209, 3, _sb.items[0]);
            if (cl.items & IT_KEY2) Sbar_DrawPic(209, 12, _sb.items[1]);
        }
        // armor
        if (cl.items & IT_INVULNERABILITY) {
            Sbar_DrawNum(24, 0, 666, 3, 1);
            Sbar_DrawPic(0, 0, draw_disc);
        }
        else {
            if (rogue) {
                Sbar_DrawNum(
                    24, 0, cl.stats[STAT_ARMOR], 3,
                    cl.stats[STAT_ARMOR] <= 25
                );
                /**/ if (cl.items & RIT_ARMOR3) Sbar_DrawPic(0, 0, _sb.armor[2]);
                else if (cl.items & RIT_ARMOR2) Sbar_DrawPic(0, 0, _sb.armor[1]);
                else if (cl.items & RIT_ARMOR1) Sbar_DrawPic(0, 0, _sb.armor[0]);
            }
            else {
                Sbar_DrawNum(
                    24, 0, cl.stats[STAT_ARMOR], 3,
                    cl.stats[STAT_ARMOR] <= 25
                );
                /**/ if (cl.items & IT_ARMOR3)  Sbar_DrawPic(0, 0, _sb.armor[2]);
                else if (cl.items & IT_ARMOR2)  Sbar_DrawPic(0, 0, _sb.armor[1]);
                else if (cl.items & IT_ARMOR1)  Sbar_DrawPic(0, 0, _sb.armor[0]);
            }
        }

        // face
        Sbar_DrawFace();

        // health
        Sbar_DrawNum(136, 0, cl.stats[STAT_HEALTH], 3
            , cl.stats[STAT_HEALTH] <= 25);

        // ammo icon
        if (rogue) {
            /**/ if (cl.items & RIT_SHELLS)         Sbar_DrawPic(224, 0, _sb.ammo[0]);
            else if (cl.items & RIT_NAILS)          Sbar_DrawPic(224, 0, _sb.ammo[1]);
            else if (cl.items & RIT_ROCKETS)        Sbar_DrawPic(224, 0, _sb.ammo[2]);
            else if (cl.items & RIT_CELLS)          Sbar_DrawPic(224, 0, _sb.ammo[3]);
            else if (cl.items & RIT_LAVA_NAILS)     Sbar_DrawPic(224, 0, rsb_ammo[0]);
            else if (cl.items & RIT_PLASMA_AMMO)    Sbar_DrawPic(224, 0, rsb_ammo[1]);
            else if (cl.items & RIT_MULTI_ROCKETS)  Sbar_DrawPic(224, 0, rsb_ammo[2]);
        }
        else {
            /**/ if (cl.items & IT_SHELLS)  Sbar_DrawPic(224, 0, _sb.ammo[0]);
            else if (cl.items & IT_NAILS)   Sbar_DrawPic(224, 0, _sb.ammo[1]);
            else if (cl.items & IT_ROCKETS) Sbar_DrawPic(224, 0, _sb.ammo[2]);
            else if (cl.items & IT_CELLS)   Sbar_DrawPic(224, 0, _sb.ammo[3]);
        }

        Sbar_DrawNum(248, 0, cl.stats[STAT_AMMO], 3,
            cl.stats[STAT_AMMO] <= 10);
    }

    if ((vid.scr.width > 320) &&
        (cl.gametype == GAME_DEATHMATCH))
        Sbar_MiniDeathmatchOverlay();
}

//=============================================================================

/*
==================
Sbar_IntermissionNumber

==================
*/
void Sbar_IntermissionNumber(int x, int y, int num, int digits, int color) {
    char str[12];
    int l = Sbar_itoa(num, str);
    cString ptr = str;
    /**/ if (l > digits)    ptr += (l - digits);
    else if (l < digits)    x += (digits - l) * 24;

    while (*ptr) {
        int frame = (*ptr == '-') ?
            STAT_MINUS : *ptr - '0';

        Draw_TransPic(x, y, _sb.nums[color][frame]);
        x += 24;
        ptr++;
    }
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
int M_DrawPicHC(int y, qPic_p pic);

void Sbar_DeathmatchOverlay() {
    scr.copyeverything = true;
    SCR_RequestRedraw();

    M_DrawPicHC(8, Draw_CachePic("gfx/ranking.lmp"));

    // scores
    Sbar_SortFrags();

    // draw the text
    int l = _scoreboardlines;

    int x = 80 + HALF(vid.scr.width - 320);
    int y = 40;
    for (int i = 0; i < l; i++) {
        int k = _fragsort[i];
        ScoreBoard_p s = &cl.scores[k];
        if (!s->name[0])
            continue;

        // draw background
        int top = s->colors & 0xf0;
        int bottom = (s->colors & 15) << 4;
        top = Sbar_ColorForMap(top);
        bottom = Sbar_ColorForMap(bottom);

        Draw_Fill(x, y, 40, 4, top);
        Draw_Fill(x, y + 4, 40, 4, bottom);

        // draw number
        int f = s->frags;
        char num[12];
        snprintf(num, sizeof(num), "%3i", f);

        Draw_Character(x + 8, y, num[0]);
        Draw_Character(x + 16, y, num[1]);
        Draw_Character(x + 24, y, num[2]);

        if (k == cl.viewentity - 1)
            Draw_Character(x - 8, y, 12);

#if 0
        {

            // draw time
            int total = cl.completed_time - s->entertime;
            int minutes = (int)total / 60;
            int n = total - minutes * 60;
            int tens = n / 10;
            int units = n % 10;

            snprintf(num, sizeof(str), "%3i:%i%i", minutes, tens, units);

            Draw_String(x + 48, y, num);
        }
#endif

        // draw name
        Draw_String(x + 64, y, s->name);

        y += 10;
    }
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
void Sbar_MiniDeathmatchOverlay() {
    if ((vid.scr.width < 512) ||
        (!sb_lines)
        ) {
        return;
    }

    scr.copyeverything = true;
    SCR_RequestRedraw();

    // scores
    Sbar_SortFrags();

    // draw the text
    int sbl = _scoreboardlines;
    int y = vid.scr.height - sb_lines;
    int numlines = EIGHTH(sb_lines);
    if (numlines < 3)
        return;


    //find us
    int i = 0;
    for (; i < sbl; i++)
        if (_fragsort[i] == cl.viewentity - 1)
            break;

    i = (i == sbl) ?
        0 :                 // we're not there
        i - HALF(numlines); // figure out start

    if (i > sbl - numlines)     i = sbl - numlines;
    else if (i < 0)                  i = 0;


    int x = 324;
    for (; (i < sbl) && (y < (vid.scr.height - 8)); i++) {
        int k = _fragsort[i];
        ScoreBoard_p s = &cl.scores[k];
        if (s->name[0]) {
            // draw background
            int top = s->colors & 0xf0;
            int bottom = (s->colors & 15) << 4;
            top = Sbar_ColorForMap(top);
            bottom = Sbar_ColorForMap(bottom);

            Draw_Fill(x, y + 1, 40, 3, top);
            Draw_Fill(x, y + 4, 40, 4, bottom);

            // draw number
            char num[12];
            snprintf(num, sizeof(num), "%3i", s->frags);

            Draw_Character(x + 8, y, num[0]);
            Draw_Character(x + 16, y, num[1]);
            Draw_Character(x + 24, y, num[2]);

            if (k == cl.viewentity - 1) {
                Draw_Character(x, y, 16);
                Draw_Character(x + 32, y, 17);
            }

#if 0
            {
                // draw time
                int total = cl.completed_time - s->entertime;
                int minutes = (int)total / 60;
                int n = total - minutes * 60;
                int tens = n / 10;
                int units = n % 10;

                snprintf(num, sizeof(str), "%3i:%i%i", minutes, tens, units);

                Draw_String(x + 48, y, num);
            }
#endif

            // draw name
            Draw_String(x + 48, y, s->name);

            y += 8;
        }
    }
}

/*
==================
Sbar_IntermissionOverlay

==================
*/
void Sbar_IntermissionOverlay() {
    scr.copyeverything = true;
    SCR_RequestRedraw();

    if (cl.gametype == GAME_DEATHMATCH) {
        Sbar_DeathmatchOverlay();
        return;
    }

    Draw_Pic(64, 24, Draw_CachePic("gfx/complete.lmp"));
    Draw_TransPic(0, 56, Draw_CachePic("gfx/inter.lmp"));

    // time
    int dig = cl.completed_time / 60;
    Sbar_IntermissionNumber(160, 64, dig, 3, 0);
    int num = cl.completed_time - dig * 60;
    Draw_TransPic(234, 64, _sb.colon);
    Draw_TransPic(246, 64, _sb.nums[0][num / 10]);
    Draw_TransPic(266, 64, _sb.nums[0][num % 10]);

    Sbar_IntermissionNumber(160, 104, cl.stats[STAT_SECRETS], 3, 0);
    Draw_TransPic(232, 104, _sb.slash);
    Sbar_IntermissionNumber(240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0);

    Sbar_IntermissionNumber(160, 144, cl.stats[STAT_MONSTERS], 3, 0);
    Draw_TransPic(232, 144, _sb.slash);
    Sbar_IntermissionNumber(240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0);

}


/*
==================
Sbar_FinaleOverlay

==================
*/
void Sbar_FinaleOverlay() {
    scr.copyeverything = true;
    qPic_p pic = Draw_CachePic("gfx/finale.lmp");
    Draw_TransPic(HALF(vid.scr.width - pic->width), 16, pic);
}
