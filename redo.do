# let's for sanity's sake assume that source files
# do not contain any whitespace or other funky characters
OBJ=$(find src/ -name '*.c' -printf "%P\n" | sed -e 's/.c$/.o/')

redo-ifchange ld $OBJ

./ld -o $3 $OBJ
