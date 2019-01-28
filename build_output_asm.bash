#!/usr/bin/env bash
nasm -f macho64 -g output.asm -o output.o

flags=-lSystem #\ -lglew\ -lglfw3\ -framework\ OpenGL\ -framework\ CoreVideo\ -framework\ IOKit\ -framework\ Cocoa\ -framework\ Carbon\ -framework\ CoreFoundation

ld -macosx_version_min 10.14 -o output output.o -e _main $flags
./output
echo $?
rm ./output
