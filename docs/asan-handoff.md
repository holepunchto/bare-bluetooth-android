# ASan handoff

Working notes for continuing this on an x86-64 Linux machine. Delete once the work lands.

The goal is an ASan build of the Android test app where the whole native stack is instrumented. Everything below the V8 prebuild already is. V8 is the last gap, and it cannot be built on macOS.

## State

### `bare-bluetooth-android` - uncommitted, on `main`

| File | Change |
|---|---|
| `test/test-app/setup.sh` | `--asan` flag: builds the addon with ASan, stages the NDK runtime and a `wrap.sh`, asserts `__asan_` in the built addon |
| `test/test-app/app/build.gradle` | packages `src/main/asan` as jniLibs, `useLegacyPackaging = true` so `wrap.sh` can run |
| `test/test-app/app/src/main/.gitignore` | ignores the staged `asan/` and `resources/` |
| `docs/asan.md` | the recipe |
| `README.md` | links it |

### `bare-kit` - uncommitted, on `ipc-in-process-queue` at `d759bde`

One file, `android/build.gradle`:

```diff
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

   packagingOptions {
     jniLibs {
       excludes.add("**/libuv.so")
+      keepDebugSymbols.add("**/libbare-kit.so")
     }
   }
```

The `ndkVersion` bump is local convenience, not a requirement: NDK 28.2.13676358 was not installed and the machine had no `sdkmanager`. What matters is that the ASan runtime staged into the app comes from the same NDK the library was built with.

## Verified

`ANDROID_HOME=$HOME/Library/Android/sdk ./gradlew :bare-kit:assembleRelease`, two minutes cold, produces a 23 MB aar whose `jni/arm64-v8a/libbare-kit.so` has 43 undefined `__asan_` symbols and a `NEEDED` on `libclang_rt.asan-aarch64-android.so`. The published release has none.

625 of the build's 748 translation units carry `-fsanitize=address`. The 123 that do not are BoringSSL hand-written `.S` assembly, mostly for other architectures. Spot-checked as instrumented: `bare-src/src/runtime.c`, `libuv-src/src/unix/core.c`, `bare-kit/android/src/main/jni/Worklet.c`.

The test APK builds with it and contains `lib/arm64-v8a/wrap.sh`, `libclang_rt.asan-aarch64-android.so`, a 98 MB unstripped `libbare-kit.so`, and a manifest with `extractNativeLibs=true`.

Use `llvm-nm -D -u` for these checks. Without `-D` it reads the static symbol table and reports `no symbols`, which means nothing.

## Not verified

**None of it has run on a device.** No phone was attached. The first thing to do anywhere is `sh test/test-app/test.sh` with a phone plugged in.

## What is still uninstrumented

`BARE_PREBUILDS` stays on, so three static libraries are downloaded rather than built: `libv8.a` (144 MB), `libjs.a` and `libc++.a`. All three measure 0 `__asan_` refs. NDK `libc++_shared.so` is uninstrumented too.

`BARE_PREBUILDS` does not control whether Bare is prebuilt - Bare Kit fetches Bare's source and compiles it either way. It only controls where V8, libjs and libc++ come from.

## The Linux job

Build an ASan V8 for `android-arm64` and link Bare Kit against it.

### Why Linux

`chromium/src/docs/android_build_instructions.md`, in the checkout itself:

> An x86-64 machine running Linux with at least 8GB of RAM.
>
> Building the Android client on Windows or Mac is not supported and doesn't work.

Several Android dependencies in `src/DEPS` are gated on `host_os == "linux"`. `holepunchto/chromium-prebuilds`' own CI builds every `android-*` target in its `linux` job for the same reason.

### Steps

Mirror what `chromium-prebuilds/.github/workflows/prebuild.yml` does in its `linux` job.

