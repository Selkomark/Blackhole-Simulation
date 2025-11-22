#!/bin/bash

# Helper script to launch Blackhole Sim.app
# This bypasses Gatekeeper for unnotarized Developer ID apps

APP_BUNDLE="export/Blackhole Sim.app"
EXECUTABLE_NAME="blackhole_sim"

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "❌ Error: App bundle not found at ${APP_BUNDLE}"
    echo "   Run 'make app' first to create the app bundle"
    exit 1
fi

echo "🚀 Launching Blackhole Sim..."
echo ""

# Method 1: Direct execution (bypasses Gatekeeper)
if [ -f "${APP_BUNDLE}/Contents/MacOS/${EXECUTABLE_NAME}" ]; then
    echo "✅ Launching app directly..."
    "${APP_BUNDLE}/Contents/MacOS/${EXECUTABLE_NAME}" &
    echo "   App launched! Check your screen for the window."
    exit 0
else
    echo "❌ Executable not found in app bundle"
    exit 1
fi

