#pragma once
#include "server.h"
#include "gamedefs.h" // item_bits_t

edict_p SvPlayer(); // sv_user.c

static inline bool SvPlayer_IsDead() { return SvPlayer()->v.health <= 0.f; }
static inline void SvPlayer_SetHealth(int heal) { SvPlayer()->v.health = (float)heal; }

// static inline bool SvPlayer_IsDead() { return SvPlayer()->v.armorvalue <= 0.f; }
static inline void SvPlayer_SetArmor(int armor) { SvPlayer()->v.armorvalue = (float)armor; }

static inline item_bits_t SvPlayer_Items() { return (item_bits_t)(SvPlayer()->v.items); }
static inline void SvPlayer_AddItems(item_bits_t item) { SvPlayer()->v.items = (item_bits_t)SvPlayer()->v.items | item; }

static inline qVmString_t SvPlayer_NetName() { return SvPlayer()->v.netname; }

static inline EntityFlags_t SvPlayer_Flags() { return (EntityFlags_t)SvPlayer()->v.flags; }
static inline bool SvPlayer_IsFlag(EntityFlags_t flag) { return (SvPlayer_Flags() & flag) != 0; }
static inline void SvPlayer_SetFlag(EntityFlags_t flag)   { SvPlayer()->v.flags = (EntityFlags_t)(SvPlayer_Flags() | flag); }
static inline void SvPlayer_ClrFlag(EntityFlags_t flag)   { SvPlayer()->v.flags = (EntityFlags_t)(SvPlayer_Flags() & ~flag); }
static inline void SvPlayer_ToggleFlag(EntityFlags_t flag){ SvPlayer()->v.flags = (EntityFlags_t)(SvPlayer_Flags() ^ flag); }

// ==== ammo: shells ====
static inline int SvPlayer_AmmoShells() { return SvPlayer()->v.ammo_shells; }
static inline void  SvPlayer_SetAmmoShells(int val) { SvPlayer()->v.ammo_shells = (float)val; }

// ==== weapon / rogue field helper (used by all Give* above) ====
static inline item_bits_t  SvPlayer_Weapon() { return (item_bits_t)SvPlayer()->v.weapon; }
static inline bool SvPlayer_WeaponAtMostLightning() { return SvPlayer_Weapon() <= IT_LIGHTNING; }

// ==== ammo: nails / lava_nails ====
static inline int SvPlayer_AmmoNails() { return SvPlayer()->v.ammo_nails; }
static inline void  SvPlayer_SetAmmoNails(int val) { SvPlayer()->v.ammo_nails = (float)val; }

// ==== ammo: rockets / multi_rockets ====
static inline int SvPlayer_AmmoRockets() { return SvPlayer()->v.ammo_rockets; }
static inline void SvPlayer_SetAmmoRockets(int val) { SvPlayer()->v.ammo_rockets = (float)val; }

// ==== ammo: cells / plasma ====
static inline int SvPlayer_AmmoCells() { return SvPlayer()->v.ammo_cells; }
static inline void SvPlayer_SetAmmoCells(int val) { SvPlayer()->v.ammo_cells = (float)val; }

// --- angles / v_angle / punchangle ---
static inline ang3_t SvPlayer_Angles() { return SvPlayer()->v.angles; }
static inline ang3_p SvPlayer_pAngles() { return &SvPlayer()->v.angles; }
static inline void SvPlayer_SetAngles(ang3_t a) { SvPlayer()->v.angles = a; }

static inline ang3_t SvPlayer_ViewAngle() { return SvPlayer()->v.v_angle; }
static inline void SvPlayer_SetViewAngle(ang3_t a) { SvPlayer()->v.v_angle = a; }

static inline ang3_t SvPlayer_PunchAngle() { return SvPlayer()->v.punchangle; }
static inline void SvPlayer_SetPunchAngle(ang3_t a) { SvPlayer()->v.punchangle = a; }

