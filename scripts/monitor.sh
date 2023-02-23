#!/usr/bin/env bash
set -euo pipefail
shopt -s inherit_errexit

kitbit="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." >/dev/null 2>&1 && pwd )"

if [[ -z "${KITBIT_PORT+x}" ]]; then
	echo "Error: set the KITBIT_PORT environment variable"
	exit 1
fi

arduino="$kitbit/bin/arduino-cli"
if [[ ! -x "$arduino" ]]; then
	echo "Error: bin/arduino-cli not found. Run 'make bin/arduino-cli' to download it."
	exit 1
fi

exec $arduino monitor -p $KITBIT_PORT
