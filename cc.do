set -e

redo-ifchange config.sh

. ./config.sh

CPPFLAGS="$CPPFLAGS -I src/"
CFLAGS="$CFLAGS -std=c11 -D_POSIX_C_SOURCE=200809L"

chmod +x "$3"

tee <<-EOF
	#!/bin/sh -e
	exec ${CC:?} ${CPPFLAGS?} ${CFLAGS?} ${LDFLAGS?} "\$@" ${LDLIBS?}
EOF
