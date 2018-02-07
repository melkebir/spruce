#!/bin/bash
for o in {0..35}
do
    f="S${o}.pbs"
    echo "#PBS -l nodes=1:ppn=32" > $f
    #echo "#PBS -l walltime=01:30:00" >> $f
    echo "#PBS -l walltime=00:05:00" >> $f
    echo "#PBS -N benchmark_S${o}" >> $f
    echo "#PBS -e benchmark_S${o}.err" >> $f
    echo "#PBS -o benchmark_S${o}.out" >> $f

    d="~/src/spruce_local"
    echo "cd ~/scratch/spruce" >> $f
    for threads in {1,2,4,8,16}
    do
        echo "(aprun -n 1 -d $threads $d/build/enumerate $d/data/ncsa/m4_n50_k4_C200_s13_0.7.input.tsv -clique $d/data/ncsa/cliques/m4_n50_k4_C200_s13_0.7.cliques -logFile t${threads} -logInterval 10 -ll 100 -r 0 -o $o -t $threads -s 1 > t${threads}_S${o}.sol 2> /dev/null) &" >> $f
    done
    echo "wait" >> $f
    echo "mv ~/scratch/spruce/*_S${o}.* $d/result/ncsa/" >> $f
done
