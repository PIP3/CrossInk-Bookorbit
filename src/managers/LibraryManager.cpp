#include "LibraryManager.h"

#include <Logging.h>

#include "network/BookOrbitCatalogProvider.h"
#include "stores/LibraryStore.h"
#include "stores/CollectionStore.h"
#include "stores/SmartScopeStore.h"

// Singleton instance
static LibraryManager* instance = nullptr;

LibraryManager::LibraryManager()
    : libraryStore_(&LIBRARY_STORE),
      collectionStore_(&COLLECTION_STORE),
      smartScopeStore_(&SMART_SCOPE_STORE) {}

LibraryManager::~LibraryManager() {
  end();
}

void LibraryManager::begin() {
  // Load all stores
  libraryStore_->ensureLoaded();
  collectionStore_->ensureLoaded();
  smartScopeStore_->ensureLoaded();
  
  LOG_INF("LIB_MGR", "LibraryManager initialized with %d libraries, %d collections, %d smart scopes",
           libraryStore_->getCount(), collectionStore_->getCount(), smartScopeStore_->getCount());
}

void LibraryManager::end() {
  // Release stores
  libraryStore_->release();
  collectionStore_->release();
  smartScopeStore_->release();
  
  LOG_INF("LIB_MGR", "LibraryManager cleaned up");
}

// Library management
bool LibraryManager::addLibrary(const Library& library) {
  if (!library.isValid()) {
    LOG_ERR("LIB_MGR", "Cannot add invalid library");
    return false;
  }
  return libraryStore_->addLibrary(library);
}

bool LibraryManager::updateLibrary(const std::string& libraryId, const Library& library) {
  if (!library.isValid()) {
    LOG_ERR("LIB_MGR", "Cannot update with invalid library");
    return false;
  }
  
  const Library* existing = libraryStore_->getLibraryById(libraryId);
  if (!existing) {
    LOG_ERR("LIB_MGR", "Library %s not found", libraryId.c_str());
    return false;
  }
  
  // Update the library in the store
  for (size_t i = 0; i < libraryStore_->getCount(); ++i) {
    const Library* lib = libraryStore_->getLibrary(i);
    if (lib && lib->id == libraryId) {
      return libraryStore_->updateLibrary(i, library);
    }
  }
  
  return false;
}

bool LibraryManager::removeLibrary(const std::string& libraryId) {
  // First, remove all collections and smart scopes associated with this library
  auto collections = collectionStore_->getCollectionsByLibrary(libraryId);
  for (const auto& collection : collections) {
    collectionStore_->removeCollectionById(collection.id);
  }
  
  auto smartScopes = smartScopeStore_->getSmartScopesByLibrary(libraryId);
  for (const auto& smartScope : smartScopes) {
    smartScopeStore_->removeSmartScopeById(smartScope.id);
  }
  
  // Then remove the library
  return libraryStore_->removeLibraryById(libraryId);
}

const std::vector<Library>& LibraryManager::getLibraries() const {
  return libraryStore_->getLibraries();
}

const Library* LibraryManager::getLibrary(const std::string& libraryId) const {
  return libraryStore_->getLibraryById(libraryId);
}

const Library* LibraryManager::getDefaultLibrary() const {
  return libraryStore_->getDefaultLibrary();
}

bool LibraryManager::setDefaultLibrary(const std::string& libraryId) {
  return libraryStore_->setDefaultLibrary(libraryId);
}

// Catalog provider creation
std::unique_ptr<CatalogProvider> LibraryManager::getCatalogProvider(const Library& library) const {
  return createProviderForLibrary(library);
}

std::unique_ptr<CatalogProvider> LibraryManager::getCatalogProvider(const std::string& libraryId) const {
  const Library* library = getLibrary(libraryId);
  if (!library) {
    LOG_ERR("LIB_MGR", "Library %s not found", libraryId.c_str());
    return nullptr;
  }
  return createProviderForLibrary(*library);
}

std::unique_ptr<CatalogProvider> LibraryManager::createProviderForLibrary(const Library& library) const {
  switch (library.providerType) {
    case Library::ProviderType::BOOKORBIT:
      return std::make_unique<BookOrbitCatalogProvider>(library.id);
    case Library::ProviderType::OPDS:
      // TODO: Implement OpdsCatalogProvider
      LOG_ERR("LIB_MGR", "OPDS provider not yet implemented");
      return nullptr;
    case Library::ProviderType::CALIBRE:
      // TODO: Implement CalibreCatalogProvider
      LOG_ERR("LIB_MGR", "Calibre provider not yet implemented");
      return nullptr;
    case Library::ProviderType::WEBDAV:
      // TODO: Implement WebDAVCatalogProvider
      LOG_ERR("LIB_MGR", "WebDAV provider not yet implemented");
      return nullptr;
    case Library::ProviderType::LOCAL:
      // TODO: Implement LocalCatalogProvider
      LOG_ERR("LIB_MGR", "Local provider not yet implemented");
      return nullptr;
    default:
      LOG_ERR("LIB_MGR", "Unknown provider type: %d", static_cast<int>(library.providerType));
      return nullptr;
  }
}

