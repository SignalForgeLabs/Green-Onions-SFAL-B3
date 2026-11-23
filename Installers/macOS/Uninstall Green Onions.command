#!/bin/bash
set -e

echo "================================================================"
echo " Signal Forge Audio Labs — Green Onions Uninstaller (macOS)"
echo "================================================================"
echo ""
echo "This will remove:"
echo "  - /Applications/Green Onions.app"
echo "  - /Library/Audio/Plug-Ins/VST3/Green Onions.vst3"
echo "  - /Library/Audio/Plug-Ins/Components/Green Onions.component"
echo "  - /Applications/Signal Forge Audio Labs/Uninstall Green Onions.command"
echo ""
read -r -p "Proceed with uninstall? [y/N]: " reply
if [[ ! "$reply" =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled."
    exit 0
fi

sudo rm -rf "/Applications/Green Onions.app"
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Green Onions.vst3"
sudo rm -rf "/Library/Audio/Plug-Ins/Components/Green Onions.component"
sudo pkgutil --forget com.signalforgeaudio.greenonions.standalone 2>/dev/null || true
sudo pkgutil --forget com.signalforgeaudio.greenonions.vst3 2>/dev/null || true
sudo pkgutil --forget com.signalforgeaudio.greenonions.au 2>/dev/null || true
sudo pkgutil --forget com.signalforgeaudio.greenonions.uninstaller 2>/dev/null || true
sudo rm -rf "/Applications/Signal Forge Audio Labs/Uninstall Green Onions.command"
sudo rmdir "/Applications/Signal Forge Audio Labs" 2>/dev/null || true

echo ""
echo "Green Onions has been completely uninstalled."
read -r -p "Press Enter to close..." _
