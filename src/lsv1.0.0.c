/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0 (updated with -R recursion)
* Usage:
*       $ lsv1.0.0 
*       $ lsv1.0.0  /home
*       $ lsv1.0.0  -R /home
*       $ lsv1.0.0  -lR /etc
*/
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
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
void do_ls_l(const char *dir);
void do_ls_x(const char *dir);

// Display mode flags
enum DisplayMode { DEFAULT, LONG, HORIZONTAL };

// Global flag for recursion (-R)
int recursive_flag = 0;

// ============================================================================
// COLOR MACROS - ANSI escape codes for terminal colors
// ============================================================================
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_CYAN    "\033[0;36m"

// ============================================================================
// COLOR DETERMINATION FUNCTION
// ============================================================================
const char* get_file_color(const char *filename, mode_t mode) {
    if (S_ISDIR(mode)) return COLOR_BLUE;
    if (S_ISLNK(mode)) return COLOR_PINK;
    if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISSOCK(mode) || S_ISFIFO(mode)) return COLOR_CYAN;
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) return COLOR_GREEN;

    const char *ext = strrchr(filename, '.');
    if (ext && (
        strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0 ||
        strcmp(ext, ".zip") == 0 || strcmp(ext, ".bz2") == 0 ||
        strcmp(ext, ".xz") == 0 || strcmp(ext, ".tgz") == 0))
        return COLOR_RED;

    return COLOR_RESET;
}

// ============================================================================
// MAIN FUNCTION - Added support for -R flag
// ============================================================================
int main(int argc, char *argv[]) {
    int opt;
    int display_mode = DEFAULT;

    // Added R flag here ↓
    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l': display_mode = LONG; break;
            case 'x': display_mode = HORIZONTAL; break;
            case 'R': recursive_flag = 1; break;  // enable recursion
            default:
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc) {
        if (display_mode == LONG)
            do_ls_l(".");
        else if (display_mode == HORIZONTAL)
            do_ls_x(".");
        else
            do_ls(".");
    } else {
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1) printf("%s:\n", argv[i]);
            if (display_mode == LONG)
                do_ls_l(argv[i]);
            else if (display_mode == HORIZONTAL)
                do_ls_x(argv[i]);
            else
                do_ls(argv[i]);
            if (i < argc - 1) printf("\n");
        }
    }

    return 0;
}

// ============================================================================
// PRINT FILE STAT FUNCTION (Unchanged)
// ============================================================================
void print_file_stat(const char *filename) {
    struct stat fileStat;
    if (lstat(filename, &fileStat) < 0) {
        fprintf(stderr, "Error: No such file or directory\n");
        return;
    }

    const char *display_name = strrchr(filename, '/');
    display_name = display_name ? display_name + 1 : filename;
    const char *color_code = get_file_color(display_name, fileStat.st_mode);

    char str[10] = "---------";
    int mode = fileStat.st_mode;
    char type = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' : S_ISCHR(mode) ? 'c' :
                S_ISBLK(mode) ? 'b' : S_ISFIFO(mode) ? 'p' :
                S_ISSOCK(mode) ? 's' : '-';

    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';

    if (mode & S_ISUID) str[2] = 's';
    if (mode & S_ISGID) str[5] = 's';
    if (mode & S_ISVTX) str[8] = 't';

    struct passwd *pw = getpwuid(fileStat.st_uid);
    struct group *gr = getgrgid(fileStat.st_gid);
    char *user = pw ? pw->pw_name : "unknown";
    char *group = gr ? gr->gr_name : "unknown";

    char timebuf[80];
    struct tm lt = *localtime(&fileStat.st_mtime);
    time_t now = time(NULL);
    if (difftime(now, fileStat.st_mtime) > 60 * 60 * 24 * 30 * 6)
        strftime(timebuf, sizeof(timebuf), "%b %d  %Y", &lt);
    else
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", &lt);

    printf("%c%s %ld %-8s %-8s %8lld %s %s%s%s\n",
           type, str, fileStat.st_nlink, user, group,
           (long long)fileStat.st_size, timebuf, color_code, display_name, COLOR_RESET);
}

