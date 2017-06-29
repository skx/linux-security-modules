/*
 * User-space helper for the `can_exec` LSM.
 *
 * This binary is invoked to decide if execution should be permitted/denied,
 * by reading the file /etc/can-exec/$USERNAME.conf
 *
 * If a command is listed there it is allowed, otherwise denied.
 *
 * Steve
 * --
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

int main(int argc, char *argv[])
{
    //
    // Ensure arguments are sane.
    //
    if (argc != 3)
    {
        fprintf(stderr, "Invalid arguments");
        exit(-1);
    }

    //
    // Get the UID + program from the command-line arguments.
    //
    uid_t uid = atoi(argv[1]);
    char *prg = argv[2];

    //
    // Get the username
    //
    struct passwd *pwd;
    pwd = getpwuid(uid);

    //
    // If the username-conversion failed then we'll abort
    //
    if (pwd == NULL)
    {
        fprintf(stderr, "Failed to convert UID %d to username\n", uid);
        return -1;
    }

    //
    // Log to syslog the UID + binary
    //
    openlog("can-exec", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, "UID:%d USER:%s CMD:%s", uid, pwd->pw_name, prg);

    //
    // For interactive testing.
    //
    fprintf(stderr, "Testing if UID %d <%s> can exec %s\n", uid, pwd->pw_name, prg);

    //
    // This won't even get used, but for clarity
    //
    if (uid == 0)
    {
        fprintf(stderr, "root can execute everything\n");
        return 0;
    }


    //
    // We'll read a per-user configuration file to see if the
    // execution should be permitted.
    //
    char filename[128] = {'\0'};
    snprintf(filename, sizeof(filename) - 1,
             "/etc/can-exec/%s.conf", pwd->pw_name);

    //
    FILE* fp = fopen(filename, "r");

    if (! fp)
    {
        fprintf(stderr, "Configuration file not found - %s - denying\n", filename);
        return 0;
    }

    //
    // Read each line and look for a match
    //
    char buffer[255];

    while (fgets(buffer, 255, (FILE*) fp))
    {
        //
        // Strip out newlines
        //
        int i;

        for (i = 0; i < strlen(buffer); i++)
            if (buffer[i] == '\r' || buffer[i] == '\n')
                buffer[i] = '\0';

        //
        // Does the command appear in the file?
        //
        if (strcmp(prg, buffer) == 0)
        {
            fprintf(stderr, "Allowing execution of command\n");
            fclose(fp);
            return 0;
        }
    }

    //
    // If we reached here we have no match, so deny.
    //
    fprintf(stderr, "Denying execution of command - no match found\n");
    fclose(fp);
    closelog();
    return -1;
}
