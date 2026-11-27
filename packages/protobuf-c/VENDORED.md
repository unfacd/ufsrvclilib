# protobuf-c (vendored)

## What this is

A vendored copy of **protobuf-c 1.4.1** — the C implementation of Protocol
Buffers plus its C code generator (`protoc-gen-c`) — built privately for the
`ufsrvcli` tree.

## Provenance (baseline)

| Field | Value |
|---|---|
| Upstream project | [protobuf-c](https://github.com/protobuf-c/protobuf-c) |
| Version | 1.4.1 (`PACKAGE_VERSION` in `build-cmake/CMakeLists.txt`) |
| Tag | `v1.4.1` |
| Commit | `abc67a11c6db271bedbb9f58be85d6f4e2ea8389` |
| Fetched on | 2026-10-19 |
| License | BSD-2-Clause (see `LICENSE`) |
| Local additions | this note and `CMakeLists.txt` (wrapper); no upstream file is edited |
| Vendored from upstream | `protobuf-c/` (runtime: `protobuf-c.c`, `protobuf-c.h`, `protobuf-c.proto`), `protoc-c/` (generator), `LICENSE` |
| Not vendored | upstream build systems and packaging (`Makefile.am`, `configure.ac`, `autogen.sh`, `m4/`, `build-cmake/`), docs (`README.md`, `CONTRIBUTING.md`, `ChangeLog`, `TODO`, `Doxyfile.in`, `DoxygenLayout.xml`), `t/` (tests), `.commit_docs.sh`, `.gitignore` |

## Why it is vendored

The CLI needs both halves of protobuf-c at one pinned revision: the runtime that
its generated `.pb-c.c` sources link against, and the `protoc-gen-c` plugin that
produces them. Taking the runtime from uflib
(`UFLIB_CAPABILITY_PROTOBUF_C`) and the generator from the system package
couples the tree to two independent version streams, so both now come from this
copy.

## Build

The local wrapper `CMakeLists.txt` builds the artefacts from the vendored
sources directly — only the sources are vendored, so no upstream build system is
involved. The version reported by the generator is the baseline recorded above.
The runtime (`protobuf-c::protobuf-c`) always builds; the `protoc-gen-c` plugin
only when the maintainer asks for it:

```bash
cmake -B build -DUFSRVCLI_PROTOBUF_C_GENERATOR=ON   # needs libprotoc-dev
```

Generating `.pb-c` sources is a maintainer task, not part of the ordinary
build. The flag is repeatable: turn it on, generate, turn it off. With the flag
on and libprotoc-dev absent, configuration stops with an error naming that
package — the dependency is never guessed.

## Update procedure

Re-vendor into a scratch directory and copy only the sources over, keeping the
two local files:

```bash
git clone --depth 1 --branch v<version> https://github.com/protobuf-c/protobuf-c.git /tmp/protobuf-c
cp /tmp/protobuf-c/LICENSE                                  packages/protobuf-c/
cp /tmp/protobuf-c/protobuf-c/protobuf-c.{c,h,proto}        packages/protobuf-c/protobuf-c/
cp /tmp/protobuf-c/protoc-c/*                               packages/protobuf-c/protoc-c/
```

Then bump `_pc_version` in `CMakeLists.txt`, record the new tag and commit in
the table above, and rebuild with the generator flag to confirm the generated
`.pb-c` pair is unchanged.
