redo-ifchange config.sh

chmod +x $3

. ./config.sh

cat <<-EOF
	#!/bin/sh
	exec $CC $CFLAGS $LIB $LIBPATH \$@
EOF
