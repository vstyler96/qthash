[![Build Status](https://github.com/vstyler96/qthash/actions/workflows/build.yml/badge.svg)](https://github.com/vstyler96/qthash/actions/workflows/build.yml)
[![Test Coverage](https://codecov.io/gh/vstyler96/qthash/branch/master/graph/badge.svg)](https://app.codecov.io/gh/vstyler96/qthash)

QtHash
=======
![Screenshot](screenshots/readme.png)

QtHash is a desktop utility for computing message digests or checksums.
Most well-known hash functions are supported, including MD5, SHA1,
SHA2 (SHA256/SHA512), SHA3 and BLAKE2.

This is a direct port of [GtkHash](https://github.com/gtkhash/gtkhash) to work smoothly with KDE Plasma and any other Qt based DE.

It's designed to be an easy to use, graphical alternative to command-line
tools such as md5sum.

Features:
* Support for verifying checksum files from sfv, sha256sum, etc.
* Keyed hashing (HMAC)
* Parallel/multi-threaded hash calculation
* Remote file access using GIO/GVfs
* Small and fast

QtHash is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later
version.

Dependencies
------------
Required:
* Qt 6 (>= 6.x)

Optional:
* Libgcrypt (default)
* libb2 (default)
* libblake3 (default)
* libxxhash (default)
* zlib (default)
* Nettle
* OpenSSL
* Mbed TLS
* Linux kernel AF_ALG

Building from Source
--------------------
To compile and run using meson:
```
meson setup _build
meson compile -C _build
meson devenv -C _build qt/qthash
```
See `meson configure _build` for the full list of build options.

Translations
------------
![Translation Details](https://hosted.weblate.org/widgets/gtkhash/-/multi-auto.svg)

If you would like to contribute a translation, the easiest way is by supporting the base project GtkHash:

https://hosted.weblate.org/engage/gtkhash/

Alternatively, updated .po files can be submitted as a Pull Request or Issue on
GitHub:

https://github.com/vstyler96/qthash
