#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "network/HttpDownloader.h"

// Forward declarations
struct Book;
struct Collection;
struct SmartScope;
struct Library;

/**
 * Abstract base class for catalog providers (BookOrbit, OPDS, Calibre, WebDAV).
 * Provides a unified interface for browsing and downloading books from various sources.
 */
class CatalogProvider {
 public:
  virtual ~CatalogProvider() = default;

  /**
   * Connect to the catalog provider.
   * @return true if connection succeeded, false otherwise.
   */
  virtual bool connect() = 0;

  /**
   * Disconnect from the catalog provider.
   */
  virtual void disconnect() = 0;

  /**
   * Check if the provider is currently connected.
   * @return true if connected, false otherwise.
   */
  virtual bool isConnected() const = 0;

  /**
   * Fetch the root-level collections for this catalog.
   * @param outCollections Vector to populate with collections.
   * @return true if fetch succeeded, false otherwise.
   */
  virtual bool fetchRootCollections(std::vector<Collection>& outCollections) = 0;

  /**
   * Fetch the books in a specific collection.
   * @param collection The collection to fetch books from.
   * @param offset Pagination offset.
   * @param limit Maximum number of books to fetch.
   * @param outBooks Vector to populate with books.
   * @param outTotal Total number of books in the collection (for pagination).
   * @return true if fetch succeeded, false otherwise.
   */
  virtual bool fetchCollectionBooks(
      const Collection& collection, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal) = 0;

  /**
   * Fetch smart scopes available for this catalog.
   * @param outSmartScopes Vector to populate with smart scopes.
   * @return true if fetch succeeded, false otherwise.
   */
  virtual bool fetchSmartScopes(std::vector<SmartScope>& outSmartScopes) = 0;

  /**
   * Fetch the books matching a smart scope's query.
   * @param smartScope The smart scope to fetch books for.
   * @param offset Pagination offset.
   * @param limit Maximum number of books to fetch.
   * @param outBooks Vector to populate with books.
   * @param outTotal Total number of books matching the scope.
   * @return true if fetch succeeded, false otherwise.
   */
  virtual bool fetchSmartScopeBooks(
      const SmartScope& smartScope, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal) = 0;

  /**
   * Search for books in this catalog.
   * @param query The search query.
   * @param offset Pagination offset.
   * @param limit Maximum number of books to fetch.
   * @param outBooks Vector to populate with books.
   * @param outTotal Total number of books matching the query.
   * @return true if search succeeded, false otherwise.
   */
  virtual bool search(
      const std::string& query, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal) = 0;

  /**
   * Download a book from this catalog.
   * @param book The book to download.
   * @param destPath Destination path on the SD card.
   * @param progress Optional progress callback.
   * @param cancelFlag Optional flag to cancel the download.
   * @param options Download options (e.g., overwrite, resume).
   * @return DownloadError code.
   */
  virtual HttpDownloader::DownloadError downloadBook(
      const Book& book, const std::string& destPath,
      HttpDownloader::ProgressCallback progress = nullptr,
      bool* cancelFlag = nullptr,
      HttpDownloader::DownloadOptions options = HttpDownloader::DownloadOptions()) = 0;

  /**
   * Get the name of this catalog provider (for display purposes).
   * @return Provider name.
   */
  virtual std::string getName() const = 0;

  /**
   * Get the type of this catalog provider (e.g., "BookOrbit", "OPDS", "Calibre").
   * @return Provider type.
   */
  virtual std::string getType() const = 0;

  /**
   * Check if this provider supports smart scopes.
   * @return true if smart scopes are supported, false otherwise.
   */
  virtual bool supportsSmartScopes() const { return false; }

  /**
   * Check if this provider supports collections.
   * @return true if collections are supported, false otherwise.
   */
  virtual bool supportsCollections() const { return true; }

  /**
   * Check if this provider supports search.
   * @return true if search is supported, false otherwise.
   */
  virtual bool supportsSearch() const { return true; }
};
