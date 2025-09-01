# containerfs

`containerfs` is a C++23 library that mounts container formats such as OLE,
ZIP, or TAR and exposes them through a simple filesystem-like API.

## Features

- **Device abstraction** – use the generic device concepts to read from files
  or other byte sources.
- **Pluggable drivers** – write a driver that understands a container format
  and combine it with a device to create a filesystem.
- **FileDevice** – a ready-to-use device for reading from regular files.
- **Experimental OLE driver** – demonstrates parsing of compound OLE files.

## Build

Dependencies are managed with [vcpkg](https://vcpkg.io); the only dependency
today is GoogleTest for the unit tests.  Build the project with CMake presets:

```bash
cmake --preset build-release
cmake --build --preset build-release
```

## Tests

The preset also enables the test suite.  Run it with:

```bash
ctest --preset build-release
```

## Next Steps

The OLE driver is incomplete and serves as a starting point.  Adding
fully-featured drivers for OLE, ZIP, and TAR or extending the test coverage are
natural next contributions.

