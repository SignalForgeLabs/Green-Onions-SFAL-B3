#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "================================================================"
echo " Signal Forge Audio Labs — Green Onions Installer (Linux)"
echo "================================================================"

BIN_DIR="${HOME}/.local/bin"
VST3_DIR="${HOME}/.vst3"
APP_DIR="${HOME}/.local/share/applications"

mkdir -p "${BIN_DIR}" "${VST3_DIR}" "${APP_DIR}"

echo "[1/4] Installing Standalone binary -> ${BIN_DIR}/green-onions"
install -m 755 "${SCRIPT_DIR}/Green Onions" "${BIN_DIR}/green-onions"

echo "[2/4] Installing VST3 plugin -> ${VST3_DIR}/Green Onions.vst3"
rm -rf "${VST3_DIR}/Green Onions.vst3"
cp -r "${SCRIPT_DIR}/Green Onions.vst3" "${VST3_DIR}/"

echo "[3/4] Installing dedicated Uninstaller -> ${BIN_DIR}/green-onions-uninstall"
install -m 755 "${SCRIPT_DIR}/uninstall.sh" "${BIN_DIR}/green-onions-uninstall"

echo "[4/4] Creating desktop & menu launchers -> ${APP_DIR}/"
cat > "${APP_DIR}/green-onions.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Green Onions
Comment=Signal Forge Audio Labs — Classic ToneWheels B-3 & Leslie 122/147
Exec=${BIN_DIR}/green-onions
Icon=multimedia-player
Terminal=false
Categories=AudioVideo;Audio;Music;Synthesizer;
EOF

cat > "${APP_DIR}/green-onions-uninstall.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Uninstall Green Onions
Comment=Remove Green Onions Standalone and VST3 Plugin
Exec=${BIN_DIR}/green-onions-uninstall
Icon=edit-delete
Terminal=true
Categories=AudioVideo;Audio;
EOF

chmod +x "${APP_DIR}/green-onions.desktop" "${APP_DIR}/green-onions-uninstall.desktop"

# Also place a Desktop shortcut if ~/Desktop exists
if [ -d "${HOME}/Desktop" ]; then
    cp "${APP_DIR}/green-onions.desktop" "${HOME}/Desktop/Green Onions.desktop"
    chmod +x "${HOME}/Desktop/Green Onions.desktop"
fi

# Refresh desktop application menu database immediately
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${APP_DIR}" 2>/dev/null || true
fi

echo ""
echo "Installation complete!"
echo "  Run Standalone : ${BIN_DIR}/green-onions"
echo "  App Menu       : Sound & Video -> Green Onions"
echo "  VST3 Location  : ${VST3_DIR}/Green Onions.vst3"
echo "  Uninstall      : ${BIN_DIR}/green-onions-uninstall"
