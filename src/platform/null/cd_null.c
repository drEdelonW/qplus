// cd_null.c — stub for non-Linux builds

int CDAudio_Init(void) { return 0; }
void CDAudio_Shutdown(void) {}
void CDAudio_Play(int track, int looping) {}
void CDAudio_Stop(void) {}
void CDAudio_Pause(void) {}
void CDAudio_Resume(void) {}
void CDAudio_Update(void) {}