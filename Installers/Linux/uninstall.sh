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
rm -f  "${HOME}/Desktop/Green Onions.desktop"
rm -f  "${HOME}/.local/bin/green-onions-uninstall"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${HOME}/.local/share/applications" 2>/dev/null || true
fi

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
