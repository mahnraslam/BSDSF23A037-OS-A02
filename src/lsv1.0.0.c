/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*/
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;
void print_file_stat(const char *filename);  
void do_ls(const char *dir);
void do_ls_l(const char *dir) ;
int main(int argc, char  *argv[])
{	
    int opt ; 

    int long_fmt = 0 ; 
    if (argc == 1)
    {
        if (long_fmt)
            do_ls_l(".");
        else
            do_ls(".");
    }
    else
    {	
    
        while((opt= getopt(argc, argv, "l")) != -1)
	    {
	    switch(opt){
		case 'l' :
			long_fmt = 1 ; 
			break ;
        default:
            fprintf(stderr, "Unknown option: -%c\n", optopt);
            exit(EXIT_FAILURE);
		}
        

	    }
        
        if (optind == argc) { 
            if (long_fmt)
                do_ls_l(".");
             
        }
        else {
            for (int i = optind ; i < argc; i++)
            {
                printf("Directory listing of %s : \n ",  argv[i]);
                if (long_fmt)
                do_ls_l(argv[i]);
            else 
                do_ls(argv[i]) ;
            puts("");
            }
    }
    }
    return 0;
}
 

void print_file_stat(const char *filename)
{
    struct stat fileStat;
    if (lstat(filename, &fileStat) < 0) {
        fprintf(stderr, "Error: No such file or directory\n");
        return;
    }

    char str[10] = "---------";
    int mode = fileStat.st_mode;
    char type = '?';

    if (S_ISREG(mode)) type = '-';
    else if (S_ISDIR(mode)) type = 'd';
    else if (S_ISCHR(mode)) type = 'c';
    else if (S_ISBLK(mode)) type = 'b';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISLNK(mode)) type = 'l';
    else if (S_ISSOCK(mode)) type = 's';

    // owner
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    // group
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    // others
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';

    // special bits
    if (mode & S_ISUID) str[2] = 's';
    if (mode & S_ISGID) str[5] = 's';
    if (mode & S_ISVTX) str[8] = 't';

    struct passwd *pw = getpwuid(fileStat.st_uid);
    struct group *gr = getgrgid(fileStat.st_gid);
    char *user = pw ? pw->pw_name : "unknown";
    char *group = gr ? gr->gr_name : "unknown";

    time_t t = fileStat.st_mtime;
    time_t now = time(NULL);
    struct tm lt = *localtime(&t);
    char timebuf[80];

    if (difftime(now, t) > 60 * 60 * 24 * 30 * 6)
        strftime(timebuf, sizeof(timebuf), "%b %d  %Y", &lt);
    else
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", &lt);

    printf("%c%s %ld %-8s %-8s %8lld %s %s\n",
           type, str, fileStat.st_nlink, user, group,
           (long long)fileStat.st_size, timebuf, filename);
}
void do_ls_l(const char *dir)
{


    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);
        print_file_stat(full_path);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir); 
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}
