#!/bin/sh

if [ ! -f $1 ]; then
    echo $1 not found
    exit 1
fi

if [ -s $1 ]; then
    cat $1
    exit 1
fi

exit 0
