# cgproject — Photorealistic Image Synthesis practicum

The ray tracing framework for the project. It provides scene loading (`.mgf`
natively, plus `.3ds`, `.obj`, `.ply`, `.gltf` and more through Assimp), an
OpenGL scene preview with camera navigation, an HDR framebuffer with tone
mapping and screenshots, and ray–scene intersection through Intel Embree.

Parts of the renderer are deliberately left unimplemented. **What you have to
build, and how it is graded, is described in the separate assignment
document — not here.** This file only covers getting the framework to compile
and run.

---

## 1. Install the tools

You need three things. Everything else is handled by the build.

| | |
|---|---|
| **A C++17 compiler** | Visual Studio 2022 (Windows) · g++ or clang (Linux) · Xcode command line tools (macOS) |
| **CMake 3.21+** | [cmake.org/download](https://cmake.org/download/), or your package manager. Visual Studio ships one. |
| **Git** | to fetch the framework and, on Windows, vcpkg |

On Windows, install Visual Studio 2022 with the **"Desktop development with
C++"** workload. That gives you the compiler, CMake and Git in one go.

---

## 2. Build

### Linux 

Everything is packaged, so this takes about two minutes total.

Ubuntu:

```bash
sudo apt install build-essential cmake git libembree-dev libassimp-dev libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
```

```bash
cmake --preset linux && cmake --build --preset linux -j
```

Fedora:

```bash
sudo dnf install gcc-c++ cmake git embree-devel assimp-devel glfw-devel mesa-libGLU-devel
```

> **Ubuntu 22.04 and older** ship Embree 3, which this framework does not use.
> Either build with `-DUSE_EMBREE=OFF`, or use the `linux-vcpkg` preset, or
> upgrade. Ubuntu 24.04 (Embree 4.3) and newer are fine.

### Windows

Install [vcpkg](https://vcpkg.io) once, anywhere on your machine:

```bash
git clone https://github.com/microsoft/vcpkg C:/vcpkg
```

```bash
C:/vcpkg/bootstrap-vcpkg.bat
```

Tell CMake where it is, once — open a **new** terminal afterwards so the
variable is picked up:

```bash
setx VCPKG_ROOT C:\vcpkg
```

> **Do not use the vcpkg bundled with Visual Studio.** Visual Studio ships a
> copy under `VC\vcpkg`, and the *Developer Command Prompt* / *Developer
> PowerShell* points `VCPKG_ROOT` at it. That copy is frozen at whatever vcpkg
> version your Visual Studio release shipped with — 2023-06-22 for VS 17.7 —
> and older ones cannot read current package manifests, failing with
> `$.default-features[0]: mismatched type: expected an identifier`.
>
> The build prefers a standalone clone automatically and says which one it
> used. A clone at `C:\vcpkg` or `%USERPROFILE%\vcpkg` is found even without
> `VCPKG_ROOT` set.

Then build:

```bash
cmake --preset windows
```

```bash
cmake --build --preset windows
```

You can also just open the folder in Visual Studio 2022 — it reads
`CMakePresets.json` and offers the presets in the configuration dropdown.

If you prefer the classic out-of-source workflow, that works too:

```bash
mkdir build && cd build && cmake .. && cmake --build . --config Release
```

CMake picks up vcpkg by itself, including the custom triplet. You do **not**
need to run `vcpkg install` by hand — this project uses vcpkg in manifest
mode, where the dependency list lives in `vcpkg.json` and is installed
automatically during configure. (`vcpkg install glfw3` will simply be refused
in manifest mode.)

**The first configure takes about 30 minutes**, because vcpkg compiles Embree
and Assimp from source. It only happens once; later builds take seconds. If
you would rather not wait, see *Making the first build faster* below.

### macOS

```bash
brew install cmake embree assimp glfw
```

```bash
cmake --preset macos && cmake --build --preset macos -j
```

### Windows (Using WSL)

If the Windows build gives you trouble, or you simply do not want to wait 30
minutes, use WSL. Windows 11 runs Linux GUI applications natively (WSLg), so
the render window works normally:

```bash
wsl --install -d Ubuntu-24.04
```

Then, inside Ubuntu, follow the Linux instructions above. Total time: a couple
of minutes. This is a perfectly valid way to do the practicum.

---

## 3. Run

Linux and macOS:

```bash
./build/linux/cgproject models/soda.mgf
```

On Windows the executable and the models land in `build\windows\Release\`, so
run it from there — the model path is relative to the working directory:

```bash
cd build\windows\Release
```

```bash
.\cgproject.exe models\soda.mgf
```

Press **F1** in the window for the key bindings. Move with the arrow keys and
page up/down; drag with the left mouse button to look around.

> The renderer is incomplete by design, so some images will not look right
> until you have done the work. That is expected, not a build problem.

### Rendering without a window

```bash
./cgproject models/soda.mgf --render out.ppm --size 1024 768
```

`--render` ray traces one frame from the default viewpoint straight to a
`.ppm` and exits without opening a window. Useful for rendering while you do
something else, and for comparing two builds on exactly the same view: the
starting camera is derived from the scene bounds, so it is identical on every
run. `--size` is optional and defaults to 512x384.

---

## 4. Build options

| Option | Default | Meaning |
|---|---|---|
| `USE_EMBREE` | `ON` | Ray traversal through Intel Embree. `OFF` falls back to the built-in uniform grid, which is slower and single threaded. |
| `USE_OPENMP` | `OFF` | Parallelise the pixel loops, roughly an 8–16x speedup. Requires `USE_EMBREE=ON`. |
| `USE_ASSIMP` | `ON` | Build the Assimp model loader. `.mgf` always works without it. |

```bash
cmake --preset linux -DUSE_OPENMP=ON
```

`USE_OPENMP` is off by default because a single-threaded render is much easier
to step through in a debugger. Turn it on once renders get slow enough to
annoy you. There are ready-made debug presets: `linux-debug`, `windows-debug`.

---

## 5. Making the first build faster

Only relevant on Windows, where vcpkg builds dependencies from source.

**Use a prebuilt Embree.** Embree is ~25 of those 30 minutes. Download the
official binary release for Windows x64 from
[github.com/RenderKit/embree/releases](https://github.com/RenderKit/embree/releases)
(`embree-4.x.y.x64.windows.zip`), unpack it into `third_party/` so you have
`third_party/embree-4.x.y.x64.windows/`, and configure with:

```bash
cmake --preset windows-embree-sdk
```

CMake finds the SDK automatically and vcpkg skips building Embree. First build
drops to about 10 minutes. The DLLs are copied next to the executable for you.

**Or set `EMBREE_ROOT`** to wherever you unpacked it, instead of using
`third_party/`.

> **For the course staff:** the single biggest improvement is a shared vcpkg
> **binary cache**. Build the dependencies once, publish them to a file share
> or a NuGet feed, and point students at it with
> `VCPKG_BINARY_SOURCES=clear;files,\\your-share\vcpkg-cache,read`. Their first
> build then takes about two minutes instead of thirty.

> Alongside the binary cache, an **asset cache** removes the other source of
> first-build flakiness: the dependency *source archives* are fetched from a
> dozen upstream hosts, and any one of them can return a 504. Point students
> at a mirror with
> `X_VCPKG_ASSET_SOURCES=clear;x-azurl,https://your-host/assets,,readwrite`
> (a plain file share works too) and those downloads come from you instead.

---

## 6. If the build fails

**Anything missing.** CMake will tell you what, and the exact command to
install it. Read the first few lines of the error, not the last.

**`'_mm_rsqrt14_ps': identifier not found`** while building Embree. Your
Visual Studio is older than the intrinsic Embree's AVX512 kernels use. The
build works around this by capping Embree at AVX2 (see
[`triplets/x64-windows-cgproject.cmake`](triplets/x64-windows-cgproject.cmake)).
If you hit it anyway, make sure the custom triplet is being used — the
configure output names it.

**`mismatched type: expected an identifier`** while vcpkg loads a port. You
are using the vcpkg bundled with Visual Studio, which is too old. See the note
in section 2 — install a standalone clone.

**`vcpkg install does not support individual package arguments`.** Correct,
and you do not need it: this project uses vcpkg in manifest mode, so the
dependency list lives in `vcpkg.json` and is installed for you during
configure. Just configure again.

**`VCPKG_ROOT` not found.** You need a new terminal after `setx`, or pass
`-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake` explicitly.

**`curl operation failed with response code 504`** (or 403, 429, or a
"Download failed, halting portfile" message). A transient failure fetching one
of the dependency source archives from GitHub — nothing to do with your setup.
vcpkg retries twice and then stops. **Just run the same configure command
again**: everything already built is cached, so it resumes rather than
starting over. If it keeps failing on the same package, you are likely behind
a proxy or a rate limit; try again later or from another network.

**`LNK1104: cannot open file 'Something\.obj'`, or a configure step failing
inside vcpkg.** Some vcpkg ports build with autotools, which does not quote
paths containing spaces and splits e.g. `Photorealistic Image Synthesis` into
pieces. The dependency that did this here has been removed, so the build works
from paths with spaces. If you hit it with some other port, move the project
somewhere like `C:\src\cgproject`.

**Out of disk space.** The vcpkg build needs about 10 GB of scratch space.
Afterwards you can delete `vcpkg/buildtrees/` to reclaim most of it.

**Nothing works and you have lost an afternoon.** Build without the optional
dependencies and get on with the actual work:

```bash
cmake -B build -DUSE_EMBREE=OFF -DUSE_ASSIMP=OFF && cmake --build build -j
```

That needs only GLFW and OpenGL. Ray tracing is slower and you lose the
non-`.mgf` loaders, but every `.mgf` scene still works. Come back to the fast
path later — and do tell us what went wrong.

---

## Credits

cgproject is an educational framework, originally developed by
**Prof. dr. Philippe Bekaert** and **dr. Tom Mertens** for the Photorealistic
Image Synthesis course. The renderer, the scene handling and the course
material are theirs.

The 2026 revision modernised the build and the back end with AI assistance, to
make the framework compile easily on current toolchains and to get students
started more quickly: CMake presets and vcpkg for dependencies, GLFW in place
of GLUT, Assimp in place of lib3ds, and Intel Embree for ray traversal.

In case of issues: see contact details in assignment!

Bundled third-party code: the MGF parser (Regents of the University of
California / Lawrence Berkeley Laboratory, 1994-1996) in `libs/mgflib`, and
the photon map implementation by Henrik Wann Jensen (2001) in `photonmap.h`.

