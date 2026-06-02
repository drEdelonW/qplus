#pragma once
/*
 * Quake Resource System - Asset Specification
 * * This engine utilizes a domain-specific asset hierarchy stored in PAK files.
 * Each asset type is handled by a specialized parser within the engine:
 * * 1. MAP Domain (Spatial/World)
 * - .bsp: Binary Space Partitioning maps (level geometry, lighting, entities).
 * - .wad: WAD container for textures/surfaces used within BSP maps.
 * * 2. MODEL Domain (Animated/Physical Entities)
 * - .mdl: 3D models with skeletal/vertex animation for players, NPCs, weapons.
 * - .spr: 2D sprite data for particles, effects, and billboards.
 * * 3. LOGIC/SCRIPT Domain (State/Behavior)
 * - .dat: Compiled QuakeC bytecode for game logic and entity behavior.
 * - .cfg: Text-based configuration files for engine settings.
 * - .rc : Execution scripts (e.g., quake.rc) for initialization sequences.
 * * 4. MEDIA Domain (Visuals/Audio)
 * - .lmp: Raw bitmap data for HUD, menus, and UI overlays.
 * - .wav: Standard waveform audio for sound effects.
 * - .dem: Demo recording stream of network commands for session playback.
 * - .bin: Raw binary data buffers (e.g., cutscene/text sequences).
 */