#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "network/HttpDownloader.h"

/**
 * A top-level BookOrbit catalog section (e.g. "Recently added", "Continue reading",
 * "All books"). Only the sections backed directly by a book listing are exposed here;
 * BookOrbit's library/collection/smart-scope/author/series browsing facets are not
 * supported, matching CrossInk's simple, single-level browse experience for OPDS.
 */
struct BookOrbitCatalogSection {
  std::string id;
  std::string title;
};

/** A single downloadable file attached to a BookOrbit book. */
struct BookOrbitCatalogFile {
  int64_t id = 0;
  std::string format;  // e.g. "epub", lowercase
  size_t sizeBytes = 0;
  std::string devicePath;  // Server-provided path like "Series Name/01 - Title.epub"
};

/** One entry in a BookOrbit book listing. */
struct BookOrbitCatalogBook {
  int64_t id = 0;
  std::string title;
  std::string author;  // first author only, for compact list display
  std::string devicePath;  // Server-provided path for device path feature
};

/** One entry in a drill-down facet listing (an author or a series). */
struct BookOrbitFacetEntry {
  std::string id;        // filter value for the books listing
  std::string title;     // display name
  std::string seriesId;  // numeric series id when the server provides one (series only)
  int count = 0;         // number of books, 0 when the server omits it
};

/** Result of a paged facet listing request (authors or series). */
struct BookOrbitFacetPage {
  std::vector<BookOrbitFacetEntry> entries;
  int page = 1;
  bool hasNext = false;
};

/** Filters for a book listing request; empty fields are omitted from the query. */
struct BookOrbitBookQuery {
  std::string sort;      // BookOrbit sort id (e.g. "recently_added", "title", "series")
  std::string query;     // free-text search
  std::string author;    // author filter (facet entry id)
  std::string seriesId;  // numeric series filter (preferred when present)
  std::string series;    // series-name filter (fallback when no seriesId)
};

/** Full detail for a single BookOrbit book, including its downloadable files. */
struct BookOrbitBookDetail {
  int64_t id = 0;
  std::string title;
  std::string author;
  std::vector<BookOrbitCatalogFile> files;
};

/** Result of a paged book listing request. */
struct BookOrbitBookPage {
  std::vector<BookOrbitCatalogBook> books;
  int page = 1;
  int total = 0;
  int pageSize = 0;
};

/** A BookOrbit library, containing collections and books. */
struct BookOrbitLibrary {
  int64_t id = 0;
  std::string name;
  std::string description;
  bool isDefault = false;
  int bookCount = 0;
};

/** A BookOrbit collection, which can be static or dynamic (smart scope). */
struct BookOrbitCollection {
  int64_t id = 0;
  std::string name;
  std::string description;
  int64_t parentId = 0;  // 0 for root-level collections
  bool isSmartScope = false;
  std::string smartScopeQuery;  // Query for smart scopes
  int bookCount = 0;
};

/** Result of a paged library listing request. */
struct BookOrbitLibraryPage {
  std::vector<BookOrbitLibrary> libraries;
  int page = 1;
  int total = 0;
  int pageSize = 0;
};

/** Result of a paged collection listing request. */
struct BookOrbitCollectionPage {
  std::vector<BookOrbitCollection> collections;
  int page = 1;
  int total = 0;
  int pageSize = 0;
};

/**
 * HTTP client for BookOrbit's KOReader-authenticated JSON catalog endpoints
 * (browsing and downloading books). Uses the same x-auth-user/x-auth-key headers
 * as BookOrbitSyncClient; unlike the sync client, catalog requests go through
 * HttpDownloader since responses can be larger than a fixed-size buffer and file
 * downloads must stream straight to the SD card. Lives under src/network (rather
 * than lib/BookOrbitSync) because it depends on HttpDownloader, an app-level
 * (src/) utility.
 *
 * Supports fetching root sections, books, and downloading EPUB files.
 * Extended to support libraries, collections, and smart scopes for enhanced browsing.
 */
class BookOrbitCatalogClient {
 public:
  static constexpr int PAGE_SIZE = 20;

