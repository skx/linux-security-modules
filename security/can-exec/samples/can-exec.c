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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>


// Log a message to STDOUT for testing, and to syslog for production use.
void logger( const char* format, ...) {

    char buf[256];
    va_list arg_ptr;

    // format
    va_start(arg_ptr, format);
    vsnprintf(buf, sizeof(buf)-1, format, arg_ptr);
    va_end(arg_ptr);

    // paranoia means we should ensure we're terminated.
    buf[sizeof(buf)] = '\0';

    // console output
    fprintf(stderr, buf);
    fprintf(stderr, "\n");

    // syslog
    syslog(LOG_NOTICE, buf);

}

int main(int argc, char *argv[])
{
    //
    // Ensure we have the correct number of arguments.
    //
    if (argc != 3)
    {
        logger("Invalid argument count.");
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
  	    logger("Invalid initial argument.");
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
        logger("Failed to convert UID %d to username", uid);
        return -1;
    }

    //
    // Log the UID, username, and command.
    //
    openlog("can-exec", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    logger("UID:%d USER:%s CMD:%s", uid, pwd->pw_name, prg);

    //
    // Root can execute everything.
    //
    if (uid == 0)
    {
        logger("root can execute everything");
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
        logger("Failed to open %s: denying execution.", filename);
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
        // TODO
        //
        // - We could allow matching regular expressions.
        // - We could allow matching based on directory
        //   - e.g. "allow /bin /sbin"
        //
        if (strncmp(prg, buffer, prg_len) == 0)
        {
            logger("allowing execution of command.");
            fclose(fp);
            return 0;
        }
    }

    //
    // If we reached here we have no match, so execution is denied.
    //
    logger("Denying execution of command - no match found.");
    fclose(fp);
    closelog();
    return -1;
}
