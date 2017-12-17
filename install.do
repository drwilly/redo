set -e
exec >&2

redo-ifchange config.sh redo redo-ifchange redo-ifcreate

. ./config.sh

mkdir -p "${DESTDIR}/${PREFIX}/bin/"
cp -v -u -t "${DESTDIR}/${PREFIX}/bin/" redo redo-ifchange redo-ifcreate
