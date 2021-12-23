# uservars
Provide a simple interface for linux programs/scripts to use (create/read/write/delete) /proc kernel variables.
## Usage
- /proc/uservars/systemvars/create_dir - Create directory under /proc/uservars
- /proc/uservars/systemvars/delete_dir - Delete directory under /proc/uservars
- /proc/uservars/systemvars/create_var - Create variable under /proc/uservars
- /proc/uservars/systemvars/delete_var - Delete variable under /proc/uservars

## TODO list
- 1st use a linked list to keep track of the vars and dirs
- create_dir
- delete_dir
- create_var
- delete_var
- use variable size variables instead of a fix one
- use a tree instead of the linked list
- free all the allocated space on module removal
