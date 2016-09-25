set -e

redo-ifchange config.sh

. ./config.sh

#CFLAGS="$CFLAGS -D_"

INC="$INC -I src/"

chmod +x "$3"

tee <<-EOF
	#!/bin/sh -e
	exec $CC -c -std=c11 $CFLAGS $INC "\$@"
EOF
