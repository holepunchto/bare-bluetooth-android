#!/bin/sh
# Fetches the two things test/test-app needs that the repository does not carry:
# the Bare Kit prebuild and a compiled addon.

set -e

root=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$root/../.." && pwd)

if [ -z "$ANDROID_NDK_HOME" ]; then
  sdk=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}
  [ -d "$sdk" ] || sdk=$HOME/Android/Sdk

  ANDROID_NDK_HOME=$(ls -d "$sdk"/ndk/* 2>/dev/null | sort -V | tail -1)
  export ANDROID_NDK_HOME
fi

if [ ! -d "$ANDROID_NDK_HOME" ]; then
  echo "No Android NDK found; set ANDROID_NDK_HOME" >&2
  exit 1
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

echo "Downloading the latest Bare Kit prebuild"

gh release download --repo holepunchto/bare-kit --pattern prebuilds.zip --dir "$tmp"
unzip -q "$tmp/prebuilds.zip" -d "$tmp"

rm -rf "$root/app/libs/bare-kit"
mkdir -p "$root/app/libs"
mv "$tmp/android/bare-kit" "$root/app/libs/"

echo "Compiling the addon for android-arm64"

cd "$repo"
npx bare-make generate --platform android --arch arm64 -D ANDROID_PLATFORM=android-34 -D ANDROID_STL=c++_shared
npx bare-make build
npx bare-make install
