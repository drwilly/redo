redo-ifchange config.sh

. ./config.sh

#CFLAGS="$CFLAGS -D_"

INC="$INC -I src/"

chmod +x $3

cat <<-EOF
	#!/bin/sh
	exec $CC -c -std=c11 $CFLAGS $INC \$@
EOF
