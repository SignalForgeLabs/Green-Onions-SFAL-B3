#!/usr/bin/env bash
set -euo pipefail

echo "================================================================"
echo " Signal Forge Audio Labs — Green Onions Uninstaller (Linux)"
echo "================================================================"

# Remove user-level installation if present
rm -f  "${HOME}/.local/bin/green-onions"
rm -rf "${HOME}/.vst3/Green Onions.vst3"
rm -f  "${HOME}/.local/share/applications/green-onions.desktop"
rm -f  "${HOME}/.local/share/applications/green-onions-uninstall.desktop"
rm -f  "${HOME}/.local/bin/green-onions-uninstall"

# If installed system-wide via .deb package, offer dpkg/apt removal
if dpkg -s green-onions >/dev/null 2>&1; then
    echo "Detected system-wide .deb package 'green-onions'."
    if [ "${EUID}" -ne 0 ]; then
        echo "Removing via sudo apt remove..."
        sudo apt-get remove -y green-onions
    else
        apt-get remove -y green-onions
    fi
fi

echo ""
echo "Green Onions has been completely uninstalled."
