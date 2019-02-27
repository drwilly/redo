set -e

IFS="$(printf '\n\t')"

files() {
	cat env/SRCS
}
objects() {
	xargs -d '\n' -a env/SRCS basename -s .c | xargs printf '%s.o\n'
}

case ${BUILD_INCREMENTAL-false} in
false)  args() files;;
*)      args() objects;;
esac

redo-ifchange cc "src/$2.c" $(args)
./cc -o "$3" "src/$2.c" $(args)
