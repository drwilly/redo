set -e

redo-ifchange config.sh

. ./config.sh

chmod +x "$3"

LIB="-lskarnet"

tee <<-EOF
	#!/bin/sh -e
	exec $CC $LDFLAGS "\$@" $LIBPATH $LIB
EOF
