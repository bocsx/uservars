# uservars
Provide a simple interface for linux programs/scripts to use (create/read/write/delete) /proc kernel variables.
## Usage
### Managing directories and variables:
- Create directory under /proc/uservars: echo -n directory_path >/proc/uservars/system/create_dir
- Delete directory under /proc/uservars: echo -n directory_path >/proc/uservars/system/delete_dir
- Create variable under /proc/uservars: echo -n variable_path >/proc/uservars/system/create_var
- Delete variable under /proc/uservars: echo -n variable_path >/proc/uservars/system/delete_var
### Using variables:
- Set a value for a variable: echo "Hello World" >/proc/uservars/myvars/world
- Read the value: cat /proc/uservars/myvars/world