  // True when the last failed fetch reached a server but got a non-catalog reply
  // (HTML page, invalid JSON...) — typically a BookOrbit version without the
  // KOReader catalog API, or a proxy serving the web app at the API path. Lets the
  // UI distinguish "check your connection" from "check your server".
  static bool lastFetchBadResponse;

  /** Fetch the catalog root sections list. Returns false on any failure. */
  static bool fetchRootSections(std::vector<BookOrbitCatalogSection>& outSections);

  /**
   * Fetch a page of books matching the given filters.
   * @param query Sort and filter fields; empty fields are omitted
   * @param page 1-based page number
   */
  static bool fetchBooks(const BookOrbitBookQuery& query, int page, BookOrbitBookPage& outPage);

  /**
   * Fetch a page of a drill-down facet listing ("authors" or "series").
   * @param sectionId The facet section id from the catalog root
   * @param page 1-based page number
   */
  static bool fetchSectionEntries(const std::string& sectionId, int page, BookOrbitFacetPage& outPage);

  /** Fetch full detail (including downloadable files) for one book. */
  static bool fetchBookDetail(int64_t bookId, BookOrbitBookDetail& outDetail);

  /** Download a catalog file (by file id, not book id) to the SD card. */
  static HttpDownloader::DownloadError downloadFile(
      int64_t fileId, const std::string& destPath, HttpDownloader::ProgressCallback progress = nullptr,
      bool* cancelFlag = nullptr, HttpDownloader::DownloadOptions options = HttpDownloader::DownloadOptions());

  /**
   * Download a catalog file using server-provided devicePath for organization.
   * If useDevicePath is enabled in settings and devicePath is available from the server,
   * the file will be saved at baseDestPath/devicePath with parent directories created as needed.
   * Otherwise, falls back to baseDestPath.
   */
  static HttpDownloader::DownloadError downloadFileWithDetail(
      int64_t fileId, const BookOrbitBookDetail& detail, const std::string& baseDestPath,
      HttpDownloader::ProgressCallback progress = nullptr, bool* cancelFlag = nullptr,
      HttpDownloader::DownloadOptions options = HttpDownloader::DownloadOptions());

  /**
   * Fetch the list of libraries available to the user.
   * @param outLibraries Vector to populate with libraries.
   * @return true if fetch succeeded, false otherwise.
   */
  static bool fetchLibraries(std::vector<BookOrbitLibrary>& outLibraries);

  /**
   * Fetch the root-level collections for a library.
   * @param libraryId The ID of the library to fetch collections from.
   * @param outCollections Vector to populate with collections.
   * @return true if fetch succeeded, false otherwise.
   */
  static bool fetchLibraryCollections(int64_t libraryId, std::vector<BookOrbitCollection>& outCollections);

  /**
   * Fetch child collections for a parent collection.
   * @param parentId The ID of the parent collection.
   * @param outCollections Vector to populate with child collections.
   * @return true if fetch succeeded, false otherwise.
   */
  static bool fetchCollectionChildren(int64_t parentId, std::vector<BookOrbitCollection>& outCollections);

  /**
   * Fetch smart scopes for a library.
   * @param libraryId The ID of the library.
   * @param outCollections Vector to populate with smart scopes (as collections).
   * @return true if fetch succeeded, false otherwise.
   */
  static bool fetchSmartScopes(int64_t libraryId, std::vector<BookOrbitCollection>& outCollections);

  /**
   * Fetch books from a specific collection.
   * @param collectionId The ID of the collection.
   * @param page 1-based page number.
   * @param outPage Result page with books.
   * @return true if fetch succeeded, false otherwise.
   */
  static bool fetchCollectionBooks(int64_t collectionId, int page, BookOrbitBookPage& outPage);

  /**
   * Fetch books matching a smart scope query.
   * @param smartScopeQuery The smart scope query string.
   * @param page 1-based page number.
   * @param outPage Result page with books.
   * @return true if fetch succeeded, false otherwise.
   */
  static bool fetchSmartScopeBooks(const std::string& smartScopeQuery, int page, BookOrbitBookPage& outPage);
};
