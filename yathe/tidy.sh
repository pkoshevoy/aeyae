#!/bin/bash

for i in `find | grep '~'`; do 
    echo rm -f $i
    rm -f $i
done
