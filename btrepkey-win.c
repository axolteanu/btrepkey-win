#include "btrepkey-win.h"
#include <stdio.h>
#include <string.h>

G_DEFINE_QUARK (btrepkey_win_error_quark, btrepkey_win_error)

void print_message(UserMessageType umt, const char* format, ...) {
    switch (umt) {
        case UM_FAILURE:
            fprintf(stderr, "failure: ");
        case UM_WARNING:
            fprintf(stderr, "warning: ");
    }

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void set_error_fatal_unexp(GError** error) {
    g_set_error(error, BTREPKEY_WIN_ERROR, E_FAILED, UNEXP_ERR_MSG);
}

char* read_ptm(int fdm) {
    char *str = calloc(BUF_MAX + 1, sizeof(char));

    if (str == NULL) {
        return NULL;
    }

    char buf[BUF_MAX + 1] = "";
    char c;
    int rc, i = 0;
    gboolean fr = TRUE;
    int flags = fcntl(fdm, F_GETFL, 0);

    while ((rc = read(fdm, &c, 1)) != EOF) {
        if (fr) {
            fcntl(fdm, F_SETFL, flags | O_NONBLOCK);
            fr = FALSE;
        }

        buf[i++] = c;
        if (i == BUF_MAX) {
            strcat(str, buf);
            str = realloc(str, strlen(str) + BUF_MAX + 1);
            if (str == NULL) {
                return NULL;
            }
            memset(buf, '\0', sizeof(buf));
            i = 0;
        }

        usleep(100);
    }

    strcat(str, buf);
    fcntl(fdm, F_SETFL, flags);

    return str;
}

void trim_macaddr(char *src, char *dst) {
    for (int i = 0, j = 0; i < strlen(src); i++)
        if (src[i] != ':')
            dst[j++] = tolower(src[i]);
}

gboolean fetch_btadap_macaddr(char* mac, int fdm, int fds) {
    char *args[3] = {"/bin/bluetoothctl", "list", NULL};
    if(!g_spawn_async_with_fds(NULL, args, NULL, G_SPAWN_DEFAULT, NULL, NULL,NULL, fds, fds, fds, NULL)) {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
        return FALSE;
    }

    char *str = read_ptm(fdm);
    if (str == NULL) {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
        return FALSE;
    }

    sscanf(str, "Controller %s", mac);
    free(str);

    if(strlen(mac) != MAC_LEN) {
        print_message(UM_FAILURE, "Could not find MAC address of Bluetooth adapter");
        return FALSE;
    }

    return TRUE;
}

gboolean fetch_btdev_macaddr(char* mac, char* name, int fdm, int fds) {
    char *args[3] = {"/bin/bluetoothctl", "devices", NULL};
    if(!g_spawn_async_with_fds(NULL, args, NULL, G_SPAWN_DEFAULT, NULL, NULL,NULL, fds, fds, fds, NULL)) {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
        return FALSE;
    }

    char *str = read_ptm(fdm);
    if (str == NULL) {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
        return FALSE;
    }

    char nameCur[50] = "";
    char *p = str;
    gboolean deviceFound = FALSE;
    while(sscanf(p, "Device %s %s", mac, nameCur) != EOF) {
        if(strcmp(name, nameCur) == 0) {
            deviceFound = TRUE;
            break;
        }
        int offset;
        sscanf(p, "%*[^\n]\n%n", &offset);
        p += offset;
        if(*p == '\0')
            break;
    }

    free(str);

    if(!deviceFound) {
        print_message(UM_FAILURE, "Could not find Bluetooth device '%s'", name);
        return FALSE;
    } else if (strlen(mac) != MAC_LEN) {
        print_message(UM_FAILURE, "Could not find MAC address of Bluetooth device '%s'", name);
        return FALSE;
    }

    return TRUE;
}

void setup_exec_chntpw(gpointer user_data) {
    int fds = *(int *)user_data;
    struct termios term_settings;
    int rc = tcgetattr(fds, &term_settings);
    cfmakeraw(&term_settings);
    tcsetattr(fds, TCSANOW, &term_settings);
}

gboolean write_ptm_cmd(int fdm, char* format, ...) {
    va_list args_count, args_print;
    va_start(args_print, format);
    va_copy(args_count, args_print);
    int size = vsnprintf(0, 0, format, args_count) + 1;
    va_end(args_count);
    char *cmd = malloc(size);
    vsnprintf(cmd, size, format, args_print);
    va_end(args_print);
    cmd[size-1] = '\n';
    int rc = write(fdm, cmd, size);
    
    if(rc == -1) {
        return FALSE;
    }

    free(cmd);
    return TRUE;
}

int fetch_winkey(char* wpath, char* key, char* adapMac, char* devMac, int fdm, int fds) {
    char *cpath = g_strconcat(wpath, "/Windows/System32/config", NULL);
    char *args[4] = {"/sbin/chntpw", "-e", "SYSTEM", (char *) NULL};
    gboolean isSuccess = g_spawn_async_with_fds(
        cpath, args, NULL, G_SPAWN_DEFAULT, setup_exec_chntpw, &fds,NULL, fds, fds, fds, NULL);
    
    free(cpath);

    if(!isSuccess) {
        return -1;
    }
    
    char *str = read_ptm(fdm);
    if (str == NULL)
        return -1;
    free(str);

    if(!write_ptm_cmd(fdm, "cd ControlSet001\\Services\\BTHPORT\\Parameters\\Keys"))
        return -1;
    
    if ((str = read_ptm(fdm)) == NULL)
        return -1;
    free(str);

    char adapMacTr[MAC_TRIM_LEN + 1] = "";
    trim_macaddr(adapMac, adapMacTr);
    
    if(!write_ptm_cmd(fdm, "cd %s", adapMacTr))
        return -1;

    if ((str = read_ptm(fdm)) == NULL)
        return -1;
    free(str);

    char devMacTr[MAC_TRIM_LEN + 1] = "";
    trim_macaddr(devMac, devMacTr);
    
    if(!write_ptm_cmd(fdm, "hex %s", devMacTr))
        return -1;

    if ((str = read_ptm(fdm)) == NULL)
        return -1;
    
    int offset;
    char *sp = str;
    sscanf(str, "%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s %n", &offset);
    sp += offset;
    for(int i = 0; i < 32; sp++) {
        if(*sp != ' ') {
            key[i++] = *sp;
        }
    }
    key[32] = '\0';

    free(str);

    int len = strlen(key);
    if(len != KEY_LEN) {
        return 0;
    }

    return len;
}

gboolean replace_linkey(char* key, char* adapMac, char* devMac) {
    char* fpath = g_strconcat("/var/lib/bluetooth/", adapMac, "/", devMac, "/info", NULL);
    FILE *fp = fopen(fpath, "r+");

    free(fpath);

    if(fp == NULL) {
        return FALSE;
    }

    char target[5];
    gboolean isSuccess = FALSE;
    while(fscanf(fp, "%4s", target) != EOF) {
        if(strcmp(target, "Key=") == 0){
            isSuccess = fputs(key, fp) != EOF;
            break;
        }
        fscanf(fp, "%*[^\n]\n");   
    }

    if(fclose(fp) == -1) {
        return FALSE;
    }

    return isSuccess;
}

gboolean restart_btserv() {
    char *args[3] = {"/etc/init.d/bluetooth", "restart", NULL};
    return g_spawn_sync(NULL, args, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, NULL, NULL);
}

gboolean is_btserv_running(GError** error) {
    int waitTimes[RESTART_MAX_TRIES] = {1, 3};
    for(int i = 0; i < RESTART_MAX_TRIES; i++) {
        sleep(waitTimes[i]);

        char *args[5] = {"/bin/systemctl", "is-active", "--quiet", "bluetooth", NULL};
        int exStat;
        if(!g_spawn_sync(NULL, args, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, &exStat, error)) {
            return FALSE;
        }

        if(WEXITSTATUS(exStat) == 0)
            return TRUE;
    }

    return FALSE;
}

gboolean connect_btdev(char* macAddress) {
    char *args[4] = {"/bin/bluetoothctl", "connect", macAddress, NULL};
    return g_spawn_sync(NULL, args, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, NULL, NULL);
}

gboolean create_pty(int* fdmp, int* fdsp) {
    int fdm = posix_openpt(O_RDWR);
    if (fdm < 0) {
        return FALSE;
    }
    
    if (grantpt(fdm) != 0) {
        return FALSE;
    }

    if (unlockpt(fdm) != 0) {
        return FALSE;
    }

    char *ptsn = ptsname(fdm);
    int fds = open(ptsn, O_RDWR);
    if (fds < 0) {
        return FALSE;
    }

    *fdmp = fdm;
    *fdsp = fds;

    return TRUE;
}

gboolean fetch_wpath(char *wpath) {
    FILE *fp = fopen(CFG_PATH, "r+");
    if(fp == NULL) {
        if(errno == ENOENT) {
            print_message(UM_FAILURE, "WPATH not found because configuration file '%s' does not exist");
        } else {
            print_message(UM_FAILURE, UNEXP_ERR_MSG);
        }
        return FALSE;
    }

    int rc = fget_wpath(fp, wpath);

    if(fclose(fp) == -1) {
        print_message(UM_WARNING, UNEXP_ERR_MSG);
        return FALSE;
    }

    if(rc) {
        return TRUE;
    } else if (rc == 0) {
        print_message(UM_FAILURE, "WPATH is missing");
    } else {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
    }

    return FALSE;
}

int fget_wpath(FILE* fp, char* wpath) {
    int rv;
    char buf[FBUFSIZ+1];
    while(!feof(fp)) {
        rv = fscans(fp, "WPATH=");
        if(rv > 0) {
            if (fgets(wpath, FBUFSIZ, fp) == NULL && ferror(fp)) {
                return -1;
            }
            wpath[strcspn(wpath, "\n")] = 0;
            return 1;
        } else if (rv == 0) {
            while(fgets(buf, FBUFSIZ, fp) != NULL && !fsol(fp))
                continue;
            if(ferror(fp)) {
                return -1;
            }
        } else {
            return -1;
        }
    }
    return 0;
}

int fscans(FILE* fp, char *s) {
    int spos = ftell(fp);
    int len = strlen(s);
    char *buf = malloc(sizeof(char)*(len+1));
    int rv = 0;

    if(fgets(buf, len+1, fp) == NULL) {
        if(ferror(fp))
            rv = -1;
        goto end;
    }

    if (strcmp(buf, s) == 0)  {
        rv = len;
        goto end;
    }

    int offs = spos - ftell(fp);
    if(fseek(fp, offs, SEEK_CUR) != 0)
        rv = -1;

    end:
    free(buf);
    return rv;
}

int fsol(FILE* fp) {
    return fseek(fp, -1, SEEK_CUR) == -1 || fgetc(fp) == '\n';
}

gboolean validate_wpath(char* wpath) {
    int len = strlen(wpath);

    if(len == 0 || wpath[0] == '\n') {
        print_message(UM_FAILURE, "WPATH cannot be empty");
        return FALSE;
    }

    if(wpath[len-1] != '/') {
        print_message(UM_FAILURE, "WPATH must end with '/'");
        return FALSE;
    }

    int rc = direxists(wpath);
    if(rc) {
        return TRUE;
    } else if (rc == 0) {
        print_message(UM_FAILURE, "WPATH '%s' is not a path to an existing directory", wpath);
    } else {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
    }

    return FALSE;
}

int direxists(char* dirpath) {
    DIR *dir = opendir(dirpath);
    if(dir) {
        if(closedir(dir) == 0) {
            return 1;
        } else {
            return -1;
        }
    } else if(errno == ENOENT) {
        return 0;
    } else {
        return -1;
    }
}

int btrepkey_win(char* devName) {
    GError *err = NULL;

    char wpath[WPATHSIZ + 1] = "";
    if(!fetch_wpath(wpath)) {
        return EXIT_FAILURE;
    }

    if(!validate_wpath(wpath)) {
        return EXIT_FAILURE;
    }

    int fdm, fds = 0;
    if(!create_pty(&fdm, &fds)){
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
        return EXIT_FAILURE;
    }

    char adapMac[MAC_LEN + 1] = "";
    if(!fetch_btadap_macaddr(adapMac, fdm, fds)){
        return EXIT_FAILURE;
    }
  
    char devMac[MAC_LEN + 1] = "";
    if(!fetch_btdev_macaddr(devMac, devName, fdm, fds)){
        return EXIT_FAILURE;
    }

    char key[KEY_LEN + 1] = "";
    int rc = fetch_winkey(wpath, key, adapMac, devMac, fdm, fds);
    if(rc == 0) {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
        return EXIT_FAILURE;
    } else if(rc == -1) {
        print_message(UM_FAILURE, "Windows key not found");
        return EXIT_FAILURE;
    }

    printf("Windows key found.\n");

    if(!replace_linkey(key, adapMac, devMac)) {
        print_message(UM_FAILURE, UNEXP_ERR_MSG);
        return EXIT_FAILURE;
    }

    printf("Linux key replaced.\n");

    if(!restart_btserv()) {
        print_message(UM_WARNING, "Could not restart Bluetooth service. \
Please try restarting the service using the command '/etc/init.d/bluetooth restart', then reconnect your Bluetooth device manually\n" );
        return EXIT_SUCCESS;
    }

    gboolean serviceRunning = is_btserv_running(&err);
    if(serviceRunning) {
        if(!connect_btdev(devMac)) {
            print_message(UM_WARNING, "Could not reconnect Bluetooth device. Try reconnecting your device manually\n");
        }
    } else if(err != NULL) {
        print_message(UM_WARNING, "Could not reconnect Bluetooth device. \
Make sure the service has restarted, then try reconnecting your device manually\n");
    } else {
        print_message(UM_WARNING, "Could not reconnect Bluetooth device: Bluetooth service took too long to restart \
Make sure the service has restarted, then try reconnecting your device manually\n");
    }

    return EXIT_SUCCESS;
}

static int parse_opts (int key, char *arg, struct argp_state *state) {
    switch (key) {
        case ARGP_KEY_ARG:
            if(state->next < state-> argc)
                argp_failure (state, 1, 0, "too many arguments"); 
            if (state->arg_num == 0)
                return btrepkey_win(arg);
            break;
        case ARGP_KEY_END:
            if (state->arg_num == 0)
                argp_failure (state, 1, 0, "missing device name"); 
            break;
    }
    return 0;
}

int main(int argc, char **argv) {
    struct argp argp = { 0, parse_opts, "DEVICE_NAME", "\nPair a Bluetooth device using an existing Windows key.\n\n\
The path to the Windows partition must be specified in a WPATH environment variable."};
    return argp_parse (&argp, argc, argv, 0, 0, 0);
}