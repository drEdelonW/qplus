engine/                       // root of the engine codebase
├─ host/                      // public façade; owns Client/Server/Net/VID
│  └─ vid/                    // window/surface layer (host-managed)
│     ├─ vid.h                // C API: init/set_mode/get_handle/begin/end/pump
│     ├─ vid_x11.c            // X11 backend (native_display/window)
│     ├─ vid_sdl.c            // SDL backend (optional)
│     └─ vid_null.c           // headless stub (tests/dedicated)
├─ shared/                    // common libs (no deps on client/server/net)
│  ├─ fs/                     // VFS: pak/wad/io helpers
│  ├─ math/                   // vec3/angles/bbox/random
│  ├─ model/                  // BSP/MDL/Sprite structs + neutral loaders
│  ├─ common/zone/console/    // allocators, small utils, logging/console
│  └─ net_wire_ids.h          // wire protocol ids/version/limits (single source of truth)
├─ net/                       // transport/runtime networking (host controls)
│  ├─ msg.h/.c                // sizebuf/bitbuf, MSG_Write/Read, packing
│  ├─ netchan/                // reliable channel, seq/ack, frag/defrag
│  └─ drivers/                // UDP/loopback drivers (sockets, poll)
├─ server/                    // private to Host (authoritative world/logic)
│  ├─ sv_main/                // lifecycle, map load/unload, tick loop
│  ├─ sv_client/              // client slots, connect/spawn, usercmd validate
│  ├─ sv_world/               // server BSP view, entity linking, PVS for send
│  ├─ world_core/             // BSP core: hulls/trace/contents/FatPVS (server-only)
│  ├─ sv_phys/                // authoritative physics: move/step/slide
│  ├─ sv_trace/               // wrappers for traces/hull selection, helpers
│  ├─ sv_send/                // snapshot build, delta, multicast per PVS
│  ├─ qcvm/                   // QuakeC VM: pr_exec/edict/cmds/load
│  └─ proto/                  // server-side codecs only
│     ├─ svc_write.h/.c       // encode SVC_* (server → client)
│     └─ clc_read.h/.c        // decode CLC_* (client → server)
└─ client/                    // private to Host (view/audio/input; no world logic)
   ├─ cl_main/                // init/connect/disconnect, resources, map flow
   ├─ cl_parse/               // decode SVC_*, update visible entity state
   ├─ input/                  // keys/binds, build usercmd (to CLC_*)
   ├─ assets/                 // visual-only map/model assets (no hull/trace)
   │  ├─ cl_bspgeo.c          // VERTEX/EDGE/SURFEDGE/FACE/TEXINFO import
   │  ├─ cl_light.c           // LIGHTING (lightmaps) handling
   │  └─ cl_tex.c             // MIPTEX load/expand, texture atlas prep
V  ├─ renderer/               // render API + backends (hot-swap via manager)
   │  ├─ render_api.h         // C vtable: init/set_mode/begin/draw_*/end
   │  ├─ render_mgr.c         // current backend switcher (SW↔GL)
   │  ├─ view/                // camera/frustum, build render commands
   │  ├─ draw2d/              // 2D/HUD/console blits
   │  ├─ r_sw/                // software rasterizer (WinQuake-style)
   │  └─ r_gl/                // hardware renderer (GL/VK/Metal one chosen)
   ├─ video/                  // (reserved/legacy) do not use; use host/vid/*
V  ├─ sound/                  // mixer and device backends (client-side)
   │  ├─ sound_api.h          // C vtable: init/mix/set_device
   │  ├─ snd_mixer.c          // mixing, spatialization
   │  ├─ sfx_cache.c          // decode/precache samples
   │  └─ backends/            // ALSA/CoreAudio/STM32-I2S, etc.
V  ├─ ui/                     // HUD/menus/console rendering helpers
   └─ proto/                  // client-side codecs only
      ├─ svc_read.h/.c        // decode SVC_* (server → client)
      └─ clc_write.h/.c       // encode CLC_* (client → server)