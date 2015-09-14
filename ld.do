redo-ifchange config.sh

. ./config.sh

chmod +x $3

cat <<-EOF
	#!/bin/sh
	exec $CC $LDFLAGS \$@ $LIB $LIBPATH
EOF
