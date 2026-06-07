// fpu_stubs.c — заглушки для MinGW, когда нет ASM/FPU кода
void MaskExceptions() {}
void Sys_SetFPCW() {}
void Sys_PushFPCW_SetHigh() {}
void Sys_PopFPCW() {}
