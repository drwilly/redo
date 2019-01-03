set -e

redo-ifchange config.sh

. ./config.sh

CPPFLAGS="$CPPFLAGS -I src/"
CFLAGS="$CFLAGS -std=c11 -D_POSIX_C_SOURCE=200809L"

chmod +x "$3"

: ${CC?must be set}
: ${CPPFLAGS?must be set}
: ${CFLAGS?must be set}

tee <<-EOF
	#!/bin/sh -e
	exec $CC $CPPFLAGS -c $CFLAGS "\$@"
EOF
