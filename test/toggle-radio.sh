#!/bin/sh
# Waits for test/state.js to start listening, then takes the radio off and on.

set -e

trap 'adb shell cmd bluetooth_manager enable > /dev/null 2>&1' EXIT INT TERM

since=$(adb shell date +%s.%N)

tries=0

until adb logcat -d -v raw -T "$since" 2> /dev/null | grep -q 'radio to be toggled'; do
  tries=$((tries + 1))

  if [ "$tries" -ge 300 ]; then exit 1; fi

  sleep 2
done

adb shell cmd bluetooth_manager disable > /dev/null

sleep 8

adb shell cmd bluetooth_manager enable > /dev/null
