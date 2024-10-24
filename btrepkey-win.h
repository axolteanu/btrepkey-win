#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <glib.h>
#include <argp.h>

#define BUF_SIZE 128
#define BUF_MAX 512
#define RESTART_MAX_TRIES 2
#define MAC_LEN 17
#define MAC_TRIM_LEN (MAC_LEN - 5)
#define KEY_LEN 32
#define CFG_PATH "/etc/btrepkey-win.cfg"
#define WPATHSIZ 128
#define FBUFSIZ 512
#define UNEXP_ERR_MSG "An unexpected error has occurred"
#define BTREPKEY_WIN_ERROR btrepkey_win_error_quark()

typedef enum {
    E_NOTFOUND,
    E_EMPTY,
    E_BADFORMAT,
    E_NOTADIR,
    E_FAILED
} BTRepKeyWinError;

typedef enum {
    UM_FAILURE,
    UM_WARNING
} UserMessageType;

int fsol(FILE* fp);
int fscans(FILE* fp, char *s);
int fget_wpath(FILE* fp, char* wpath);
gboolean validate_wpath(char* wpath);
int direxists(char* dirpath);