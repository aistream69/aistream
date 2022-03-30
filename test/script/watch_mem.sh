#!/bin/sh

pid=$1
echo "pid:" $oud
while true
do
    if [ -e /proc/$pid/status ]; then
        cat /proc/$pid/status | grep VmRSS
        free -h
    fi
    sleep 10
done

