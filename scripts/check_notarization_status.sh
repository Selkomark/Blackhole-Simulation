#!/bin/bash

# Script to check notarization status without waiting
# Usage: ./scripts/check_notarization_status.sh [submission-id]

KEYCHAIN_PROFILE="${NOTARY_KEYCHAIN_PROFILE:-notarization-profile}"

if [ -n "$1" ]; then
    SUBMISSION_ID="$1"
    echo "Checking status for submission: ${SUBMISSION_ID}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    xcrun notarytool log "${SUBMISSION_ID}" --keychain-profile "${KEYCHAIN_PROFILE}" 2>&1
else
    echo "Recent notarization submissions:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    xcrun notarytool history --keychain-profile "${KEYCHAIN_PROFILE}" 2>&1 | head -20
    
    echo ""
    echo "To check a specific submission:"
    echo "  ./scripts/check_notarization_status.sh <submission-id>"
fi

