set -e

redo-ifchange config.def.sh
tee < config.def.sh
