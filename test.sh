#!/bin/bash

for i in $(seq $1 $2); do
    echo -n " aaa$i/" >/proc/uservars/system/create_dir
    echo -n "   //aaa$i/bbb$i   // " >/proc/uservars/system/create_var
    echo "vau$i" >/proc/uservars/aaa$i/bbb$i
done