// Collection management
bool LibraryManager::addCollection(const Collection& collection) {
  if (!collection.isValid()) {
    LOG_ERR("LIB_MGR", "Cannot add invalid collection");
    return false;
  }
  
  if (!ensureLibraryExists(collection.libraryId)) {
    LOG_ERR("LIB_MGR", "Library %s does not exist", collection.libraryId.c_str());
    return false;
  }
  
  return collectionStore_->addCollection(collection);
}

bool LibraryManager::updateCollection(const std::string& collectionId, const Collection& collection) {
  if (!collection.isValid()) {
    LOG_ERR("LIB_MGR", "Cannot update with invalid collection");
    return false;
  }
  
  if (!ensureLibraryExists(collection.libraryId)) {
    LOG_ERR("LIB_MGR", "Library %s does not exist", collection.libraryId.c_str());
    return false;
  }
  
  const Collection* existing = collectionStore_->getCollectionById(collectionId);
  if (!existing) {
    LOG_ERR("LIB_MGR", "Collection %s not found", collectionId.c_str());
    return false;
  }
  
  for (size_t i = 0; i < collectionStore_->getCount(); ++i) {
    const Collection* col = collectionStore_->getCollection(i);
    if (col && col->id == collectionId) {
      return collectionStore_->updateCollection(i, collection);
    }
  }
  
  return false;
}

bool LibraryManager::removeCollection(const std::string& collectionId) {
  // First, remove all child collections
  auto children = collectionStore_->getChildCollections(collectionId);
  for (const auto& child : children) {
    collectionStore_->removeCollectionById(child.id);
  }
  
  return collectionStore_->removeCollectionById(collectionId);
}

const std::vector<Collection> LibraryManager::getCollections(const std::string& libraryId) const {
  if (!ensureLibraryExists(libraryId)) {
    LOG_ERR("LIB_MGR", "Library %s does not exist", libraryId.c_str());
    return {};
  }
  return collectionStore_->getCollectionsByLibrary(libraryId);
}

const Collection* LibraryManager::getCollection(const std::string& collectionId) const {
  return collectionStore_->getCollectionById(collectionId);
}

// Smart scope management
bool LibraryManager::addSmartScope(const SmartScope& smartScope) {
  if (!smartScope.isValid()) {
    LOG_ERR("LIB_MGR", "Cannot add invalid smart scope");
    return false;
  }
  
  if (!ensureLibraryExists(smartScope.libraryId)) {
    LOG_ERR("LIB_MGR", "Library %s does not exist", smartScope.libraryId.c_str());
    return false;
  }
  
  return smartScopeStore_->addSmartScope(smartScope);
}

bool LibraryManager::updateSmartScope(const std::string& smartScopeId, const SmartScope& smartScope) {
  if (!smartScope.isValid()) {
    LOG_ERR("LIB_MGR", "Cannot update with invalid smart scope");
    return false;
  }
  
  if (!ensureLibraryExists(smartScope.libraryId)) {
    LOG_ERR("LIB_MGR", "Library %s does not exist", smartScope.libraryId.c_str());
    return false;
  }
  
  const SmartScope* existing = smartScopeStore_->getSmartScopeById(smartScopeId);
  if (!existing) {
    LOG_ERR("LIB_MGR", "Smart scope %s not found", smartScopeId.c_str());
    return false;
  }
  
  for (size_t i = 0; i < smartScopeStore_->getCount(); ++i) {
    const SmartScope* scope = smartScopeStore_->getSmartScope(i);
    if (scope && scope->id == smartScopeId) {
      return smartScopeStore_->updateSmartScope(i, smartScope);
    }
  }
  
  return false;
}

bool LibraryManager::removeSmartScope(const std::string& smartScopeId) {
  return smartScopeStore_->removeSmartScopeById(smartScopeId);
}

const std::vector<SmartScope> LibraryManager::getSmartScopes(const std::string& libraryId) const {
  if (!ensureLibraryExists(libraryId)) {
    LOG_ERR("LIB_MGR", "Library %s does not exist", libraryId.c_str());
    return {};
  }
  return smartScopeStore_->getSmartScopesByLibrary(libraryId);
}

const SmartScope* LibraryManager::getSmartScope(const std::string& smartScopeId) const {
  return smartScopeStore_->getSmartScopeById(smartScopeId);
}

