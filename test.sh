#!/bin/bash

for i in $(seq $1 $2); do
    echo -n " aaa$i/" >/proc/uservars/command/create_dir
    echo -n "   //aaa$i/bbb$i   // " >/proc/uservars/command/create_var
    echo "vau$i" >/proc/uservars/aaa$i/bbb$i
    echo -n "aaa$i/ddd$i" >/proc/uservars/command/create_var
    echo "brrr$i" >/proc/uservars/aaa$i/ddd$i
    echo -n "aaa$i/ddd$i" >/proc/uservars/command/delete
done
