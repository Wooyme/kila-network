#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "Illegal number of parameters"
    exit 1
fi
filename=$1
size=$2
uuid=$(uuidgen)
echo $filename:$uuid
sqlite3 kila.sqlite "insert into files (uuid,path,size,type) VALUES ('$uuid','$filename','$size',0);"

