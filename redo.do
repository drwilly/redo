OBJ=$(sed -e '/^$/d' <<-EOF
	environment.o
	path.o
	predeps.o
	reporting.o
	$2.o
EOF
)

redo-ifchange ld $OBJ

./ld -o $3 $OBJ
