set -e

redo-ifchange config.sh redo redo-ifchange redo-ifcreate

. ./config.sh

mkdir -p "${DESTDIR}/${PREFIX}/bin/"
cp -t "${DESTDIR}/${PREFIX}/bin/" redo redo-ifchange redo-ifcreate
