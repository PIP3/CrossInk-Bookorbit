---
title: Data Cache
nav_order: 16
---

# Data Cache

CrossInk caches data aggressively on the SD card to minimize RAM use. The ESP32-C3 has about 380 KB of usable RAM, so rebuilding every book structure in memory on every open would be too expensive.

The main data directory is `.crosspoint` on the SD card. It stores render caches and persistent user/device data.

## Directory Layout

```text
.crosspoint/
├── global_stats.bin        # All-time reading stats, including total books read
├── global_stats.bin.bak    # Backup used if the main global stats file is corrupt
├── synced_stats/           # Stats snapshots received from other readers
├── crossink-settings.json  # CrossInk device settings
├── settings.json           # Legacy settings fallback, if present
├── settings.bin.bak        # Legacy binary settings file after migration, if present
├── state.json              # Last-opened book and sleep/session state
├── state.bin.bak           # Legacy binary state file after migration, if present
├── recent.json             # Recent books list
├── recent.bin.bak          # Legacy binary recent-books file after migration, if present
├── wifi.json               # Saved Wi-Fi networks
├── opds.json               # Saved OPDS servers
├── koreader.json           # KOReader sync credentials
├── bookmarks/              # Bookmark files, one per book
├── home_carousel_cache.bin # Lyra Carousel home-screen snapshot cache
├── sleep_frame.bin         # Temporary sleep overlay framebuffer, when used
├── epub_12471232/          # Each EPUB is cached to epub_<hash>
│   ├── progress.bin        # Reading position (chapter, page, etc.)
│   ├── stats.bin           # Legacy per-book reading stats
│   ├── stats_v5.bin        # Version 5 per-book reading stats
│   ├── reader_settings.bin # Per-book reader settings and auto-page-turn interval
│   ├── cover.bmp           # Book cover image, once generated
│   ├── thumb_*.bmp         # Home/recent-books thumbnail images
│   ├── book.bin            # Book metadata, spine, table of contents, etc.
│   ├── css_rules.cache     # Parsed CSS rules
│   └── sections/           # Pre-rendered chapter/page layout data
│       ├── 0.bin
│       ├── 1.bin
│       └── ...
├── xtc_12471232/           # XTC progress and generated cover/thumb images
└── txt_12471232/           # TXT progress, page index, and generated cover image
```

## Clearing Cache Data

Deleting the entire `.crosspoint` directory resets caches, settings, saved network/server data, bookmarks, recent books, reading progress, and reading stats.

To clear EPUB/XTC render caches from the device UI without deleting settings or global stats, use:

**Settings > System > Files & Cache > Clear Reading Cache**

## Book Moves And Cache Identity

Cache folders are path-based. Moving a book file can create a new cache directory, so the moved copy may start with fresh reading progress unless the firmware migrates the cache for that move. CrossInk migrates cache and bookmark data for the built-in move-to-Read flow and related file-browser move actions.

EPUB reader font, page layout, styling, and reading-aid settings normally come from the global Reader settings. If those settings are changed from inside an EPUB, CrossInk stores a per-book override in that book's `reader_settings.bin`; books without that override continue to follow the global defaults.

Cache data is cleared by supported CrossInk delete/move flows. If you remove or rename books outside CrossInk by editing the SD card directly, old cache folders may remain until you clear reading cache.

All-time reading stats can also be backed up outside `.crosspoint` in:

```text
/.crossink-stats-backup/
```

For binary file layout details, see [File Formats](./file-formats.md).
