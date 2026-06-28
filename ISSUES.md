
1. soft render give texture distortion under shoot. palette not clamped from high?

x1. memory leak on texturize. see: src/engine/ui/screen.c:315 void SCR_DrawRam() - not see for a long time... fixed?
R_RecursiveWorldNode

x2. mipmap issues  - not see for a long time... fixed?

FIXED: cyclic bright down in soft render on x86 linux