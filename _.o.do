redo-ifchange cc

trap "exit 1"          1 2 3 15
trap "rm -f -- '$2.d'" 0

./cc -MD -MF "$2.d" -MT "" -o "$3" "src/$2.c"

read DEPS < "$2.d"
redo-ifchange ${DEPS#:}
