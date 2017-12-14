set -e

redo-ifchange cc

./cc -MD -MF "$2.d" -MT "" -o "$3" "src/$2.c"

exec < "$2.d"
rm -f -- "$2.d"
unset IFS
read DEPS
redo-ifchange ${DEPS#:}