// --- idealpitch ---
static inline Angle_t SvPlayer_IdealPitch() { return SvPlayer()->v.idealpitch; }
static inline void SvPlayer_SetIdealPitch(Angle_t p) { SvPlayer()->v.idealpitch = p; }

// // --- fixangle ---
static inline bool SvPlayer_FixAngle() { return SvPlayer()->v.fixangle != 0.f; }
// static inline void SvPlayer_SetFixAngle(bool on) { SvPlayer()->v.fixangle = on ? 1.f : 0.f; }

// --- movetype ---
static inline movetype_t SvPlayer_MoveType() { return (movetype_t)SvPlayer()->v.movetype; }
static inline void SvPlayer_SetMoveType(movetype_t mt) { SvPlayer()->v.movetype = (movetype_t)mt; }
static inline bool SvPlayer_IsMoveType(movetype_t mt) { return SvPlayer_MoveType() == mt; }

// --- origin ---
static inline vec3_t SvPlayer_Origin() { return SvPlayer()->v.origin; }
static inline vec3_p SvPlayer_pOrigin() { return &SvPlayer()->v.origin; }
static inline void SvPlayer_SetOrigin(vec3_t o) { SvPlayer()->v.origin = o; }

// --- view offset ---
static inline vec3_t SvPlayer_ViewOffs() { return SvPlayer()->v.view_ofs; }

// --- teleport_time ---
static inline SimTime_t SvPlayer_TeleportTime() { return (SimTime_t)SvPlayer()->v.teleport_time; }
static inline void SvPlayer_SetTeleportTime(SimTime_t t) { SvPlayer()->v.teleport_time = (float)t; }
static inline bool SvPlayer_TeleportTimeElapsed() { return SV_GetTime() > SvPlayer_TeleportTime(); }

// --- waterlevel ---
static inline WaterLevel_t SvPlayer_WaterLevel() { return (WaterLevel_t)SvPlayer()->v.waterlevel; }
// static inline void SvPlayer_SetWaterLevel(WaterLevel_t lvl) { SvPlayer()->v.waterlevel = (float)lvl; }
// static inline bool SvPlayer_IsSubmergedAtLeast(WaterLevel_t lvl) { return SvPlayer_WaterLevel() >= lvl; }

// --- velocity ---
static inline vec3_t SvPlayer_Velocity() { return SvPlayer()->v.velocity; }
static inline vec3_p SvPlayer_pVelocity() { return &SvPlayer()->v.velocity; }
// static inline void SvPlayer_SetVelocity(vec3_t v) { SvPlayer()->v.velocity = v; }

// --- movedir ---
static inline vec3_t SvPlayer_MoveDir() { return SvPlayer()->v.movedir; }
// static inline void SvPlayer_SetMoveDir(vec3_t d) { SvPlayer()->v.movedir = d; }

// --- mins (bbox) ---
static inline vec3_t SvPlayer_Mins() { return SvPlayer()->v.mins; }
// static inline void SvPlayer_SetMins(vec3_t m) { SvPlayer()->v.mins = m; }

#if 0
// ==== movetype ====
static inline movetype_t SvPlayer_MoveType() { return (movetype_t)SvPlayer()->v.movetype; }
static inline void SvPlayer_SetMoveType(movetype_t mt) { SvPlayer()->v.movetype = (movetype_t)mt; }
static inline bool SvPlayer_IsMoveType(movetype_t mt) { return SvPlayer_MoveType() == mt; }

// ==== cheat toggles (built on SvPlayer_ToggleFlag from before) ====
static inline bool SvPlayer_ToggleGodmode()  { SvPlayer_ToggleFlag(FL_GODMODE);  return SvPlayer_IsFlag(FL_GODMODE); }
static inline bool SvPlayer_ToggleNotarget() { SvPlayer_ToggleFlag(FL_NOTARGET); return SvPlayer_IsFlag(FL_NOTARGET); }
#endif