#pragma once

#include "network/CatalogProvider.h"
#include "network/BookOrbitCatalogClient.h"
#include "models/CatalogModels.h"

/**
 * Catalog provider implementation for BookOrbit.
 * Adapts the existing BookOrbitCatalogClient to the CatalogProvider interface.
 */
class BookOrbitCatalogProvider : public CatalogProvider {
 public:
  explicit BookOrbitCatalogProvider(const std::string& libraryId = "")
      : libraryId_(libraryId) {}

  // CatalogProvider interface
  bool connect() override;
  void disconnect() override;
  bool isConnected() const override;
  bool fetchRootCollections(std::vector<Collection>& outCollections) override;
  bool fetchCollectionBooks(
      const Collection& collection, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal) override;
  bool fetchSmartScopes(std::vector<SmartScope>& outSmartScopes) override;
  bool fetchSmartScopeBooks(
      const SmartScope& smartScope, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal) override;
  bool search(
      const std::string& query, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal) override;
  HttpDownloader::DownloadError downloadBook(
      const Book& book, const std::string& destPath,
      HttpDownloader::ProgressCallback progress = nullptr,
      bool* cancelFlag = nullptr,
      HttpDownloader::DownloadOptions options = HttpDownloader::DownloadOptions()) override;
  std::string getName() const override;
  std::string getType() const override;
  bool supportsSmartScopes() const override { return true; }

 private:
  std::string libraryId_;  // ID of the associated library (for multi-library support)

  // Helper methods
  Book convertToBook(const BookOrbitCatalogBook& catalogBook) const;
  Book convertToBook(const BookOrbitBookDetail& detail) const;
  Collection convertToCollection(const BookOrbitCatalogSection& section) const;
  BookOrbitBookQuery buildQueryFromSmartScope(const SmartScope& smartScope) const;
};
