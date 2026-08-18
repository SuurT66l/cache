# Cache

A command-line, in-memory key-value store built from scratch in C — no
external libraries, just the standard library and a hand-written hash table.
`cache` manages up to 20 independent named tables, each holding up to 10,000
key-value pairs, through a small REPL (read-eval-print loop) driven entirely
by stdin/stdout.

This project was written to get hash tables, collision resolution, and
manual memory management right at a low level: every byte allocated with
`malloc`/`calloc`/`strdup` is tracked to a single matching `free`, verified
with Valgrind (see [Memory Management](#memory-management) and
[Testing](#testing)).

## Highlights

- **Hash table from first principles** — direct-indexed slot array plus a
  separate-chaining overflow structure for collisions, hashed with djb2.
  See [Architecture](#architecture).
- **Zero memory leaks, verified, not assumed** — every allocation has a
  traced owner and deallocation path; a full Valgrind run reports `0 bytes
  in 0 blocks` at exit. See [Memory Management](#memory-management).
- **Defensive, no-crash CLI** — malformed input, unknown commands, missing
  tables/keys, capacity limits, and EOF are all handled explicitly with
  clear user-facing messages instead of undefined behavior.
- **Strict compiler discipline** — builds warning-free under
  `-Wall -Wextra -Werror`.
- **Fast average-case operations** — insert, lookup, and delete all resolve
  in constant time for the common (non-colliding) case.

## Table of Contents

- [Highlights](#highlights)
- [Building](#building)
- [Running](#running)
- [Commands](#commands)
  - [HELP](#help)
  - [CREATE](#create-table)
  - [LIST](#list)
  - [ADD](#add-table-key-value)
  - [GET](#get-table-key)
  - [COUNT](#count-table)
  - [DELETE](#delete-table-key)
  - [DROP](#drop-table)
  - [EXIT](#exit)
- [Error Handling](#error-handling)
- [Limits](#limits)
- [Architecture](#architecture)
  - [Data structures](#data-structures)
  - [Hashing](#hashing)
  - [Collision resolution](#collision-resolution)
  - [Table lifecycle](#table-lifecycle)
  - [Command parsing](#command-parsing)
- [Memory Management](#memory-management)
- [Project Layout](#project-layout)
- [Testing](#testing)
  - [Manual smoke test](#manual-smoke-test)
  - [Valgrind](#valgrind)
- [Known Limitations](#known-limitations)
- [Educational Purpose](#educational-purpose)
- [Author](#author)

`cache` behaves like a tiny, disposable Redis: it never touches disk, keeps
everything in RAM, and forgets everything as soon as it exits. Data lives
only for the lifetime of the process.

## Building

The project ships with a `Makefile` that requires nothing beyond `gcc` and
`make`:

```sh
$ make
```

This compiles every `.c` file in the directory in a single `gcc` invocation
with strict warning flags:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -Werror -I$(SRC_DIR)
```

`-Wall -Wextra -Werror` means the build treats warnings as errors — the code
compiles cleanly with zero warnings. Because the build produces the `cache`
binary directly from source files without leaving intermediate `.o` files
behind, there is nothing left over to clean up; running `make clean` (or
just re-running `make`) is enough to remove the compiled binary if needed:

```sh
$ make clean   # removes the `cache` binary
```

## Running

Start the program with no arguments:

```sh
$ ./cache
Welcome to Cache.
Type HELP to see the usage.
Type EXIT to exit.

cache>
```

You'll get a `cache>` prompt. Type a command and press Enter. Commands are
whitespace-separated and case-sensitive (commands must be UPPERCASE, e.g.
`CREATE`, not `create`).

You can also pipe a script of commands into it non-interactively:

```sh
$ printf 'CREATE users\nADD users alice 30\nGET users alice\nEXIT\n' | ./cache
```

## Commands

### HELP

Prints the list of available commands.

```
cache> HELP
Available commands:

HELP - HELP
LIST - LIST
CREATE - CREATE <table>
ADD - ADD <table> <key> <value>
COUNT - COUNT <table>
GET - GET <table> <key>
DELETE - DELETE <table> <key>
DROP - DROP <table>
EXIT - EXIT
```

### CREATE \<table\>

Creates a new, empty hash table with the given name. Table names must be
unique across the whole program (not per-table — globally unique).

```
cache> CREATE table1
TABLE table1 created

cache> CREATE table1
TABLE table1 already exists
```

If 20 tables already exist, table creation is refused:

```
cache> CREATE table21
Cannot create more than 20 tables
```

### LIST

Lists every table that currently exists, one name per line.

```
cache> LIST
table1
table2
```

If no tables exist:

```
cache> LIST
No tables exist
```

### ADD \<table\> \<key\> \<value\>

Inserts a key-value pair into the given table. If the key already exists in
that table, its value is overwritten (an "upsert") rather than creating a
duplicate entry.

```
cache> ADD table1 key1 value1
VALUE value1 at KEY key1 added to TABLE table1

cache> ADD table1 key1 value2
VALUE value2 at KEY key1 added to TABLE table1
```

If the table doesn't exist:

```
cache> ADD nosuch key1 value1
TABLE nosuch doesn't exist
```

If the table already holds 10,000 entries:

```
cache> ADD table1 key10001 value
Table is full
```

### GET \<table\> \<key\>

Looks up a key in the given table and prints its value.

```
cache> GET table1 key1
VALUE: value2
```

If the key isn't present:

```
cache> GET table1 nosuchkey
KEY nosuchkey doesn't exist in TABLE table1
```

If the table doesn't exist:

```
cache> GET nosuch key1
TABLE nosuch doesn't exist
```

### COUNT \<table\>

Prints how many key-value pairs are currently stored in the given table.

```
cache> COUNT table1
Item count in TABLE table1: 1
```

If the table doesn't exist:

```
cache> COUNT nosuch
TABLE nosuch doesn't exist
```

### DELETE \<table\> \<key\>

Removes a single key-value pair from the given table.

```
cache> DELETE table1 key1
KEY key1 deleted from TABLE table1
```

If the key isn't present:

```
cache> DELETE table1 key1
KEY key1 doesn't exist in TABLE table1
```

If the table doesn't exist:

```
cache> DELETE nosuch key1
TABLE nosuch doesn't exist
```

### DROP \<table\>

Deletes an entire table and every entry inside it.

```
cache> DROP table1
TABLE table1 deleted
```

If the table doesn't exist:

```
cache> DROP nosuch
TABLE nosuch doesn't exist
```

### EXIT

Exits the program cleanly, freeing all remaining memory first.

```
cache> EXIT
Exiting...
```

## Error Handling

The parser distinguishes between two kinds of mistakes:

1. **Unknown command** — if the first word typed isn't one of `HELP`,
   `LIST`, `CREATE`, `ADD`, `COUNT`, `GET`, `DELETE`, `DROP`, or `EXIT`, the
   program prints:

   ```
   cache> EXITT
   Error: invalid command
   ```

2. **Wrong number of arguments** — if a *valid* command is given the wrong
   number of arguments, the program prints that command's correct usage
   instead of guessing what was meant:

   ```
   cache> CREATE t1 t2
   Usage: CREATE <table>

   cache> GET table1
   Usage: GET <table> <key>
   ```

Blank lines (just pressing Enter) are silently ignored and simply redraw the
prompt — they are not treated as invalid commands.

## Limits

| Limit                        | Value  |
|-------------------------------|--------|
| Maximum number of tables      | 20     |
| Maximum entries per table     | 10,000 |

Both limits are enforced with clear, user-visible messages rather than
silently failing or crashing:

```
cache> CREATE table21
Cannot create more than 20 tables

cache> ADD table1 key10001 value
Table is full
```

## Architecture

### Data structures

The program is split into two translation units:

- **`main.c`** — owns the REPL loop and command-line tokenizing/dispatch. It
  knows nothing about how hash tables are implemented internally; it only
  calls into the public API declared in `hash_table.h`.
- **`hash_table.c` / `hash_table.h`** — implements the hash table and the
  "collection" of tables.

Three structs model the whole system:

```c
typedef struct ht_item {
    char* key;
    char* value;
} ht_item;

typedef struct linked_list {
    ht_item* item;
    struct linked_list* next;
} linked_list;

typedef struct ht_hash_table {
    char* name;
    int size;              // capacity (10000 slots)
    int count;              // number of entries currently stored
    ht_item** items;        // direct-indexed slot array
    linked_list** buckets;  // per-slot overflow chain for collisions
} ht_hash_table;

typedef struct ht_collection {
    int size;               // capacity (20 tables)
    int count;              // number of tables currently alive
    ht_hash_table** tables;
} ht_collection;
```

A single global `ht_collection* collection` holds every table that currently
exists. Each `ht_hash_table` owns two parallel arrays of length `TABLE_SIZE`
(10,000):

- `items[]` — holds the *first* entry that hashed to a given slot, indexed
  directly by hash value, so the common case (no collision) is a single
  array lookup with no pointer chasing.
- `buckets[]` — holds a singly linked list of any *additional* entries that
  hashed to the same slot (a collision chain), so lookups only need to walk
  a list when a collision has actually occurred.

### Hashing

Keys are hashed with **djb2**, a well-known, fast string-hashing algorithm
(originally by Daniel J. Bernstein):

```c
int get_hash(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;   // hash * 33 + c
    }

    return (int)(hash % TABLE_SIZE);
}
```

Starting from the prime seed `5381` and using the `hash * 33 + c` mixing
step gives djb2 good avalanche behavior and a low collision rate for
typical short string keys, while being cheap enough to compute (a shift and
an add per character — no multiplication instruction needed) that hashing
never becomes a bottleneck.

### Collision resolution

Collisions (two different keys hashing to the same slot) are resolved with
**separate chaining**:

- The first key to land on a slot is stored directly in `items[index]`.
- Any subsequent key that hashes to the same `index` is appended to the
  singly linked list at `buckets[index]` instead of overwriting the
  existing entry.
- Lookups (`GET`), inserts (`ADD`), and deletes (`DELETE`) all check
  `items[index]` first (O(1), no chain walk) and only fall through to
  walking `buckets[index]` if the direct slot doesn't match the key.
- On `ADD` with a colliding key, `update_bucket_if_duplicated` first checks
  whether the key already exists somewhere in the chain (to upsert its
  value) before appending a brand-new node.
- On `DELETE` of the key currently occupying `items[index]` when a
  collision chain exists, the head of the chain is promoted to take that
  slot's place, so future lookups for chained keys keep working in O(1) for
  the common case.

### Table lifecycle

- **`CREATE`** allocates a new `ht_hash_table`, including its two
  10,000-entry arrays (`calloc`'d, so unused slots start as `NULL`/zeroed),
  and stores it in the first free slot of `collection->tables`.
- **`DROP`** walks every slot in the table, freeing any direct item and any
  collision chain, then frees the table's arrays, its name, and the table
  struct itself. It then shifts every table after the dropped one down by
  one slot (`move_tables_up`) so the collection never has "holes" between
  index `0` and `collection->count - 1`, keeping table iteration
  straightforward.
- **`EXIT`** (or `EOF`) triggers `delete_collection()`, which repeats the
  same per-table cleanup as `DROP` for every table still alive, then frees
  the `tables` array and the collection struct itself, guaranteeing nothing
  is left allocated when the process ends.

### Command parsing

`main.c`'s `process_command` duplicates the input line with `strdup` (so
`strtok` — which mutates its input — never touches the caller's buffer),
splits it on spaces, and matches the first token case-sensitively against a
fixed table of known command names. Based on how many tokens were parsed,
it either:

- calls the matching hash-table function with the right arguments, or
- prints that command's `Usage: ...` line if the argument count is wrong, or
- prints `Error: invalid command` if the first token didn't match any known
  command at all.

The duplicated line (`temp`) is always freed before `process_command`
returns, on every code path, including the early-return paths for empty
input and invalid commands.

## Memory Management

Every allocation in the program has a single, well-defined owner and a
matching deallocation path:

| Allocated                             | Freed by                              |
|----------------------------------------|----------------------------------------|
| `collection`, `collection->tables`     | `delete_collection` (on `EXIT`/EOF)     |
| `ht_hash_table`, its `name`, `items[]`, `buckets[]` | `drop_table` (on `DROP`) or `delete_collection` (on exit) |
| `ht_item` (`key` + `value` + struct)   | `free_item`, called wherever an entry is removed, overwritten, or its table is torn down |
| `linked_list` nodes (collision chains) | `free_linkedlist`, called per-chain during table teardown, or per-node during targeted `DELETE` |
| `temp` (duplicated input line)         | freed at the end of every `process_command` code path |

No allocation is ever leaked, double-freed, or left dangling — dropping a
table clears its slot in the collection so no later `LIST`, `GET`, `ADD`,
`COUNT`, or `DELETE` can ever dereference freed memory. This is verified
directly with Valgrind (see [Testing](#testing) below).

## Project Layout

```
cache/
├── Makefile        # build rules (gcc, -Wall -Wextra -Werror)
├── main.c          # REPL loop, input tokenizing, command dispatch
├── hash_table.h     # public struct/function declarations
├── hash_table.c     # hash table + collection implementation
└── README.md        # this file
```

## Testing

### Manual smoke test

After `make`, pipe a scripted session in and diff it against the expected
transcript from the spec:

```sh
$ printf 'HELP\nCREATE table1\nCREATE table1\nADD table1 key1 value1\nADD table1 key1 value2\nGET table1 key1\nCOUNT table1\nLIST\nDELETE table1 key1\nGET table1 key1\nDELETE table1 key1\nDROP table1\nLIST\nCOUNT table1\nADD table1 key1 value1\nDELETE table1 key1\nGET table1 key1\nDROP table1\nEXIT\n' | ./cache
```

You can also just run `./cache` interactively and type commands at the
`cache>` prompt, or exercise EOF handling directly:

```sh
$ ./cache
Welcome to Cache.
Type HELP to see the usage.
Type EXIT to exit.

cache> ^D

EOF received. Exiting...
```

(`^D` is Ctrl-D on Linux/macOS, which sends EOF on stdin.)

### Valgrind

To confirm there are no leaks, invalid reads/writes, or double-frees, run
the program under Valgrind with full leak checking, feeding it a session
that creates tables, adds/overwrites/deletes keys (including keys that
collide), drops tables, and exits both via `EXIT` and via EOF:

```sh
$ valgrind --leak-check=full --show-leak-kinds=all ./cache < session.txt
```

A clean run ends with:

```
==NNNNN== HEAP SUMMARY:
==NNNNN==     in use at exit: 0 bytes in 0 blocks
==NNNNN==   total heap usage: ... allocs, ... frees, ... bytes allocated
==NNNNN==
==NNNNN== All heap blocks were freed -- no leaks are possible
```

On macOS, the built-in `leaks` tool can be used the same way:

```sh
$ leaks --atExit -- ./cache < session.txt
```

## Known Limitations

- Table and key/value data is kept entirely in memory; nothing is persisted
  to disk, and all data is lost when the process exits.
- Input lines are capped at 1023 characters (plus the null terminator);
  longer lines will be truncated by `fgets`.
- Commands and table/key names are matched case-sensitively — `create` and
  `Create` are not recognized as `CREATE`.

## Educational Purpose

This project was created for educational purposes, as part of learning
low-level systems programming and data structures in C.

## Author

- K real