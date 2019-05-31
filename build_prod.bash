#!/usr/bin/env bash
output=./thi
olvl=-O3
compiler=clang
src=./src/*.c
flags=-DNDEBUG\ -Wno-switch
std=c99
$compiler $src -std=$std $olvl $flags -o $output
