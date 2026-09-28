#!/bin/sh
# Builds the test app, installs it on the connected device and follows the TAP output.

set -e

root=$(cd "$(dirname "$0")" && pwd)

app=to.holepunch.bare.bluetooth.test

adb logcat -c

gradle -p "$root" installDebug

if [ -n "$1" ]; then
  adb shell am start -W -n "$app/.MainActivity" --es suite "$1" > /dev/null

  if [ -f "$root/$1.sh" ]; then
    sh "$root/$1.sh" &
  fi
else
  adb shell am start -W -n "$app/.MainActivity" > /dev/null
fi

adb logcat --pid="$(adb shell pidof -s "$app" | tr -d '\r')" -v raw -s .bluetooth.test
