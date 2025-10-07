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


General logic for "down then across" columnar format

When printing filenames in a grid, the goal is to fill the screen **top‑to‑bottom first, then left‑to‑right**.  

- A **simple single loop** (`for (i=0; i<count; i++)`) only prints one filename per line, producing a single vertical list.  
- To achieve a grid, we must think in terms of **rows and columns**:
  - First calculate how many columns fit on the screen.
  - Then calculate how many rows are needed to display all items.
  - For each row, print the element at index:

    ```
    index = row + col * num_rows
    ```

    where `row` is the current row number and `col` is the current column number.

- This ensures the first column is filled top‑to‑bottom, then the second column, and so on.

**Example (10 files, 3 columns):**
This requires **nested loops** (rows × columns) and index arithmetic, not just a single linear loop.

---

## Question 2: Purpose of the `ioctl` system call

- The `ioctl` system call with the `TIOCGWINSZ` request queries the terminal driver for the **current window size** (rows and columns).  
- In this program, we specifically use `w.ws_col` (the number of character columns) to decide how many filenames can fit side by side.  
- Without this, the program would have no way of knowing the actual width of the user’s terminal.

---

## Question 3: Limitations of using only a fixed‑width fallback

If the program always assumes a fixed width (e.g., 80 characters):

- On **narrower terminals** (e.g., 60 columns), filenames will wrap awkwardly or overflow, breaking alignment.  
- On **wider terminals** (e.g., 150 columns), the program will under‑utilize the space, showing fewer columns than possible and wasting screen real estate.  
- The output will feel rigid and less user‑friendly, because it won’t adapt to the user’s environment.

By using `ioctl`, the program becomes **responsive**: it adapts the number of columns dynamically to the actual terminal size, just like the real `ls` command.