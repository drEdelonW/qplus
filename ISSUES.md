
1. soft render give texture distortion under shoot. palette not clamped from high?

x1. memory leak on texturize. see: src/engine/ui/screen.c:315 void SCR_DrawRam() - not see for a long time... fixed?
R_RecursiveWorldNode

x2. mipmap issues  - not see for a long time... fixed?

FIXED: cyclic bright down in soft render on x86 linux



2. on load [map e1m3] console messages:
```
75 entities inhibited
walkmonster in wall at: ' 40.0 -468.0 -360.0'
walkmonster in wall at: ' 88.0 -468.0 -360.0'
walkmonster in wall at: '-1218.0 -532.0 -440.0'
walkmonster in wall at: '-1270.0 -558.0 -440.0'
walkmonster in wall at: '-866.0 -44.0 -456.0'
walkmonster in wall at: '-194.0 -1466.0 112.0'
walkmonster in wall at: '-154.0 -1002.0  72.0'
walkmonster in wall at: '704.0 -1192.0 -128.0'
walkmonster in wall at: '184.0 -856.0 -328.0'
walkmonster in wall at: '-48.0 -856.0 -328.0'
walkmonster in wall at: '-556.0 -12.0 -368.0'
walkmonster in wall at: '-1384.0 -856.0 -368.0'
walkmonster in wall at: '384.0 -352.0 -288.0'
walkmonster in wall at: '776.0 -360.0 -288.0'
walkmonster in wall at: '320.0 -616.0 -288.0'
walkmonster in wall at: '-224.0 -656.0 -328.0'
walkmonster in wall at: '238.0 -18.0 -184.0'
walkmonster in wall at: '552.0 -216.0 -288.0'
walkmonster in wall at: '800.0 -216.0 -288.0'
walkmonster in wall at: '656.0 -480.0 -288.0'
walkmonster in wall at: '1024.0 -88.0 -184.0'
walkmonster in wall at: '1216.0 -72.0 -184.0'
walkmonster in wall at: '1216.0 -608.0 -184.0'
walkmonster in wall at: '1024.0 -604.0 -184.0'
walkmonster in wall at: '1224.0 768.0 560.0'
walkmonster in wall at: '302.0 -1410.0 -40.0'
walkmonster in wall at: '790.0 -346.0 -40.0'
walkmonster in wall at: ' 34.0 -1466.0  72.0'
walkmonster in wall at: '1584.0 816.0 560.0'
walkmonster in wall at: '-912.0 -88.0 -456.0'
walkmonster in wall at: '-1400.0 -256.0 -72.0'
walkmonster in wall at: '1608.0 -96.0  80.0'
walkmonster in wall at: '1608.0 -344.0  84.0'
walkmonster in wall at: '368.0 -1136.0 -128.0'
walkmonster in wall at: '-16.0 -904.0 -328.0'
walkmonster in wall at: '808.0 -488.0 -288.0'
walkmonster in wall at: '-136.0 -1416.0 104.0'
walkmonster in wall at: '1536.0 104.0 -136.0'
walkmonster in wall at: '1440.0 168.0 -136.0'
walkmonster in wall at: '1592.0 112.0 -136.0'
walkmonster in wall at: '-104.0 -1480.0 112.0'
walkmonster in wall at: ' 28.0 -1352.0  72.0'
Server spawned.
```