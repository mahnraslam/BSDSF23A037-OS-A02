# LS Command Report

## Table of Contents
1. [Crucial Difference Between `stat()` and `lstat()`](#question-1-crucial-difference-between-stat-and-lstat)
2. [Extracting File Type and Permissions Using `st_mode`](#question-2-extracting-file-type-and-permissions-using-st_mode)
3. [General Logic for "Down Then Across" Columnar Format](#question-3-general-logic-for-down-then-across-columnar-format)
4. [Purpose of `ioctl` and Limitations of Fixed-Width](#question-4-purpose-of-ioctl-and-limitations-of-fixed-width)
5. [Vertical vs Horizontal Printing Complexity](#question-5-vertical-vs-horizontal-printing-complexity)
6. [Strategy for Managing Display Modes (-l, -x, Default)](#question-6-strategy-for-managing-display-modes--l--x--default)

---

## Question 1: Crucial Difference Between `stat()` and `lstat()`

| Function  | Behavior with Symbolic Links           | Use Case in `ls`                  |
|-----------|---------------------------------------|----------------------------------|
| `stat()`  | Follows the link and gives info about the target | Not suitable for `ls -l` on symlinks |
| `lstat()` | Gives info about the link itself      | Use in `ls -l` to show symlink details |

**Example:**

```bash
ln -s /etc/passwd mylink

stat("mylink")   → info about /etc/passwd
lstat("mylink")  → info about mylink (the symlink)
```

**Explanation:**
- `stat()` follows symbolic links, reporting info about the target file.  
- `lstat()` reports info about the link itself.  
- In `ls -l`, `lstat()` is preferred to display symlink details.

---

## Question 2: Extracting File Type and Permissions Using `st_mode`

The `st_mode` field in `struct stat` contains **file type** and **permission bits**.

**File type extraction using bitwise AND:**

```c
if ((st.st_mode & S_IFMT) == S_IFDIR)  → directory
if ((st.st_mode & S_IFMT) == S_IFREG)  → regular file
if ((st.st_mode & S_IFMT) == S_IFLNK)  → symbolic link
```

**Permission extraction using bitwise AND:**

```c
// Owner permissions
if (st.st_mode & S_IRUSR) → read
if (st.st_mode & S_IWUSR) → write
if (st.st_mode & S_IXUSR) → execute

// Group permissions
if (st.st_mode & S_IRGRP) → read
if (st.st_mode & S_IWGRP) → write
if (st.st_mode & S_IXGRP) → execute

// Others permissions
if (st.st_mode & S_IROTH) → read
if (st.st_mode & S_IWOTH) → write
if (st.st_mode & S_IXOTH) → execute
```

> Bitwise AND (`&`) is used to **mask unwanted bits**, isolating file type or permission bits.

---

## Question 3: General Logic for "Down Then Across" Columnar Format

**Goal:** Fill the terminal **top-to-bottom first, then left-to-right**.

**Why a single loop is insufficient:**
- A linear loop prints filenames one by one, producing a vertical list.
- A grid requires calculating **rows and columns** and using **nested loops**.

**Implementation Logic:**

```c
index = row + col * num_rows;
```

- `row` → current row  
- `col` → current column  
- `num_rows` → total rows needed  

**Example (10 files, 3 columns):**

```
file1   file5   file9
file2   file6   file10
file3   file7
file4   file8
```

---

## Question 4: Purpose of `ioctl` and Limitations of Fixed-Width

- **Purpose:**  
  `ioctl` with `TIOCGWINSZ` fetches the terminal width (`w.ws_col`) to determine how many filenames can fit side by side.

- **Limitations of fixed-width fallback (e.g., 80 columns):**  
  - On **narrow terminals**, output may wrap awkwardly.  
  - On **wide terminals**, available space is underutilized.  
  - Output becomes **rigid**, not responsive to the terminal size.

---

## Question 5: Vertical vs Horizontal Printing Complexity

| Aspect  | Vertical ("Down then Across")         | Horizontal ("Across")        |
|---------|-------------------------------------|-----------------------------|
| Pre-calculation | Requires rows, columns, and index arithmetic | Only linear iteration needed |
| Complexity | Higher: nested loops and alignment logic | Lower: single loop works |
| Display | Top-to-bottom per column | Left-to-right per row |

> Vertical printing requires more **pre-calculation** to ensure proper column alignment.

---

## Question 6: Strategy for Managing Display Modes (-l, -x, Default)

- **Parsing command-line options:** Use `getopt` to detect `-l` or `-x`.  
- **Set flags:**  
  - `long_fmt` → `-l`  
  - `across_fmt` → `-x`  
- **Decide printing function:**  
  - `-l` → `do_ls_l()` (long listing)  
  - `-x` → `do_ls_x()` (horizontal)  
  - Default → `do_ls()` (columnar "down then across")

> This approach ensures **flexible and accurate output**, similar to the real `ls` command.

# Answers of task 5: Display Logic Questions

### 1. Compare the implementation complexity of the "down then across" (vertical) printing logic versus the "across" (horizontal) printing logic. Which one requires more pre-calculation and why?

- **Down-then-across (vertical)**:  
  - Requires calculating **number of rows and columns** based on terminal width and longest filename.  
  - Index for each printed element is `row + col * num_rows`.  
  - **More pre-calculation is needed** because rows must be computed to organize files vertically before printing.

- **Across (horizontal)**:  
  - Prints sequentially and wraps lines based on terminal width.  
  - Only needs terminal width to determine when to wrap; no row calculations required.  
  - **Less pre-calculation** compared to vertical layout.

**Conclusion:** Vertical ("down then across") printing is more complex because it requires extra calculations to map file indices correctly.

---

### 2. Describe the strategy you used in your code to manage the different display modes (-l, -x, and default). How did your program decide which function to call for printing?

- The program uses a **flag variable** (`long_fmt`) to determine the mode:  
  - `long_fmt = 1` → Long-listing (`-l`) → calls `do_ls_l()`.  
  - `long_fmt = 0` → Column display (default) → calls `do_ls()`.

- **Command-line parsing** is done using `getopt()`. The program checks options and sets the appropriate flags.

- After parsing, the program loops over directories and calls the correct display function based on the flag.  

- This strategy allows **easy extension** for other modes (e.g., horizontal `-x`) by adding flags and corresponding function calls.
- 
# Recursive Function Concepts in `ls`

## 1. What is a "base case" in recursion?

- A **base case** is a condition in a recursive function that stops further recursive calls.  
- Without a base case, recursion would continue indefinitely, eventually causing a **stack overflow** error.
- In the context of a **recursive `ls`**, the base case occurs when:
  - The function encounters a directory that has **no subdirectories** to explore, or
  - It reaches a **non-directory file**, meaning there is nothing further to list recursively.

In such cases, the function stops calling itself and returns to the previous level of recursion.

---

## 2. Why is it essential to construct a full path before making a recursive call?

- When listing directories recursively, you must build the **complete path** (e.g., `"parent_dir/subdir"`) before calling the recursive function.
- This ensures the recursive `do_ls()` function can **open and read the correct directory** on the filesystem.

### Example:
If you are currently inside `"parent_dir"` and find a subdirectory `"subdir"`, you must pass:
```c
do_ls("parent_dir/subdir");
