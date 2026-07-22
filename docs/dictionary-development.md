# Dictionary Development Guide

This guide describes the dictionary implementation currently shipped on this branch. For installation and use, see [dictionary.md](dictionary.md).

## Supported StarDict Files

| File | Required | Purpose |
|------|----------|---------|
| `.dict` | Yes | Uncompressed definition data |
| `.idx` | Yes | Sorted headwords and offsets into `.dict` |
| `.ifo` | Recommended | Metadata and `sametypesequence` used to interpret definition fields |
| `.syn` | Optional | Alternate forms mapped to `.idx` ordinals |
| `.idx.oft` / `.syn.oft` | Optional | Coarse page offsets used to narrow a scan |
| `.idx.oft.cspt` / `.syn.oft.cspt` | Optional | CrossInk prefix indexes used as the fastest lookup path |
| `.qidx` | Generated | Disposable sampled index built by the device when `.idx` has no prepared accelerator |

The device requires an uncompressed `.dict`; it does not read `.dict.dz` or `.syn.dz` directly. Use `scripts/dictionary_tools.py prep` on a computer when decompression is needed or to generate the fastest `.oft`/`.cspt` accelerators. For an uncompressed dictionary without those accelerators, the device automatically generates `.qidx` on first lookup.

Dictionary discovery is implemented by `DictionaryRegistry`. It checks `/.dictionaries` first and `/dictionaries` second, using only the first root directory found. Each child folder must contain exactly one `.idx`, no more than one `.ifo`, and a `.dict` file. Hidden folders and ambiguous folders are skipped.

## Lookup Paths

`Dictionary::locate` searches `.idx` in this order:

1. Use `.idx.oft.cspt` to select a small byte range.
2. Fall back to `.idx.oft` to select an offset page.
3. Fall back to the device-generated `.qidx` sampled index.
4. Fall back to scanning `.idx` from the beginning if the sidecar cannot be read or written.

`Dictionary::resolveAltForm` uses `.syn.oft.cspt`, then `.syn.oft`, then a full `.syn` scan; `.qidx` accelerates the main `.idx` only. `Dictionary::findSimilar` uses `.idx.oft` when available; without it, suggestion generation scans a much larger range. Missing accelerator files affect speed, not correctness, as long as the uncompressed `.dict` and `.idx` exist.

`.qidx` uses a 20-byte little-endian header containing `QIDX`, format version, sample interval, sample count, and source `.idx` size, followed by the byte offset of every 256th `.idx` entry. It is written through a temporary file and installed only after the complete scan succeeds. A size mismatch or invalid header causes it to be rebuilt.

StarDict files with `idxoffsetbits=64` are parsed, but entries whose definition offset exceeds the device's supported 32-bit range fail safely.

## Definition Rendering

`DictionaryDefinitionActivity` resolves the selected `.dict` byte range and renders one page at a time. `DictHtmlRenderer` streams HTML input, while `DictLayout::Wrapper` wraps styled spans into page lines. Keeping only the current page bounds peak RAM and avoids materializing a large definition as one in-memory document. Definitions currently use the fixed built-in `UI_12_FONT_ID`; there are no separate definition font settings.

Paging re-parses the definition from its start. This trades extra sequential reads for predictable memory usage on the ESP32-C3.

Chained lookups use `LookupChain`, which stores compact history positions and page numbers instead of owned copies of every headword. The chain is bounded by `LookupHistory::MAX_VISIBLE_ENTRIES` (currently 50).

## Lookup History

Each EPUB cache stores history at `<cachePath>/dictionary_history.txt`. Lines use `word|STATUS`, where the status is direct, stemmed, alternate form, suggestion, or not found.

The file is append-only and is not automatically truncated. The UI loads only the newest 50 entries. Cache-clear helpers preserve this file as user state.

## Offline CLI

The standard-library-only tool is `scripts/dictionary_tools.py`:

```bash
# Decompress .dict.dz/.syn.dz and generate .oft/.cspt files.
python3 scripts/dictionary_tools.py prep /path/to/dictionary-folder

# Perform an exact lookup.
python3 scripts/dictionary_tools.py lookup /path/to/dictionary-folder apple

# Merge prepared dictionaries.
python3 scripts/dictionary_tools.py merge \
  --source /path/to/dict-a \
  --source /path/to/dict-b \
  --output /path/to/merged-dict
```

`lookup` and `merge` require an uncompressed `.dict`. `merge` writes a prepared output including applicable `.oft` and `.cspt` files.

## CrossInk Prefix Index (`.cspt`)

Both `.idx.oft.cspt` and `.syn.oft.cspt` use the same format:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic `CSPT` |
| 4 | 1 | Version (`1`) |
| 5 | 1 | Prefix length (`16`) |
| 6 | 2 | Producer stride (`16`, little-endian) |
| 8 | 4 | Entry count (little-endian) |
| 12... | 20 each | 16-byte, zero-padded prefix plus 4-byte source offset |

The reader performs a case-insensitive binary search for the last prefix less than or equal to the target, then scans only until the next recorded source offset. Invalid or missing `.cspt` data falls back to `.oft`, then to a full scan.

The header's stride field is currently informational. Producers must continue to emit `16` until the format version and readers are updated together.

## Verification

There is no dictionary-specific host fixture suite in this branch. For changes:

1. Run `python3 scripts/dictionary_tools.py prep` and `lookup` against a representative StarDict dictionary.
2. Build the simulator with `pio run -e simulator` for reader/UI integration.
3. On hardware, test dictionaries with and without `.oft`/`.cspt`, a `.syn` dictionary, HTML definitions, long definitions, lookup history, chained lookup, and per-book overrides.

Multi-word selection is limited to the currently rendered page. Reducing the reader or definition font size can fit more of a phrase on one page.
