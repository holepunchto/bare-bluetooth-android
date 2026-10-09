#!/bin/sh
# Toggles the radio so test/state.js has something to observe.

set -e

trap 'adb shell cmd bluetooth_manager enable > /dev/null 2>&1' EXIT INT TERM

sleep "${1:-20}"

adb shell cmd bluetooth_manager disable > /dev/null

sleep 8

adb shell cmd bluetooth_manager enable > /dev/null
