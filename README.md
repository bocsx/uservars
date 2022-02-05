# uservars
Kernel module that provides a simple interface for linux programs/scripts to use (create/read/write/delete) /proc kernel variables.
## Usage
### Managing directories and variables:
- Create directory under /proc/uservars: echo -n directory_path >/proc/uservars/command/create_dir
- Create variable under /proc/uservars: echo -n variable_path >/proc/uservars/command/create_var
- Delete directory or variable under /proc/uservars: echo -n variable_path >/proc/uservars/command/delete
- List the currently existing variables: cat /proc/uservars/command/list
### Using variables:
- Set a value for a variable: echo "Hello World" >/proc/uservars/myvars/world
- Read the value: cat /proc/uservars/myvars/world
