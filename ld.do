redo-ifchange config.sh

. ./config.sh

chmod +x "$3"

LIB="-lskarnet"

cat <<-EOF
	#!/bin/sh
	exec $CC $LDFLAGS "\$@" $LIBPATH $LIB
EOF
