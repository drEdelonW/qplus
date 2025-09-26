#include "cmd.h"

#include <string.h>
#include "console.h"
#include "types.h"
#include "q_tools.h"
#include "zone.h"

extern cstring cmd_argv[];

#define MAX_ALIAS_NAME (32)

struct cmdalias_s;
typedef struct cmdalias_s cmdalias_t;
typedef cmdalias_t* cmdalias_p;
struct cmdalias_s {
    cmdalias_p next;
    char       name[MAX_ALIAS_NAME];
    cstring    value;
};

static cmdalias_p cmd_alias = NULL; // alias linked list entry point


void Cmd_Alias_f() {
    cmdalias_p aliasIt;
    char        cmd[1024];

    if (Cmd_Argc() == 1) {
        Con_Printf("Current alias commands:\n");
        for (aliasIt = cmd_alias; aliasIt; aliasIt = aliasIt->next) {
            Con_Printf("%s : %s\n", aliasIt->name, aliasIt->value);
        }
        return;
    }

    cstring _argSt = Cmd_Argv(1);
    if (strlen(_argSt) >= MAX_ALIAS_NAME) {
        Con_Printf("Alias name is too long\n");
        return;
    }

    // if the alias allready exists, reuse it
    for (aliasIt = cmd_alias; aliasIt; aliasIt = aliasIt->next) {
        if (!strcmp(_argSt, aliasIt->name)) {
            Z_Free(aliasIt->value);
            break;
        }
    }

    if (!aliasIt) {
        aliasIt = Z_Malloc(sizeof(cmdalias_t));
        aliasIt->next = cmd_alias;
        cmd_alias = aliasIt;
    }
    strcpy(aliasIt->name, _argSt);

    // copy the rest of the command line
    cmd[0] = 0;  // start out with a null string
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

bool checkAlias() {
    for (cmdalias_p aliasIt = cmd_alias; aliasIt; aliasIt = aliasIt->next) {
        if (!Q_strcasecmp(cmd_argv[0], aliasIt->name)) {
            Cbuf_InsertText(aliasIt->value);
            return true;
        }
    }
    return false;
}