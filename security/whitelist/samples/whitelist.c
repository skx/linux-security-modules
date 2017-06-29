/*
 * Simple script to setup whitelisting attributes on files:
 *
 *   whitelist --add /bin/bash /bin/sh /usr/bin/id /usr/bin/uptime [..]
 *
 *   whitelist --del /bin/bash /bin/sh [..]
 *
 *   whitelist --list [/sbin /usr/sbin]
 *
 * With no arguments it displays whitelisted binaries beneath the current directory,
 * recursively.  If you prefer you can list the directories to search explicitly.
 *
 * Steve
 * --
 */

/* We want POSIX.1-2008 + XSI, i.e. SuSv4, features */
#define _XOPEN_SOURCE 700


#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <getopt.h>
#include <malloc.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <string.h>
#include <unistd.h>


static int add_flag = 0;
static int del_flag = 0;
static int list_flag = 0;


/*
 * Add the whitelist attribute to the given path.
 */
void add_whitelist(const char *path)
{
    char value[2] = "1";

    if (setxattr(path, "security.whitelisted", value, strlen(value), 0) == -1)
        perror("setxattr");
}

/*
 * Remove the whitelist attribute from the given path.
 */
void del_whitelist(const char *path)
{
    if (removexattr(path, "security.whitelisted") != 0)
        perror("removexattr");
}

int print_entry(const char *filepath, const struct stat *info,
                const int typeflag, struct FTW *pathinfo)
{

    /*
     * We only care about files.
     */
    if (typeflag == FTW_F)
    {
        int length = getxattr(filepath, "security.whitelisted", NULL, 0);

        if (length > 0)
            printf("%s\n", filepath);
    }

    return 0;
}
/*
 * Look at all the files in the given directory, show those that are
 * whitelisted.
 */
void list_whitelist(const char *directory)
{

#ifndef USE_FDS
#define USE_FDS 15
#endif


    int result = nftw(directory, print_entry, USE_FDS, FTW_PHYS);

    if (result > 0)
        perror("nftw");
}


/*
 * Entry-Point.
 */
int main(int argc, char **argv)
{
    int c;

    while (1)
    {
        /*
         * Parse our command-line options.
         */
        static struct option long_options[] =
        {
            {"add",  no_argument, &add_flag, 1},
            {"del",  no_argument, &del_flag, 1},
            {"list", no_argument, &list_flag, 1},
            {0, 0, 0, 0}
        };

        int option_index = 0;

        c = getopt_long(argc, argv, "", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
        case 0:

            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            abort();
        }
    }

    /* No action? Then list. */
    if ((add_flag == 0) && (del_flag == 0) && (list_flag == 0))
        list_flag = 1;

    /* Adding whitelist to some files? */
    if (add_flag)
    {
        while (optind < argc)
            add_whitelist(argv[optind++]) ;
    }

    /* Removing whitelist from some files? */
    if (del_flag)
    {
        while (optind < argc)
            del_whitelist(argv[optind++]) ;
    }

    /* Listing files */
    if (list_flag)
    {
        /* If we have arguments assume they're directories to list. */
        if (optind < argc)
        {
            while (optind < argc)
                list_whitelist(argv[optind++]);
        }
        else
        {
            list_whitelist(".");
        }
    }

    /* All done */
    exit(0);
}
