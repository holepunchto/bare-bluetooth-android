#!/bin/sh
# Fetches the two things test/test-app needs that the repository does not carry:
# the Bare Kit prebuild and a compiled addon.
#
# Usage: sh setup.sh [--asan]
#
# --asan builds the addon with AddressSanitizer and stages the NDK ASan runtime
# next to a wrap.sh that preloads it. There is no ASan build of Bare Kit to
# point at, so the runtime stays uninstrumented and the reports cover this
# addon. Run test.sh as usual; they arrive in the TAP output.

set -e

asan=

if [ "$1" = --asan ]; then
  asan=1
elif [ -n "$1" ]; then
  echo "Usage: sh setup.sh [--asan]" >&2
  exit 1
fi

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

llvm=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

echo "Downloading the latest Bare Kit prebuild"

gh release download --repo holepunchto/bare-kit --pattern prebuilds.zip --dir "$tmp"
unzip -q "$tmp/prebuilds.zip" -d "$tmp"

rm -rf "$root/app/libs/bare-kit"
mkdir -p "$root/app/libs"
mv "$tmp/android/bare-kit" "$root/app/libs/"

# Both directories are staged by --asan alone, so a plain run clears them.
rm -rf "$root/app/src/main/asan" "$root/app/src/main/resources"

if [ -n "$asan" ]; then
  echo "Staging the ASan runtime"

  runtime=$(find "$llvm" -name libclang_rt.asan-aarch64-android.so | head -1)

  if [ -z "$runtime" ]; then
    echo "No ASan runtime found in $ANDROID_NDK_HOME" >&2
    exit 1
  fi

  mkdir -p "$root/app/src/main/asan/arm64-v8a"
  cp "$runtime" "$root/app/src/main/asan/arm64-v8a/"

  # https://developer.android.com/ndk/guides/asan
  mkdir -p "$root/app/src/main/resources/lib/arm64-v8a"

  cat > "$root/app/src/main/resources/lib/arm64-v8a/wrap.sh" <<'WRAP'
#!/system/bin/sh
HERE="$(cd "$(dirname "$0")" && pwd)"
export ASAN_OPTIONS=log_to_syslog=false,allow_user_segv_handler=1
export LD_PRELOAD="$(ls "$HERE"/libclang_rt.asan-*-android.so) $HERE/libc++_shared.so"
exec "$@"
WRAP
fi

echo "Compiling the addon for android-arm64"

set -- --platform android --arch arm64 \
  -D ANDROID_PLATFORM=android-34 \
  -D ANDROID_STL=c++_shared

# --sanitize only reaches the C flags, and the binding is C++.
if [ -n "$asan" ]; then
  set -- "$@" --debug --sanitize address \
    -D "CMAKE_CXX_FLAGS=-fsanitize=address -fno-omit-frame-pointer"
fi

cd "$repo"

# --no-cache: the sanitizer flags would otherwise survive a run without --asan.
npx bare-make generate --no-cache "$@"
npx bare-make build
npx bare-make install

if [ -n "$asan" ]; then
  nm=$(find "$llvm" -name llvm-nm | head -1)
  addon=$repo/prebuilds/android-arm64/bare-bluetooth-android.bare

  if ! "$nm" "$addon" | grep -q __asan_; then
    echo "$addon came out without ASan" >&2
    exit 1
  fi
fi