// Fetch books from a collection
bool LibraryManager::fetchCollectionBooks(
    const std::string& collectionId, size_t offset, size_t limit,
    std::vector<Book>& outBooks, size_t& outTotal) {

  const Collection* collection = getCollection(collectionId);
  if (!collection) {
    LOG_ERR("LIB_MGR", "Collection %s not found", collectionId.c_str());
    return false;
  }

  auto provider = getCatalogProvider(collection->libraryId);
  if (!provider) {
    LOG_ERR("LIB_MGR", "Failed to create provider for library %s", collection->libraryId.c_str());
    return false;
  }

  return provider->fetchCollectionBooks(*collection, offset, limit, outBooks, outTotal);
}

// Fetch books from a smart scope
bool LibraryManager::fetchSmartScopeBooks(
    const std::string& smartScopeId, size_t offset, size_t limit,
    std::vector<Book>& outBooks, size_t& outTotal) {

  const SmartScope* smartScope = getSmartScope(smartScopeId);
  if (!smartScope) {
    LOG_ERR("LIB_MGR", "Smart scope %s not found", smartScopeId.c_str());
    return false;
  }

  auto provider = getCatalogProvider(smartScope->libraryId);
  if (!provider) {
    LOG_ERR("LIB_MGR", "Failed to create provider for library %s", smartScope->libraryId.c_str());
    return false;
  }

  return provider->fetchSmartScopeBooks(*smartScope, offset, limit, outBooks, outTotal);
}

// Search for books in a library
bool LibraryManager::searchBooks(
    const std::string& libraryId, const std::string& query,
    size_t offset, size_t limit,
    std::vector<Book>& outBooks, size_t& outTotal) {

  auto provider = getCatalogProvider(libraryId);
  if (!provider) {
    LOG_ERR("LIB_MGR", "Failed to create provider for library %s", libraryId.c_str());
    return false;
  }

  return provider->search(query, offset, limit, outBooks, outTotal);
}

// Refresh a library
bool LibraryManager::refreshLibrary(const std::string& libraryId) {
  const Library* library = getLibrary(libraryId);
  if (!library) {
    LOG_ERR("LIB_MGR", "Library %s not found", libraryId.c_str());
    return false;
  }

  auto provider = getCatalogProvider(*library);
  if (!provider) {
    LOG_ERR("LIB_MGR", "Failed to create provider for library %s", libraryId.c_str());
    return false;
  }

  // Fetch root collections
  std::vector<Collection> collections;
  if (!provider->fetchRootCollections(collections)) {
    LOG_ERR("LIB_MGR", "Failed to fetch root collections for library %s", libraryId.c_str());
    return false;
  }

  // Update the collections in the store
  for (const auto& collection : collections) {
    // Check if the collection already exists
    const Collection* existing = collectionStore_->getCollectionById(collection.id);
    if (existing) {
      collectionStore_->updateCollection(
          std::distance(collectionStore_->getCollections().begin(), 
                        std::find(collectionStore_->getCollections().begin(), 
                                  collectionStore_->getCollections().end(), *existing)),
          collection);
    } else {
      collectionStore_->addCollection(collection);
    }
  }

  // Fetch smart scopes
  std::vector<SmartScope> smartScopes;
  if (provider->supportsSmartScopes() && provider->fetchSmartScopes(smartScopes)) {
    for (const auto& smartScope : smartScopes) {
      // Check if the smart scope already exists
      const SmartScope* existing = smartScopeStore_->getSmartScopeById(smartScope.id);
      if (existing) {
        smartScopeStore_->updateSmartScope(
            std::distance(smartScopeStore_->getSmartScopes().begin(),
                          std::find(smartScopeStore_->getSmartScopes().begin(),
                                    smartScopeStore_->getSmartScopes().end(), *existing)),
            smartScope);
      } else {
        smartScopeStore_->addSmartScope(smartScope);
      }
    }
  }

  // Update the library's last synced timestamp
  Library updatedLibrary = *library;
  updatedLibrary.lastSynced = millis();  // TODO: Use a proper timestamp
  updateLibrary(libraryId, updatedLibrary);

  LOG_INF("LIB_MGR", "Refreshed library %s: %d collections, %d smart scopes",
           libraryId.c_str(), collections.size(), smartScopes.size());
  return true;
}

// Refresh all libraries
bool LibraryManager::refreshAllLibraries() {
  bool allSuccess = true;
  for (const auto& library : getLibraries()) {
    if (!refreshLibrary(library.id)) {
      LOG_ERR("LIB_MGR", "Failed to refresh library %s", library.id.c_str());
      allSuccess = false;
    }
  }
  return allSuccess;
}

// Get total book count
size_t LibraryManager::getTotalBookCount() const {
  size_t total = 0;
  for (const auto& library : getLibraries()) {
    // TODO: Cache the book count per library
    // For now, return a placeholder
  }
  return total;
}

// Helper to check if a library exists
bool LibraryManager::ensureLibraryExists(const std::string& libraryId) const {
  return getLibrary(libraryId) != nullptr;
}

// Singleton accessor
LibraryManager& getLibraryManager() {
  if (!instance) {
    instance = new LibraryManager();
    instance->begin();
  }
  return *instance;
}
