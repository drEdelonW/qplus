// fpu_stubs.c — заглушки для MinGW, когда нет ASM/FPU кода
void MaskExceptions(void) {}
void Sys_SetFPCW(void) {}
void Sys_PushFPCW_SetHigh(void) {}
void Sys_PopFPCW(void) {}
