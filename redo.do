IFS=$(printf '\n\t')
OBJ=$(sed -e '/^$/d' <<-EOF
	checksum.o
	environment.o
	options.o
	path.o
	predeps.o
	reporting.o
	tmpfile.o
	$2.o
EOF
)

redo-ifchange ld $OBJ

./ld -o "$3" $OBJ
