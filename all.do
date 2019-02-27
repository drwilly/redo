set -e

xargs -d '\n' -a env/PROGS redo-ifchange
