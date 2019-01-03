set -e

# Environment-variables affecting the build process
# should be read here and only here.
# If unset, defaults from config.def.sh are used.
# The resulting environment is stored in config.sh.

. ./config.def.sh

tee <<-EOF
	# config.sh contains all variables
	# used during the build process
	DESTDIR="$DESTDIR"
	PREFIX="$PREFIX"

	CC="$CC"
	CPPFLAGS="$CPPFLAGS"
	CFLAGS="$CFLAGS"
	LDFLAGS="$LDFLAGS"
EOF
