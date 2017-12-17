: ${PREFIX:="$HOME"}

CC="clang"
CFLAGS="-g -O0 -Wall -pedantic"

INC="-I'$PREFIX/include/'"
LIBPATH="-L'$PREFIX/lib/' -Wl,-rpath='\$ORIGIN/../lib/'"
