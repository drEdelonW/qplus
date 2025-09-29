// cd_null.c — stub for non-Linux builds

int CDAudio_Init() { return 0; }
void CDAudio_Shutdown() {}
void CDAudio_Play(int track, int looping) {}
void CDAudio_Stop() {}
void CDAudio_Pause() {}
void CDAudio_Resume() {}
void CDAudio_Update() {}