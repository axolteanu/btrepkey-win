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

#define CFG_PATH "/etc/btrepkey-win.cfg"
#define UNEXP_ERR_MSG "An unexpected error has occurred"

#define FBUFSIZ 512
#define PTBUFSIZ 512
#define WPATHSIZ 128

#define MAC_LEN 17
#define MAC_TRIM_LEN (MAC_LEN - 5)
#define KEY_LEN 32

#define RESTART_MAX_TRIES 2
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

static int parse_opts (int key, char *arg, struct argp_state *state);
int btrepkey_win(char* devName);

void print_message(UserMessageType umt, const char* format, ...);
void set_error_fatal_unexp(GError** error);

gboolean create_pty(int* fdmp, int* fdsp);
char* read_ptm(int fdm);
gboolean write_ptm_cmd(int fdm, char* format, ...);

gboolean fetch_wpath(char *wpath);
int fget_wpath(FILE* fp, char* wpath);
int fsol(FILE* fp);
int fscans(FILE* fp, char *s);
gboolean validate_wpath(char* wpath);
int direxists(char* dirpath);

gboolean fetch_btadap_macaddr(char* mac, int fdm, int fds);
gboolean fetch_btdev_macaddr(char* mac, char* name, int fdm, int fds);
void trim_macaddr(char *src, char *dst);

void setup_exec_chntpw(gpointer user_data);
int fetch_winkey(char* wpath, char* key, char* adapMac, char* devMac, int fdm, int fds);
gboolean replace_linkey(char* key, char* adapMac, char* devMac);

gboolean connect_btdev(char* macAddress);
gboolean is_btserv_running(GError** error);
gboolean restart_btserv();