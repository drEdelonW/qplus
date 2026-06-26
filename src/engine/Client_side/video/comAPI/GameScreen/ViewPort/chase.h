#pragma once
//
// chase
//
#ifdef __cplusplus
extern "C" {
#endif

    void Chase_Init();
    void Chase_Reset();
    void Chase_Update(); // using r_refdef.vieworg and cl.viewangles

#ifdef __cplusplus
}
#endif