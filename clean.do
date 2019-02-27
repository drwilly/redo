set -e

rm -f -- *.prereqs
rm -f -- *.d *.o
rm -f -- cc
xargs -d '\n' -a env/PROGS rm -f --
