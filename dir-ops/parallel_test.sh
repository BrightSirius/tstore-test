#!/bin/bash
#if [ $# -lt 1 ]; then
#    echo "please specify logfile"
#    exit -1
#fi
#logfile

readonly path=/mnt/disp

for i in `seq 1 100`;do
    starttime=`date +'%Y-%m-%d %H:%M:%S'`
    ./p-write 8 $path
    if [ $? -ne 0 ]; then
        echo "$i iter write fails" 
        break
    else
        rm -f  ${path}/*
        if [ $? -ne 0 ]; then
            echo "$i iter rm fails"
            break
        fi
    fi
    endtime=`date +'%Y-%m-%d %H:%M:%S'`
    start_seconds=$(date --date="$starttime" +%s);
    end_seconds=$(date --date="$endtime" +%s);
    gap=$((end_seconds-start_seconds))
    #bindwidth=$((1024/$((end_seconds - start_seconds))))
    echo "$i iter succeeds with ${gap}s"
done
    

