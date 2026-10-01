# AddressSanitizer on device

`setup.sh --asan` instruments the addon and stages the NDK ASan runtime into the test app. That covers `binding.cc` and needs nothing beyond an NDK.

It does not cover Bare Kit. The published `libbare-kit.so` carries no `__asan_` symbols, so a bad access inside the runtime is missed. This document is how to build one that is instrumented, and how to run the tests against it.

Bare Kit has no sanitizer switch and its `CONTRIBUTING.md` documents only the iOS build, so the patch below is a local change to a Bare Kit checkout. It belongs upstream eventually.

## What instrumentation buys

ASan is two separate pieces:

- **The allocator** puts red zones around every block. It arrives through `LD_PRELOAD` in `wrap.sh`, so it applies to the whole process. Double frees, invalid frees and `memcpy` overflows are caught anywhere, instrumented or not.
- **The checks** are emitted by the compiler before each read and write. They exist only in code built with `-fsanitize=address`.

With the addon alone instrumented, a bad access in `binding.cc` is caught even on memory Bare Kit allocated, but Bare Kit reading memory the addon freed is not. Instrumenting Bare Kit closes that.

## What stays uninstrumented, on purpose

`BARE_PREBUILDS` does not decide whether Bare is prebuilt. Bare Kit fetches Bare's source and compiles it either way; the option only decides where V8, libjs and libc++ come from. Turning it off means building them from a Chromium checkout, which buys a debug V8 rather than an instrumented one - the Apple side pairs its ASan `bare` with a plain `v8-debug` for exactly that reason.

On, it downloads exactly three static libraries, and none of them carry `__asan_` symbols: `libv8.a`, `libjs.a` and `libc++.a`. Everything else is compiled locally and does get instrumented - 625 of the build's 748 translation units, the other 123 being BoringSSL hand-written assembly, which cannot be instrumented and is mostly for other architectures. Bare, libuv and the Bare Kit JNI layer are all in the instrumented half.

V8 arrives as a prebuild inside Bare Kit and stays uninstrumented. That matches the known-good pairing on the Apple side, where an ASan `bare` links against the plain `v8-debug` prebuild; pairing it with `v8-asan` instead puts two ASan runtimes in one process and crashes at `v8::V8::Initialize`. `BARE_PREBUILDS` stays on, so nothing needs a Chromium checkout.

NDK `libc++_shared.so` is uninstrumented as well. Instrumented and uninstrumented users of one libc++ disagree on container annotations:

> The prebuild must be instrumented with the same sanitizers as the program it is linked into, or the two will disagree on the annotations they apply to shared libc++ containers.
>
> -- [chromium-prebuilds](https://github.com/holepunchto/chromium-prebuilds#building)

If reports come back as `container-overflow` in `std::` code, add `detect_container_overflow=0` to the `ASAN_OPTIONS` line `setup.sh` writes into `wrap.sh`.

## Building an instrumented Bare Kit

### 1. Settle the NDK

`android/build.gradle` pins `ndkVersion`, and the ASan runtime staged into the app has to come from that same NDK. Either install the pinned version, or repin to one you already have and point `setup.sh` at it.

The build below was done by repinning to `29.0.13846066`.

### 2. Patch `android/build.gradle`

```diff
 android {
   namespace = "to.holepunch.bare.kit"
   compileSdk 34
-  ndkVersion = "28.2.13676358"
+  ndkVersion = "29.0.13846066"

   defaultConfig {
     minSdk 29
     targetSdk 34

+    ndk {
+      abiFilters "arm64-v8a"
+    }
+
     externalNativeBuild {
       cmake {
         targets "bare_kit"
-        arguments "-DDRIVE_CORESTORE_DIR=build/_drive", "-DANDROID_STL=c++_shared"
+        arguments "-DDRIVE_CORESTORE_DIR=build/_drive", "-DANDROID_STL=c++_shared", "-DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address"
+        cFlags "-fsanitize=address", "-fno-omit-frame-pointer"
+        cppFlags "-fsanitize=address", "-fno-omit-frame-pointer"
       }
     }

@@
   packagingOptions {
     jniLibs {
       excludes.add("**/libuv.so")
+      keepDebugSymbols.add("**/libbare-kit.so")
     }
   }
```

`cFlags` and `cppFlags` are both needed: Bare is C, the engine glue is C++. `CMAKE_SHARED_LINKER_FLAGS` is what puts `libclang_rt.asan-aarch64-android.so` in the library's `DT_NEEDED`. `keepDebugSymbols` stops the release build stripping the symbols ASan needs to name a frame. `abiFilters` drops three architectures the test app never installs.

### 3. Build

```console
cd bare-kit
npm i
ANDROID_HOME=$HOME/Library/Android/sdk ./gradlew :bare-kit:assembleRelease
```

Two minutes cold. The archive lands at `android/build/outputs/aar/bare-kit-release.aar`, 23 MB.

### 4. Check it took

```console
unzip -p android/build/outputs/aar/bare-kit-release.aar jni/arm64-v8a/libbare-kit.so > /tmp/libbare-kit.so
llvm-nm -D -u /tmp/libbare-kit.so | grep -c __asan_
llvm-readelf -d /tmp/libbare-kit.so | grep asan
```

Expect 43 and a `NEEDED` line. `llvm-nm` without `-D` reads the static symbol table and reports `no symbols`, which says nothing either way. Both tools live under `$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/*/bin`.

## Running the tests against it

`setup.sh` replaces `app/libs/bare-kit` with the published release on every run, so unpack yours after it:

```console
sh test/test-app/setup.sh --asan
rm -rf test/test-app/app/libs/bare-kit
unzip -q ../bare-kit/android/build/outputs/aar/bare-kit-release.aar -d test/test-app/app/libs/bare-kit
sh test/test-app/test.sh
```

`rm -rf` first, or the release's other three architectures stay behind uninstrumented.

If this stops being a one-off, `setup.sh` should take a `BARE_KIT=<path to an aar>` override and assert `__asan_` in it, the way the Apple repository's `run-test-asan` takes `BARE=<path to an ASan bare>`.

### What the APK should contain

```console
unzip -l test/test-app/app/build/outputs/apk/debug/app-debug.apk | grep -E "wrap.sh|asan|bare-kit"
```

`lib/arm64-v8a/` holds `wrap.sh`, `libclang_rt.asan-aarch64-android.so` and a `libbare-kit.so` around 98 MB, up from the stripped 63 MB of the release. The manifest must say `extractNativeLibs=true`, which `useLegacyPackaging` in the test app's `build.gradle` sets; `wrap.sh` cannot run without it.

Old addon builds linger in `app/src/main/addons/arm64-v8a` across version bumps and get packaged alongside the current one. Delete them.
