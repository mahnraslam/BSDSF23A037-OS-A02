# `stat()` vs `lstat()`

| Function | Behavior with symbolic links                  | Use case in `ls`                    |
|----------|-----------------------------------------------|-------------------------------------|
| `stat()` | Follows the link and gives info about target  | Not suitable for `ls -l` on symlinks |
| `lstat()`| Gives info about the link itself              | Use in `ls -l` to show symlink details |

##  Example

```bash
ln -s /etc/passwd mylink

stat("mylink")   → info about /etc/passwd  
lstat("mylink")  → info about mylink (the symlink)
--2
if ((st.st_mode & S_IFMT) == S_IFDIR)  → directory
if ((st.st_mode & S_IFMT) == S_IFREG)  → regular file
if ((st.st_mode & S_IFMT) == S_IFLNK)  → symbolic link
if (st.st_mode & S_IRUSR) → owner read
if (st.st_mode & S_IWUSR) → owner write
if (st.st_mode & S_IXUSR) → owner execute

if (st.st_mode & S_IRGRP) → group read
if (st.st_mode & S_IWGRP) → group write
if (st.st_mode & S_IXGRP) → group execute

if (st.st_mode & S_IROTH) → others read
if (st.st_mode & S_IWOTH) → others write
if (st.st_mode & S_IXOTH) → others execute
