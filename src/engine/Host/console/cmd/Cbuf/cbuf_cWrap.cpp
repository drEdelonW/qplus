#include "cbuf.h"
#include "cbuf.hpp"


void Cmd_Wait_f() {
    cbuf.setWait();
}

void Cbuf_Init() {
    cbuf.Init();
}

void Cbuf_AddText(cStringRO text) {
    cbuf.AddText(text);
}

void Cbuf_InsertText(cStringRO text) {
    cbuf.InsertText(text);
}

void Cbuf_Execute() {
    cbuf.Execute();
}
