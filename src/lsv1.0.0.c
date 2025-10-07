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
void do_ls_l(const char *dir) ;
void do_ls_x(const char *dir) ;
// Display mode flags
enum DisplayMode { DEFAULT, LONG, HORIZONTAL };

// ============================================================================
// COLOR MACROS - ANSI escape codes for terminal colors
// ============================================================================

// Reset to default terminal color
#define COLOR_RESET   "\033[0m"
// Regular colors
#define COLOR_BLUE    "\033[0;34m"   // Directories
#define COLOR_GREEN   "\033[0;32m"   // Executable files  
#define COLOR_RED     "\033[0;31m"   // Archive files
#define COLOR_PINK    "\033[0;35m"   // Symbolic links
#define COLOR_CYAN    "\033[0;36m"   // Special files

// ============================================================================
// COLOR DETERMINATION FUNCTION
// ============================================================================

// Function to determine and return the appropriate color code for a file
const char* get_file_color(const char *filename, mode_t mode) {
    // Check if it's a directory
    if (S_ISDIR(mode)) {
        return COLOR_BLUE;
    }
    
    // Check if it's a symbolic link
    if (S_ISLNK(mode)) {
        return COLOR_PINK;
    }
    
    // Check if it's a special file
    if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISSOCK(mode) || S_ISFIFO(mode)) {
        return COLOR_CYAN;
    }
    
    // Check if it's a regular file with execute permissions
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        return COLOR_GREEN;
    }
    
    // Check for archive files by extension
    if (S_ISREG(mode)) {
        const char *ext = strrchr(filename, '.');
        if (ext != NULL) {
            if (strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0 || 
                strcmp(ext, ".zip") == 0 || strcmp(ext, ".bz2") == 0 ||
                strcmp(ext, ".xz") == 0 || strcmp(ext, ".tgz") == 0) {
                return COLOR_RED;
            }
        }
    }
    
    // Default: no color
    return COLOR_RESET;
}

int main(int argc, char *argv[]) {
    int opt;
    int display_mode = DEFAULT; // Default display mode

    // Parse command-line options
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                display_mode = LONG;
                break;
            case 'x':
                display_mode = HORIZONTAL;
                break;
            default:
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    // If no directory is given, use current directory
    if (optind == argc) {
        if (display_mode == LONG)
            do_ls_l(".");
        else if (display_mode == HORIZONTAL)
            do_ls_x(".");
        else
            do_ls(".");
    } else {
        // Loop through each directory argument
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            if (display_mode == LONG)
                do_ls_l(argv[i]);
            else if (display_mode == HORIZONTAL)
                do_ls_x(argv[i]);
            else
                do_ls(argv[i]);
            puts(""); // Add spacing between listings
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

    // Extract just the filename from the full path for display
    const char *display_name = strrchr(filename, '/');
    if (display_name == NULL) {
        display_name = filename;
    } else {
        display_name++;
    }

    // Get the appropriate color for this file type
    const char *color_code = get_file_color(display_name, fileStat.st_mode);

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

    // Modified to include color in filename output
    printf("%c%s %ld %-8s %-8s %8lld %s %s%s%s\n",
           type, str, fileStat.st_nlink, user, group,
           (long long)fileStat.st_size, timebuf, color_code, display_name, COLOR_RESET);
}
 
int cmpstr(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

void do_ls(const char *dir) {
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (!dp) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    // --- Step 1: Gather filenames ---
    size_t count = 0, capacity = 64;
    char **filenames = malloc(capacity * sizeof(char *));
    if (!filenames) { perror("malloc"); closedir(dp); return; }

    size_t max_len = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue; // skip hidden

        if (count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(filenames, capacity * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                for (size_t i = 0; i < count; i++) free(filenames[i]);
                free(filenames);
                closedir(dp);
                return;
            }
            filenames = tmp;
        }

        filenames[count] = strdup(entry->d_name);
        if (!filenames[count]) {
            perror("strdup");
            for (size_t i = 0; i < count; i++) free(filenames[i]);
            free(filenames);
            closedir(dp);
            return;
        }

        size_t len = strlen(entry->d_name);
        if (len > max_len) max_len = len;
        count++;
    }
    closedir(dp);

    if (count == 0) { free(filenames); return; }

    // --- Step 2: Sort filenames ---
    qsort(filenames, count, sizeof(char *), cmpstr);

    // --- Step 3: Get terminal width ---
    struct winsize w;
    int term_width = 80;
    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    // --- Step 4: Calculate layout ---
    int spacing = 2;
    int col_width = max_len + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    // --- Step 5: Print down-then-across ---
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            int index = row + col * num_rows;
            if (index >= (int)count) break;
            
            // Get file info for color determination
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, filenames[index]);
            struct stat file_stat;
            if (lstat(full_path, &file_stat) == 0) {
                const char *color_code = get_file_color(filenames[index], file_stat.st_mode);
                printf("%s%-*s%s", color_code, col_width, filenames[index], COLOR_RESET);
            } else {
                printf("%-*s", col_width, filenames[index]);
            }
        }
        printf("\n");
    }

    // --- Step 6: Cleanup ---
    for (size_t i = 0; i < count; i++) free(filenames[i]);
    free(filenames);
}

