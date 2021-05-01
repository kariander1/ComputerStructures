#!/bin/bash

FILES=./tests/*
for f in $FILES # Loop over all test files
do
    fb="$(basename -- $f)"
    f_wo_ext=$(echo "$fb" | cut -f 1 -d '.')

    echo $f
    `./bp_main $f >  ./ref_results/$fb.out`
    diff ./ref_results/$fb.out ./ref_results/$f_wo_ext.out
done

