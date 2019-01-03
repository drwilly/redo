: ${PREFIX:="$HOME"}

: ${CC:="clang"}
: ${CPPFLAGS:="-I'\$PREFIX/include/'"}
: ${CFLAGS:="-g -O0 -Wall -pedantic"}
: ${LDFLAGS:="-L'\$PREFIX/lib/' -Wl,-rpath='\$ORIGIN/../lib/'"}
