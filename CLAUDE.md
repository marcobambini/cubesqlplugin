# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CubeSQLPlugin is a Xojo plugin (C/C++) that wraps the CubeSQL C SDK, enabling Xojo developers to connect to CubeSQL database servers. It targets 9 platform/architecture combinations: Linux (i386, x86-64, ARM32, ARM64), macOS (x86-64, ARM64, Universal), and Windows (32-bit, 64-bit, ARM64).

## Build Commands

**Linux** (from `gcc/` directory):
```
make                # auto-detects architecture
make clean          # clean build artifacts
```
Override architecture: `LINUX_LIBPATH=../libs/linux-arm64 make`

**macOS** (from repo root):
```
xcodebuild -project XCode/CubeSQLPlugin.xcodeproj -scheme CubeSQLPlugin -configuration Release
```

**Windows** (Visual Studio 2019+):
```
msbuild VisualC/CubeSQLPlugin/CubeSQLPlugin.sln /p:Configuration=Release /p:Platform=x64
```
Platforms: `Win32`, `x64`, `ARM64`

**CI/CD**: GitHub Actions workflow (`.github/workflows/cubesqlplugin.yaml`) builds all 9 targets in parallel via manual dispatch. It packages everything into `CubeSQLPlugin.xojo_plugin` (a ZIP archive).

## Architecture

The plugin has two layers:

1. **CubeSQL C SDK** (`CubeSQL_SDK/`) — Low-level C client library handling network protocol, encryption (AES128/192/256, SSL/TLS via LibreSSL), compression (zlib), and wire format. Key files: `cubesql.c`, `cubesql.h` (public API), `csql.h` (private internals). The `crypt/` subdirectory contains AES, SHA1, base64, and PRNG implementations.

2. **Xojo Plugin Wrapper** (`sources/`) — C++ layer that maps the C SDK onto Xojo's plugin framework (`rb_plugin.h` from `Xojo_SDK/`). Exposes three Xojo classes:
   - `CubeSQLServer` — Database connection, SQL execution, transactions, chunked data transfer
   - `CubeSQLVM` — Virtual machine / prepared statement interface
   - `CubeSQLPreparedStatement` — Xojo `PreparedSQLStatement` subclass

   All class definitions, property arrays, and method arrays are declared in `CubeSQLPlugin.h`; implementations are in `CubeSQLPlugin.cpp`.

## Key Conventions

- **Version** is defined as `PLUGIN_VERSION` in `sources/CubeSQLPlugin.h`
- **Pre-built libraries** (LibreSSL, zlib) live in `libs/` organized by platform
- **Build outputs** go into `CubeSQLPlugin/Build Resources/<Platform>/` as `.so`, `.dylib`, or `.dll`
- The final `.xojo_plugin` file is a ZIP of the `CubeSQLPlugin/` directory
- Compiler flags include `-DCUBESQL_ENABLE_SSL_ENCRYPTION=1` to enable SSL support
- Linux builds use `-std=c++11 -fPIC` and link against `-lz -ldl -lpthread -ltls`
