#!/bin/sh
# Builds the test app, installs it on the connected device and follows the TAP output.

set -e

root=$(cd "$(dirname "$0")" && pwd)

app=to.holepunch.bare.bluetooth.test

adb logcat -c

gradle -p "$root" installDebug

adb shell am start -W -n "$app/.MainActivity" > /dev/null

adb logcat --pid="$(adb shell pidof -s "$app" | tr -d '\r')" -v raw -s .bluetooth.test
