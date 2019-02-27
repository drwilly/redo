set -e
exec >&2

xargs -d '\n' -a env/PROGS redo-ifchange config.sh

. ./config.sh

: ${DESTDIR?}
: ${PREFIX?}

mkdir -p "${DESTDIR}/${PREFIX}/bin/"
xargs -d '\n' -a env/PROGS cp -v -u -t "${DESTDIR}/${PREFIX}/bin/"
