// Minimal SDL2 wireframe 3D cube (C, macOS-friendly)
// No OpenGL; simple perspective + line drawing.
// Build: clang -O2 -g sdl3d_demo.c -o sdl3d_demo $(pkg-config --cflags --libs sdl2)

#if __has_include(<SDL2/SDL.h>)
#  include <SDL2/SDL.h>
#else
#  include <SDL.h>
#endif
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float x, y, z;
} Vec3;

// Simple 3D rotation around X and Y
static Vec3 rotate_xy(Vec3 v, float ax, float ay) {
    // rotate around Y
    float cY = cosf(ay), sY = sinf(ay);
    float x1 =  v.x * cY + v.z * sY;
    float z1 = -v.x * sY + v.z * cY;

    // rotate around X
    float cX = cosf(ax), sX = sinf(ax);
    float y2 =  v.y * cX - z1 * sX;
    float z2 =  v.y * sX + z1 * cX;

    Vec3 r = { x1, y2, z2 };
    return r;
}

// Perspective projection (very simple)
static void project_point(
    const Vec3 p,
    int w, int h,
    float fov_scale,
    int* sx, int* sy
) {
    // shift forward to keep z > 0
    float z = p.z + 4.0f;
    if (z < 0.1f) z = 0.1f;

    float px = (p.x / z) * fov_scale;
    float py = (p.y / z) * fov_scale;

    // screen space: origin top-left, y grows down
    *sx = (int)(w * 0.5f + px);
    *sy = (int)(h * 0.5f - py);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    // const int win_w = 960;
    // const int win_h = 720;
    const int win_w = 320;
    const int win_h = 240;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    // Prefer accelerated renderer if available
    SDL_Window* win = SDL_CreateWindow(
        "SDL2 Wireframe Cube",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_SHOWN
    );
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        // fallback to software renderer
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // Cube vertices in object space
    const float s = 1.0f;
    Vec3 verts[8] = {
        {-s, -s, -s}, { s, -s, -s}, { s,  s, -s}, {-s,  s, -s},
        {-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}
    };

    // 12 edges of the cube (pairs of vertex indices)
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // back face
        {4,5},{5,6},{6,7},{7,4}, // front face
        {0,4},{1,5},{2,6},{3,7}  // side connections
    };

    // Faces (for optional backface culling of edges)
    const int faces[6][4] = {
        {0,1,2,3}, {4,5,6,7}, {0,1,5,4},
        {2,3,7,6}, {1,2,6,5}, {0,3,7,4}
    };

    float angle_x = 0.0f;
    float angle_y = 0.0f;

    // FOV scale in pixels
    float fov_scale = win_h * 0.9f; // tuned for a nice size

    int running = 1;
    uint64_t prev = SDL_GetPerformanceCounter();
    const uint64_t freq = SDL_GetPerformanceFrequency();

    while (running) {
        // events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }

        // dt
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = (float)((double)(now - prev) / (double)freq);
        prev = now;

        // animate
        angle_x += 0.7f * dt;
        angle_y += 1.1f * dt;

        // transform vertices
        Vec3 tv[8];
        for (int i = 0; i < 8; ++i) {
            tv[i] = rotate_xy(verts[i], angle_x, angle_y);
        }

        // precompute projected screen coords
        int sx[8], sy[8];
        for (int i = 0; i < 8; ++i) {
            project_point(tv[i], win_w, win_h, fov_scale, &sx[i], &sy[i]);
        }

        // clear
        SDL_SetRenderDrawColor(ren, 10, 12, 16, 255);
        SDL_RenderClear(ren);

        // optional backface culling for edges: compute face normals and draw edges only for faces facing camera
        // simple heuristic: normal.z < 0 (after rotation) means back-facing; camera looks towards +z in object+translation space
        int face_visible[6] = { 0 };
        for (int f = 0; f < 6; ++f) {
            Vec3 a = tv[faces[f][0]];
            Vec3 b = tv[faces[f][1]];
            Vec3 c = tv[faces[f][2]];
            Vec3 ab = { b.x - a.x, b.y - a.y, b.z - a.z };
            Vec3 ac = { c.x - a.x, c.y - a.y, c.z - a.z };
            // cross product ab x ac
            Vec3 n = {
                ab.y*ac.z - ab.z*ac.y,
                ab.z*ac.x - ab.x*ac.z,
                ab.x*ac.y - ab.y*ac.x
            };
            // camera is at z ~ -4 looking towards +z, so faces with n.z <= 0 face the camera
            face_visible[f] = (n.z <= 0.0f) ? 1 : 0;
        }

        // draw edges; color front vs back
        for (int eidx = 0; eidx < 12; ++eidx) {
            int i0 = edges[eidx][0];
            int i1 = edges[eidx][1];

            // Determine if this edge belongs to any visible face
            int draw = 0;
            for (int f = 0; f < 6; ++f) {
                if (!face_visible[f]) continue;
                // check membership of edge in face
                int count = 0;
                for (int k = 0; k < 4; ++k) {
                    int vi = faces[f][k];
                    if (vi == i0 || vi == i1) count++;
                }
                if (count == 2) { draw = 1; break; }
            }

            if (draw) {
                SDL_SetRenderDrawColor(ren, 240, 240, 240, 255);
            }
            else {
                SDL_SetRenderDrawColor(ren, 90, 90, 90, 255);
            }
            SDL_RenderDrawLine(ren, sx[i0], sy[i0], sx[i1], sy[i1]);
        }

        // present
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}