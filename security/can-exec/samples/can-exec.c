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
    // Ensure we have the correct number of arguments.
    //
    if (argc != 3)
    {
        fprintf(stderr, "Invalid arguments\n");
        exit(-1);
    }

    //
    // First argument should be 100% numeric.
    //
    for (int i = 0; i < strlen(argv[1]); i++)
    {
        if ((argv[1][i] < '0') ||
                (argv[1][i] > '9'))
        {
            fprintf(stderr, "Invalid initial argument\n");
            return -1;
        }
    }


    //
    // Get the UID + program from the command-line arguments.
    //
    uid_t uid      = atoi(argv[1]);
    char *prg      = argv[2];
    size_t prg_len = strlen(prg);


    //
    // Get the username
    //
    struct passwd *pwd = getpwuid(uid);

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
    // Root can execute everything.
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
    // Open the configuration-file.
    //
    FILE* fp = fopen(filename, "r");

    if (! fp)
    {
        fprintf(stderr, "Failed to open - %s - denying execution.\n", filename);
        return -1;
    }

    //
    // Read each line and look for a match
    //
    char buffer[255];

    while (fgets(buffer, sizeof(buffer) - 1, (FILE*) fp))
    {
        //
        // Strip out newlines
        //
        for (int i = 0; i < strlen(buffer); i++)
            if (buffer[i] == '\r' || buffer[i] == '\n')
                buffer[i] = '\0';

        //
        // Skip lines which are comments.
        //
        if (buffer[0] == '#')
            continue;

        //
        // Does the command the user is trying to execute
        // match this line?
        //
        if (strncmp(prg, buffer, prg_len) == 0)
        {
            fprintf(stderr, "Allowing execution of command.\n");
            fclose(fp);
            return 0;
        }
    }

    //
    // If we reached here we have no match, so the execution
    // will be denied.
    //
    fprintf(stderr, "Denying execution of command - no match found.\n");
    fclose(fp);
    closelog();
    return -1;
}
