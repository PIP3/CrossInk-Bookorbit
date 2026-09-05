#pragma once

#include <memory>
#include <string>
#include <vector>

#include "models/CatalogModels.h"
#include "network/CatalogProvider.h"

// Forward declarations
class LibraryStore;
class CollectionStore;
class SmartScopeStore;

/**
 * Manages libraries and their associated catalog providers.
 * Provides a high-level interface for accessing libraries, collections, and smart scopes.
 */
class LibraryManager {
 public:
  LibraryManager();
  ~LibraryManager();

  // Initialize the manager (load stores)
  void begin();

  // Clean up resources
  void end();

  // Library management
  bool addLibrary(const Library& library);
  bool updateLibrary(const std::string& libraryId, const Library& library);
  bool removeLibrary(const std::string& libraryId);
  const std::vector<Library>& getLibraries() const;
  const Library* getLibrary(const std::string& libraryId) const;
  const Library* getDefaultLibrary() const;
  bool setDefaultLibrary(const std::string& libraryId);

  // Get a catalog provider for a library
  std::unique_ptr<CatalogProvider> getCatalogProvider(const Library& library) const;
  std::unique_ptr<CatalogProvider> getCatalogProvider(const std::string& libraryId) const;

  // Collection management
  bool addCollection(const Collection& collection);
  bool updateCollection(const std::string& collectionId, const Collection& collection);
  bool removeCollection(const std::string& collectionId);
  const std::vector<Collection> getCollections(const std::string& libraryId) const;
  const Collection* getCollection(const std::string& collectionId) const;

  // Smart scope management
  bool addSmartScope(const SmartScope& smartScope);
  bool updateSmartScope(const std::string& smartScopeId, const SmartScope& smartScope);
  bool removeSmartScope(const std::string& smartScopeId);
  const std::vector<SmartScope> getSmartScopes(const std::string& libraryId) const;
  const SmartScope* getSmartScope(const std::string& smartScopeId) const;

  // Fetch books from a collection
  bool fetchCollectionBooks(
      const std::string& collectionId, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal);

  // Fetch books from a smart scope
  bool fetchSmartScopeBooks(
      const std::string& smartScopeId, size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal);

  // Search for books in a library
  bool searchBooks(
      const std::string& libraryId, const std::string& query,
      size_t offset, size_t limit,
      std::vector<Book>& outBooks, size_t& outTotal);

  // Refresh a library (update collections and smart scopes)
  bool refreshLibrary(const std::string& libraryId);

  // Refresh all libraries
  bool refreshAllLibraries();

  // Get the total number of books across all libraries
  size_t getTotalBookCount() const;

 private:
  // Prevent copying
  LibraryManager(const LibraryManager&) = delete;
  LibraryManager& operator=(const LibraryManager&) = delete;

  // Helper methods
  std::unique_ptr<CatalogProvider> createProviderForLibrary(const Library& library) const;
  bool ensureLibraryExists(const std::string& libraryId) const;

  // Member variables
  LibraryStore* libraryStore_;
  CollectionStore* collectionStore_;
  SmartScopeStore* smartScopeStore_;
};

// Singleton accessor
LibraryManager& getLibraryManager();
