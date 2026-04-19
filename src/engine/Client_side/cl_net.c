#include "cl_net.h"
#include "client.h"
#include "common.h"
#include "msg.h"
#include "sound.h"
#include "console.h"
#include "host.h"
#include "screen.h"

#include "cvar_q1.h"

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect() {
    // stop sounds (especially looping!)
    S_StopAllSounds(true);

    // bring the console down and fade the colors back to normal
    // SCR_BringDownConsole ();

    // if running a local server, shut it down
    if (cls.demoplayback)   CL_StopPlayback();
    else if (cls.state == ca_connected) {
        if (cls.demorecording)
            CL_Stop_f();

        Con_DPrintf("Sending clc_disconnect\n");
        SZ_Clear(&cls.message);
        MSG_WriteByte(&cls.message, clc_disconnect);    NET_SendUnreliableMessage(cls.netcon, &cls.message);
        SZ_Clear(&cls.message);
        NET_Close(cls.netcon);

        cls.state = ca_disconnected;
        if (Host_IsServerActive())      Host_ShutdownServer(false);
    }

    cls.demoplayback = cls.timedemo = false;
    cls.signon = 0;
}



/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply() {
    Con_DPrintf("CL_SignonReply: %i\n", cls.signon);

    switch (cls.signon) {
    case 1: {
        MSG_WriteByte(&cls.message, clc_stringcmd); MSG_WriteString(&cls.message, "prespawn");
    } break;

    case 2: {
        MSG_WriteByte(&cls.message, clc_stringcmd); MSG_WriteString(&cls.message, va("name \"%s\"\n", cl_name.string));

        MSG_WriteByte(&cls.message, clc_stringcmd);
        MSG_WriteString(
            &cls.message,
            va("color %i %i\n",
                ((int)cl_color.value) >> 4,
                ((int)cl_color.value) & 15
            ));

        char  str[8192];
        MSG_WriteByte(&cls.message, clc_stringcmd);
        snprintf(str, sizeof(str), "spawn %s", cls.spawnparms);
        MSG_WriteString(&cls.message, str);
    } break;

    case 3: {
        MSG_WriteByte(&cls.message, clc_stringcmd); MSG_WriteString(&cls.message, "begin");
        Cache_Report();  // print remaining memory
    } break;

    case SIGNONS: {
        SCR_EndLoadingPlaque();  // allow normal screen updates
    } break;
    }
}
