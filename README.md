> **This is a personal fork of [CrossInk](https://github.com/uxjulia/CrossInk)** that adds [BookOrbit](https://github.com/bookorbit/bookorbit) integration: reading-progress sync and a catalog browser for downloading books straight from your own BookOrbit server.

Everything else — fonts, themes, reader features, reading stats, controls, the web server — comes from CrossInk unchanged. See the [upstream README](https://github.com/uxjulia/CrossInk#readme) for those, and the [docs](./docs/) folder in this repository for the detailed guides.

**Note**: like upstream, this firmware runs on both the Xteink X3 and X4.

---

## What's different in this fork

- **BookOrbit progress sync** — sync your reading position with a self-hosted BookOrbit server, independently of (and alongside) KOReader Sync.
- **BookOrbit catalog browser** — browse your server's library on the device and download EPUBs over WiFi, including by author and by series.
- **Offline shortcuts to your own books** — "On device" and "In progress" categories that open a book directly, without touching the network.

BookOrbit exposes a KOReader-compatible sync API, so this fork talks to it the same way the official BookOrbit KOReader plugin does. Your BookOrbit server must be recent enough to serve the KOReader plugin endpoints under `{server}/api/v1/koreader` — including `plugin/catalog/*` for the catalog browser.

---

## Setting up your BookOrbit account

1. On the device, go to **Settings → System → BookOrbit Sync**.
2. Fill in **Username**, **Password** and **Server URL**. The URL accepts a bare hostname (`books.example.com`); `https://` is assumed, and a pasted `/api/v1` or `/api/v1/koreader` suffix is stripped for you.
3. Choose **Authenticate**. The device connects to WiFi and validates the credentials against your server. You should see *Successfully authenticated!*

Credentials live on the SD card in `/.crosspoint/bookorbit.json`, obfuscated with the device's hardware MAC — the same scheme CrossInk uses for KOReader credentials. KOReader Sync keeps its own separate credentials and server, so you can use both providers at once.

## Syncing reading progress

Open a book, then **Menu → Bookmarks tab → BookOrbit Sync**. The device compares the position stored on the server with your local one — chapter, page, percentage and the device name that last uploaded — and offers two choices:

- **Apply remote progress** — jump to the position from your other reader.
- **Upload local progress** — publish your current position to the server.

The option matching the furthest-read position is preselected. If the server has no progress for the book yet, you are offered a straight upload.

BookOrbit identifies books by the binary partial-MD5 hash of the EPUB file (the same "Binary" matching KOReader offers), so the **same EPUB file** has to be present on both readers for positions to pair up. A book downloaded from your BookOrbit catalog is the exact file your server knows about, so it matches automatically.

### Syncing without opening the menu

**Settings → Controls** lets you bind *BookOrbit Sync* to the power button (short or long press) or to a long press on Menu or Back. The action also works outside the reader: it syncs the book you last had open, or opens the BookOrbit settings if no account is configured yet.

## Browsing and downloading from the catalog

Reach the catalog from **Settings → System → BookOrbit Sync → Browse Catalog**, or from the **BookOrbit** entry in the home menu (it appears once an account is configured).

The root list contains:

| Entry | What it shows |
| --- | --- |
| Recently added / Continue reading / All books | Your server's own sections |
| **Authors** / **Series** | Paged lists with a book count per entry; pick one to see its books (series are listed in series order) |
| **Search** | Free-text search of your library |
| **On device** | Every EPUB already in the SD card root and the `/Read` folder — works offline |
| **In progress** | Recent books you haven't finished yet — works offline |

In a server book list, **Confirm** downloads the book to the SD card root as `Title - Author.epub`. A download that gets interrupted resumes automatically on retry, and **Back** cancels it. Books already present on the device are marked with a dot at the end of the line, so you can tell at a glance what is worth downloading.

In **On device** and **In progress**, Confirm opens the book in the reader instead of downloading it.

> If the catalog reports *"BookOrbit sent an unexpected reply"*, the server answered but not with the catalog API — usually a BookOrbit version without the KOReader catalog endpoints, a wrong server URL, or a reverse proxy/SSO layer intercepting `/api/v1/koreader/plugin/*`.

---

## Installation

Download a `firmware-*.bin` from **this repository's** [releases page](https://github.com/agosez/CrossInk-Bookorbit/releases) — not upstream's, which does not include BookOrbit support — then flash it with the web installer or the command line. See [Installation](./docs/installation.md) for step-by-step flashing and revert instructions, and [Font Build Variants](./docs/font-build-variants.md) to pick between the `tiny` and `xlarge` builds.

Once this firmware is installed, **Settings → System → Check for Updates** resolves against this fork's releases, so later versions arrive over the air. Devices running upstream CrossInk will not see these releases: the first install has to be done by USB or SD card.

## Development

```sh
pio run -e tiny --target upload   # build and flash over USB-C
```

See [Getting Started](./docs/contributing/getting-started.md) for prerequisites and validation commands, and [Testing and Debugging](./docs/contributing/testing-debugging.md) for serial logging and static analysis. `AGENTS.md` holds the repository's engineering conventions.

## Documentation

- [User Guide](./USER_GUIDE.md) and [Reader Features](./docs/reader-features.md)
- [Controls](./docs/controls.md) — full button action list
- [Installation](./docs/installation.md) and [Font Build Variants](./docs/font-build-variants.md)
- [Data Cache](./docs/data-cache.md) and [File Formats](./docs/file-formats.md)
- [Common issues](./docs/troubleshooting.md)