void do_ls_l(const char *dir) {
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    // --- STEP 1: GATHER FILENAMES INTO DYNAMIC ARRAY ---
    size_t count = 0;        // Number of files found
    size_t capacity = 64;    // Initial array size
    char **filenames = malloc(capacity * sizeof(char *));  // Allocate array of string pointers
    if (!filenames) {
        perror("malloc");
        closedir(dp);
        return;
    }

    // Read all directory entries
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')  // Skip hidden files
            continue;

        // Resize array if full (dynamic array growth)
        if (count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(filenames, capacity * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                // Cleanup on error: free all allocated strings
                for (size_t i = 0; i < count; i++) free(filenames[i]);
                free(filenames);
                closedir(dp);
                return;
            }
            filenames = tmp;
        }

        // Duplicate filename string (we need our own copy)
        filenames[count] = strdup(entry->d_name);
        if (!filenames[count]) {
            perror("strdup");
            for (size_t i = 0; i < count; i++) free(filenames[i]);
            free(filenames);
            closedir(dp);
            return;
        }
        count++;
    }
    closedir(dp);  // Close directory when done reading

    if (count == 0) {
        free(filenames);
        return;
    }

    // --- STEP 2: SORT FILENAMES ALPHABETICALLY ---
    qsort(filenames, count, sizeof(char *), cmpstr);

    // --- STEP 3: PRINT FILES WITH DETAILED INFORMATION ---
    for (size_t i = 0; i < count; i++) {
        char full_path[1024];
        // Create full path by combining directory and filename
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, filenames[i]);
        print_file_stat(full_path);  // Print file information (now includes color)
        
        free(filenames[i]);  // Free the duplicated string
    }

    free(filenames);  // Free the array itself
}

void do_ls_x(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (!dir) {
        perror("opendir");
        return;
    }

    // Step 1: Read filenames into array
    struct dirent *entry;
    char *filenames[1000]; // Max 1000 entries
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            filenames[count++] = strdup(entry->d_name);
        }
    }
    closedir(dir);

    // ✅ Step 2: Sort filenames alphabetically
    qsort(filenames, count, sizeof(char *), cmpstr);

    // Step 3: Get terminal width
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col;

    // Step 4: Find longest filename
    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len) max_len = len;
    }

    // Step 5: Calculate column width
    int col_width = max_len + 2;
    int current_width = 0;

    // Step 6: Print filenames horizontally
    for (int i = 0; i < count; i++) {
        if (current_width + col_width > term_width) {
            printf("\n");
            current_width = 0;
        }
        
        // Get file info for color determination
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirname, filenames[i]);
        struct stat file_stat;
        if (lstat(full_path, &file_stat) == 0) {
            const char *color_code = get_file_color(filenames[i], file_stat.st_mode);
            printf("%s%-*s%s", color_code, col_width, filenames[i], COLOR_RESET);
        } else {
            printf("%-*s", col_width, filenames[i]);
        }
        
        current_width += col_width;
    }
    printf("\n");

    // Step 7: Free memory
    for (int i = 0; i < count; i++) {
        free(filenames[i]);
    }
}