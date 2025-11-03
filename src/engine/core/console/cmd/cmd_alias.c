#include "cmd.h"
#include "cbuf.h"

#include <string.h>
#include "console.h"
#include "types.h"
#include "q_tools.h"
#include "zone.h"


#define MAX_ALIAS_NAME (32)

typedef struct CmdAlias_s CmdAlias_t;
typedef CmdAlias_t* CmdAlias_p;
struct CmdAlias_s {
    CmdAlias_p next;
    char       name[MAX_ALIAS_NAME];
    cString    value;
};

static CmdAlias_p _cmdAlias = NULL; // alias linked list entry point

void Cmd_Alias_f() {

    if (Cmd_Argc() == 1) {
        Con_Printf("Current alias commands:\n");
        for (CmdAlias_p aliasIt = _cmdAlias; aliasIt; aliasIt = aliasIt->next) {
            Con_Printf("%s : %s\n", aliasIt->name, aliasIt->value);
        }
        return;
    }

    cString _argSt = Cmd_Argv(1);
    if (strlen(_argSt) >= MAX_ALIAS_NAME) {
        Con_Printf("Alias name is too long\n");
        return;
    }

    // if the alias allready exists, reuse it
    CmdAlias_p aliasIt = _cmdAlias;
    for (; aliasIt; aliasIt = aliasIt->next) {
        if (!strcmp(_argSt, aliasIt->name)) {
            Z_Free(aliasIt->value);
            break;
        }
    }

    if (!aliasIt) {
        aliasIt = Z_Malloc(sizeof(CmdAlias_t));
        aliasIt->next = _cmdAlias;
        _cmdAlias = aliasIt;
    }
    strcpy(aliasIt->name, _argSt);

    // copy the rest of the command line
    char cmd[1024] = { 0 }; // start out with a null string
    int argCnt = Cmd_Argc();
    for (int i = 2; i < argCnt; i++) {
        strcat(cmd, Cmd_Argv(i));
        if (i != argCnt) {
            strcat(cmd, " ");
        }
    }
    strcat(cmd, "\n");

    aliasIt->value = CopyString(cmd);
}

extern cString cmd_argv[];
bool checkAlias() {
    for (CmdAlias_p aliasIt = _cmdAlias; aliasIt; aliasIt = aliasIt->next) {
        if (!Q_strcasecmp(cmd_argv[0], aliasIt->name)
            ) {
            Cbuf_InsertText(aliasIt->value);
            return true;
        }
    }
    return false;
}