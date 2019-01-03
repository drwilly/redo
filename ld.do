set -e

redo-ifchange config.sh

. ./config.sh

chmod +x "$3"

LDLIBS="-lskarnet"

: ${CC?must be set}
: ${LDFLAGS?must be set}
: ${LDLIBS?must be set}

tee <<-EOF
	#!/bin/sh -e
	exec $CC $LDFLAGS "\$@" $LDLIBS
EOF