int cmpstr(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

// ============================================================================
// DEFAULT MODE (with recursion support)
// ============================================================================
void do_ls(const char *dir) {
    printf("%s:\n", dir); // Show current directory name (for -R)
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (!dp) { fprintf(stderr, "Cannot open directory: %s\n", dir); return; }

    size_t count = 0, capacity = 64;
    char **filenames = malloc(capacity * sizeof(char *));
    if (!filenames) { perror("malloc"); closedir(dp); return; }

    size_t max_len = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (count >= capacity) {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
        }
        filenames[count++] = strdup(entry->d_name);
        size_t len = strlen(entry->d_name);
        if (len > max_len) max_len = len;
    }
    closedir(dp);

    qsort(filenames, count, sizeof(char *), cmpstr);

    struct winsize w;
    int term_width = 80;
    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    int spacing = 2;
    int col_width = max_len + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            int index = row + col * num_rows;
            if (index >= (int)count) break;

            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, filenames[index]);
            struct stat file_stat;
            if (lstat(full_path, &file_stat) == 0) {
                const char *color = get_file_color(filenames[index], file_stat.st_mode);
                printf("%s%-*s%s", color, col_width, filenames[index], COLOR_RESET);
            }
        }
        printf("\n");
    }

    // Recursive step for -R
    if (recursive_flag) {
        for (size_t i = 0; i < count; i++) {
            char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", dir, filenames[i]);
            struct stat st;
            if (lstat(subpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (strcmp(filenames[i], ".") == 0 || strcmp(filenames[i], "..") == 0)
                    continue;
                printf("\n");
                do_ls(subpath);
            }
        }
    }

    for (size_t i = 0; i < count; i++) free(filenames[i]);
    free(filenames);
}

// ============================================================================
// LONG LISTING MODE (with recursion support)
// ============================================================================
void do_ls_l(const char *dir) {
    printf("%s:\n", dir);
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (!dp) { fprintf(stderr, "Cannot open directory: %s\n", dir); return; }

    size_t count = 0, capacity = 64;
    char **filenames = malloc(capacity * sizeof(char *));
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (count >= capacity) { capacity *= 2; filenames = realloc(filenames, capacity * sizeof(char *)); }
        filenames[count++] = strdup(entry->d_name);
    }
    closedir(dp);
    qsort(filenames, count, sizeof(char *), cmpstr);

    for (size_t i = 0; i < count; i++) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, filenames[i]);
        print_file_stat(full_path);
    }

    // Recursive step for -R
    if (recursive_flag) {
        for (size_t i = 0; i < count; i++) {
            char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", dir, filenames[i]);
            struct stat st;
            if (lstat(subpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (strcmp(filenames[i], ".") == 0 || strcmp(filenames[i], "..") == 0)
                    continue;
                printf("\n");
                do_ls_l(subpath);
            }
        }
    }

    for (size_t i = 0; i < count; i++) free(filenames[i]);
    free(filenames);
}

// ============================================================================
// HORIZONTAL MODE (with recursion support)
// ============================================================================
void do_ls_x(const char *dirname) {
    printf("%s:\n", dirname);
    DIR *dir = opendir(dirname);
    if (!dir) { perror("opendir"); return; }

    struct dirent *entry;
    char *filenames[1000];
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') filenames[count++] = strdup(entry->d_name);
    }
    closedir(dir);

    qsort(filenames, count, sizeof(char *), cmpstr);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col;
    int max_len = 0;
    for (int i = 0; i < count; i++)
        if ((int)strlen(filenames[i]) > max_len) max_len = strlen(filenames[i]);

    int col_width = max_len + 2, current_width = 0;

    for (int i = 0; i < count; i++) {
        if (current_width + col_width > term_width) {
            printf("\n");
            current_width = 0;
        }
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirname, filenames[i]);
        struct stat st;
        if (lstat(full_path, &st) == 0) {
            const char *color = get_file_color(filenames[i], st.st_mode);
            printf("%s%-*s%s", color, col_width, filenames[i], COLOR_RESET);
        }
        current_width += col_width;
    }
    printf("\n");

    // Recursive step for -R
    if (recursive_flag) {
        for (int i = 0; i < count; i++) {
            char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", dirname, filenames[i]);
            struct stat st;
            if (lstat(subpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (strcmp(filenames[i], ".") == 0 || strcmp(filenames[i], "..") == 0)
                    continue;
                printf("\n");
                do_ls_x(subpath);
            }
        }
    }

    for (int i = 0; i < count; i++) free(filenames[i]);
}