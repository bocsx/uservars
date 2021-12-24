# uservars
Provide a simple interface for linux programs/scripts to use (create/read/write/delete) /proc kernel variables.
## Usage
### Managing directories and variables:
- echo -n directory_path >/proc/uservars/system/create_dir - Create directory under /proc/uservars
- echo -n directory_path >/proc/uservars/system/delete_dir - Delete directory under /proc/uservars
- echo -n variable_path >/proc/uservars/system/create_var - Create variable under /proc/uservars
- echo -n variable_path >/proc/uservars/system/delete_var - Delete variable under /proc/uservars
### Using variables:
- Set a value for a variable: echo "Hello World" >/proc/uservars/myvars/world
- Read the value: cat /proc/uservars/myvars/world
## TODO list
- 1st use a linked list to keep track of the vars and dirs
- create_dir
- delete_dir
- create_var
- delete_var
- use variable size variables instead of a fix one
- use a tree instead of the linked list
- free all the allocated space on module removal
- review error handling (right error messages back to the user)
