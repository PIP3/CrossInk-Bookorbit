# Dictionary

## Supported Format

The reader supports **StarDict** dictionaries. When searching for dictionaries online, look for "StarDict format" or files with `.dict`, `.idx`, and `.ifo` extensions.

A dictionary folder typically contains:

- `.dict` -- definition data (required; compressed `.dict.dz` files must be prepared on a computer first)
- `.idx` -- word index (required)
- `.ifo` -- metadata such as dictionary name and word count (recommended)
- `.syn` -- alternate forms and synonyms (optional, enhances lookup coverage)

Minimum requirement: one `.dict` and one `.idx` file in the same folder. Their filename stems should match. Without `.ifo`, the dictionary will still work but metadata and HTML definition rendering may be limited.

---

## Setting Up a Dictionary

1. Copy your dictionary folder(s) to one of these directories on the SD card:
   - `/.dictionaries/` (checked first)
   - `/dictionaries/`
2. If only one dictionary is found and no dictionary has been selected before, it is selected automatically.
3. To choose between multiple dictionaries, open **Settings -> Dictionary** on the device and select one from the list.

To deselect the current dictionary, select **None** from the list.

### Preparing Compressed or Large Dictionaries

CrossInk does not decompress dictionaries on the device. If a download contains `.dict.dz` or `.syn.dz`, run the bundled preparation tool on a computer before copying the folder to the SD card (see [Offline Dictionary Tools](#offline-dictionary-tools)).

An uncompressed dictionary with only `.dict` and `.idx` files is usable without desktop preparation. On the first lookup, CrossInk generates a small `.qidx` sidecar next to `.idx`; later lookups use it to jump close to the requested word. The sidecar is a disposable cache and can be deleted safely. CrossInk rebuilds it when it is missing, invalid, or records a different `.idx` size.

Desktop-generated `.oft` and `.cspt` files remain the fastest lookup path and take priority over `.qidx`. If CrossInk cannot write the sidecar, the dictionary remains usable through a slower full-index scan.

---

## Looking Up a Word

The **Look Up Word** option in the reader menu is only visible when a dictionary is active.

1. Open the reader menu and choose **Look Up Word**.
2. The page becomes a word-select overlay - one word is highlighted, initially near the centre of the page.
3. Use **Up/Down** to move between rows, **Left/Right** to move between words on the same row.
4. Press **Confirm** to look up the highlighted word.
5. Press **Back** to exit word-select without looking anything up.

### Quick Lookup

Set a shortcut to **Look Up Word** in **Settings -> Controls**. Holding that shortcut in the EPUB reader then enters word-select directly, skipping the reader menu. Release and navigate to a word as usual.

### How Lookup Works

When you select a word, the reader searches for it in this order:

1. **Direct match** - the word is found as-is in the dictionary index.
2. **Stemming** - the reader automatically tries common word forms (plurals, verb conjugations, comparatives). For example, "running" finds "run".
3. **Alternate forms** - if the dictionary includes a synonym/alternate forms file and no match was found yet, a prompt appears. Press **Confirm** to search alternate forms, or **Back** to skip.
4. **Suggestions** - if nothing matched, a list of similar words from the dictionary is shown. Select one to view its definition.
5. **Not found** - if no matches or suggestions exist, a not-found message appears. Press **Back** to return to word-select, or **Confirm** to exit to the reader.

---

## The Definition Screen

When a word is found, the definition screen shows the headword at the top and the definition text below.

- **Page Forward** - next page (for long definitions)
- **Page Back** - previous page
- **Confirm** (labelled **Look Up Word**) - enter word-select mode on the definition text (see Chaining Lookups below)
- **Left** (labelled **Switch**) - choose another dictionary and repeat the current lookup
- **Back** (short press) - return to the previous screen
- **Back** (long press) - exit all the way back to the reader

---

## Chaining Lookups

From a definition screen, you can look up any word within the definition text without returning to the reader.

1. Press **Confirm** (**Look Up Word**) on the definition screen.
2. A word in the definition becomes highlighted. Navigate to any word and press **Confirm**.
3. A new definition screen opens for that word.
4. You can chain further by pressing **Look Up Word** again from the new definition.
5. Short-press **Back** to exit word-select and return to the current definition.
6. Short-press **Back** again to go back through the chain (each press returns to the previous definition).
7. Long-press **Back** at any point to exit directly to the reader.

Going back returns you to each prior definition **on the page you were reading** when you chained away from it, not the first page.

**Chain depth limit:** the chain follows the 50 entries visible in lookup history. If you chain deeper than that, the oldest chain entries are dropped, so backing out eventually returns you to the reader.

---

## Phrase / Multi-word Lookup

In word-select mode, you can select a sequence of words to look up as a phrase.

1. Navigate to the first word of the phrase.
2. Long-press **Confirm** to anchor on that word.
3. Use the navigation buttons to extend (or shrink) the selection to cover the full phrase. All selected words are highlighted.
4. Short-press **Confirm** to look up the selected phrase.
5. Press **Back** to cancel and return to single-word select mode.

Multi-word select works in both the reader word-select and the definition word-select (chained lookup).

**Limitation:** Multi-word selection cannot span a page boundary. If a phrase crosses from one page to the next, only the words on the current page are available for selection. As a workaround, reduce the reader font size so more words fit on a single page, perform the lookup, then restore the original font size.

---

## Per-Book Dictionary

Each book can have its own dictionary, independent of the global setting.

1. Open the reader menu, navigate to the 3rd tab (settings gear icon) and choose **Book Dictionary**.
2. Select a dictionary from the picker.
3. The per-book choice is saved and restored each time the book is opened.
4. To remove the override, open **Book Dictionary** and select **Use Global**. The picker shows the current global dictionary name in parentheses next to Use Global.

The global dictionary is automatically restored whenever the book is closed. Changing a book's dictionary does not affect the global setting or other books.

---

## Lookup History

Each book maintains its own lookup history, accessible from the reader menu.

1. Open the reader menu and choose **Lookup History**.
2. Each entry shows the searched word and a status indicator:
   - Square root (√) -- found directly
   - Tilde (~) -- resolved via stemming or alternate forms
   - Question mark (?) -- reached via the suggestions screen
   - X -- not found
3. Select any entry and press **Confirm** to look it up again. Press **Back** from the definition to return to the history screen.
4. To delete an entry, long-press **Confirm** on it. A confirmation popup appears -- press **Confirm** to delete, or **Back** to cancel.

The history screen shows the 50 most recent entries. The on-disk history is append-only, so older entries remain stored but are not shown. The visible window also bounds how deep a chained lookup can go back (see Chaining Lookups).

---

## IPA Phonetic Characters

Dictionary definitions can render common IPA phonetic characters (pronunciation symbols like /ˈæp.əl/) using the built-in definition font.

If you see a filled diamond where a pronunciation symbol should appear, that character is outside the supported IPA range.

---

## Offline Dictionary Tools

A command-line tool is included for working with StarDict dictionaries on your computer, without the device. It requires Python 3 and has no external dependencies.

### Pre-processing

Decompress dictionary data and generate accelerator indexes on your computer:

```bash
python3 scripts/dictionary_tools.py prep /path/to/dictionary-folder
```

Run this before copying compressed dictionaries to the SD card. It is also recommended for large uncompressed dictionaries because it makes lookups much faster.

The command produces, when applicable, `.dict`, `.syn`, `.idx.oft`, `.syn.oft`, `.idx.oft.cspt`, and `.syn.oft.cspt`. Copy these alongside the original `.idx` and optional `.ifo`. CrossInk uses the `.cspt` index first, falls back to `.oft`, and finally scans the original index if neither accelerator exists.

### Looking Up a Word

Look up a word from the command line:

```bash
python3 scripts/dictionary_tools.py lookup /path/to/dictionary-folder apple
```

Prints the definition to stdout. Compressed dictionary data must be prepared with `prep` first.

### Merging Dictionaries

Combine two or more StarDict dictionaries into a single monolithic dictionary:

```bash
python3 scripts/dictionary_tools.py merge \
  --source /path/to/dict-a \
  --source /path/to/dict-b \
  --output /path/to/merged-dict
```

Specify `--source` once per dictionary to include. The merged output contains the full union of all headwords and synonyms. When the same word appears in multiple sources, definitions are concatenated in source order.

Source dictionaries must be prepared (decompressed `.dict` files) before merging. The output is a complete, ready-to-use StarDict dictionary that can be copied directly to the SD card.
