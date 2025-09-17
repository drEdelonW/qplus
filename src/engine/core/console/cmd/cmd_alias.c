#include <string.h>
#include "cmd.h"
#include "console.h"
#include "zone.h"
#include "q_tools.h"

extern char* cmd_argv[];

#define	MAX_ALIAS_NAME	(32)
typedef struct cmdalias_s{
    struct cmdalias_s*  next;
    char                name[MAX_ALIAS_NAME];
    char*               value;
} cmdalias_t;

static cmdalias_t* cmd_alias = NULL; // alias linked list entry point


void Cmd_Alias_f(void){
	cmdalias_t* aliasIt;
	char        cmd[1024];

    if (Cmd_Argc() == 1){
        Con_Printf("Current alias commands:\n");
        for (aliasIt = cmd_alias; aliasIt; aliasIt = aliasIt->next){
            Con_Printf("%s : %s\n", aliasIt->name, aliasIt->value);
        }
        return;
    }

    char* _argSt = Cmd_Argv(1);
    if (strlen(_argSt) >= MAX_ALIAS_NAME){
        Con_Printf ("Alias name is too long\n");
        return;
    }

    // if the alias allready exists, reuse it
    for (aliasIt = cmd_alias; aliasIt; aliasIt = aliasIt->next){
        if (!strcmp(_argSt, aliasIt->name)){
            Z_Free(aliasIt->value);
            break;
        }
    }

    if (!aliasIt){
        aliasIt = Z_Malloc(sizeof(cmdalias_t));
        aliasIt->next = cmd_alias;
        cmd_alias = aliasIt;
    }
    strcpy(aliasIt->name, _argSt);

    // copy the rest of the command line
    cmd[0] = 0;		// start out with a null string
    int argCnt = Cmd_Argc();
    for (int i = 2; i < argCnt; i++){
        strcat(cmd, Cmd_Argv(i));
        if (i != argCnt){
            strcat(cmd, " ");
        }
    }
    strcat(cmd, "\n");

    aliasIt->value = CopyString(cmd);
}

bool checkAlias(){
	for (cmdalias_t* aliasIt = cmd_alias; aliasIt; aliasIt = aliasIt->next){
		if (!Q_strcasecmp(cmd_argv[0], aliasIt->name)){
			Cbuf_InsertText(aliasIt->value);
			return true;
		}
	}
	return false;
}