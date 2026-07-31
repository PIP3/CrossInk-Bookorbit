# Changelog

This repository is a fork of [CrossInk](https://github.com/uxjulia/CrossInk) and
records only its own additions. Each release states the CrossInk version it is based
on; for everything inherited from upstream, see the
[CrossInk changelog](https://github.com/uxjulia/CrossInk/blob/main/CHANGELOG.md).

## [v1.4.1+bookorbit.1] - 2026-07-31

Based on CrossInk v1.4.0.

### Added

- BookOrbit Sync, an alternative reading-progress sync provider alongside KOReader Sync: configure your account under Settings > BookOrbit Sync, then sync a book from the reader menu. It can also be assigned to the power button (short or long press) or to long-press Menu/Back in the reader, like KOReader's Sync Progress.
- A BookOrbit catalog browser for finding and downloading EPUBs from your server, reachable from Settings > BookOrbit Sync > Browse Catalog or from a BookOrbit entry in the home menu. It lists the server's own sections, browses books by author or by series, searches the library, and adds two offline categories: "On device" (EPUBs already in the download and /Read folders) and "In progress" (recent books you haven't finished) — picking one of those opens the book in the reader. Books already present on the device are marked with a dot at the end of the line.

### Fixed

- Downloads (BookOrbit and OPDS) allocate their transfer buffer before the TLS handshake instead of at the post-handshake heap low point, so they no longer fail to start when memory is tight, and pressing Back during the connection phase now cancels instead of being ignored until the first bytes arrive.