```sh
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PWD/depot_tools:$PATH"

mkdir chromium && cd chromium
cat > .gclient <<'GCLIENT'
solutions = [
  { "name": "src/prebuilds",
    "url": "https://github.com/holepunchto/chromium-prebuilds.git",
    "deps_file": "DEPS",
    "managed": False,
    "custom_deps": {},
    "custom_vars": { "checkout_pgo_profiles": True },
  },
]
target_os = ["linux", "android"]
target_cpu = ["x64", "arm64"]
GCLIENT

gclient sync -D
git apply --directory src src/prebuilds/patches/*.patch
```

Patches must be reverted before any later `gclient sync` and reapplied after; the sync needs a clean tree.

Then the only line that differs from CI is the extra sanitizer import:

```sh
cd src
gn gen out/android-arm64/v8-asan --args="import(\"//prebuilds/v8.gni\") import(\"//prebuilds/mode/debug.gni\") import(\"//prebuilds/target/android-arm64.gni\") import(\"//prebuilds/sanitizer/address.gni\")"
ninja -C out/android-arm64/v8-asan prebuilds
```

Output: `out/android-arm64/v8-asan/obj/prebuilds/{libv8.a,libc++.a}`. The macOS debug V8 is 6+ GB, so budget disk.

### Then point Bare Kit at it

Add to the `cmake` block in `bare-kit/android/build.gradle`, alongside the ASan flags already there:

```
"-DBARE_PREBUILDS=OFF",
"-DGN_DIR=<path>/chromium/src",
"-DGN_OUT_DIR=<path>/chromium/src/out/android-arm64/v8-asan",
```

With `BARE_PREBUILDS=OFF`, libjs is fetched and compiled from source instead of downloaded, so `cppFlags` starts mattering: `libjs` is C++ and would otherwise sit uninstrumented between an instrumented V8 and an instrumented Bare - exactly the libc++ annotation mismatch chromium-prebuilds warns about.

### The thing that might not work

On macOS, pairing an ASan `bare` with the ASan V8 crashes at `v8::V8::Initialize`, because two ASan runtimes end up in one process: homebrew's `libclang_rt` and Apple's `libsystem_sanitizers`. That is why the Apple setup pairs an ASan `bare` with the plain `v8-debug`.

Android has one ASan runtime, the NDK's, so the cause does not transfer. This is a hypothesis, not a result. If it does crash the same way, fall back to a plain `out/android-arm64/v8-debug` - that still buys a debug V8 with symbols, just no instrumentation.

### The cheaper alternative

`chromium-prebuilds/.github/workflows/prebuild.yml` already builds `android-arm64` on `beefcake-linux-x64`, release only, with no sanitizer input. Adding a `sanitizer` choice to its `workflow_dispatch` and threading it into the `gn gen` line is about fifteen lines of YAML, and the artifact comes back as a download. Worth weighing against standing up a Linux box.

## Levers once it runs

`setup.sh` writes `ASAN_OPTIONS=log_to_syslog=false,allow_user_segv_handler=1` into `wrap.sh`.

- `container-overflow` reports inside `std::` code mean the libc++ annotation mismatch. Add `detect_container_overflow=0`.
- Leak detection is off by default but the NDK runtime does ship LeakSanitizer - `__lsan_do_leak_check` is present in `libclang_rt.asan-aarch64-android.so`. `detect_leaks=1` is worth a try.

## Traps hit along the way

`bare-make --sanitize address` only fills `CMAKE_C_FLAGS`, never `CMAKE_CXX_FLAGS`. Any C++ needs `-D CMAKE_CXX_FLAGS=-fsanitize=address` passed by hand. This bites `binding.cc` here and `libjs` in the Bare build.

Old addon builds pile up in `test/test-app/app/src/main/addons/arm64-v8a` across version bumps and all of them get packaged. A stale uninstrumented `libbare-bluetooth-android.0.9.0.so` shipped in the APK next to the current one. Delete them.

`setup.sh` replaces `app/libs/bare-kit` with the published release on every run, so a locally built aar has to be unpacked after it, not before, and the directory wiped first so the release's other architectures do not linger.
