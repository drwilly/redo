set -e

IFS="$(printf '\n\t')"
OBJ=$(sed -e '/^$/d' <<-EOF
	checksum.o
	environment.o
	options.o
	path.o
	prereqs.o
	reporting.o
	stralloc_string.o
	$2.o
EOF
)

redo-ifchange ld $OBJ

./ld -o "$3" $OBJ
