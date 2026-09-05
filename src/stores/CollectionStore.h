#pragma once

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <string>
#include <vector>

#include "models/CatalogModels.h"

/**
 * Singleton class for storing Collection configurations on the SD card.
 */
class CollectionStore : public PersistableStore<CollectionStore> {
 private:
  std::vector<Collection> collections;
  bool loaded_ = false;

  static constexpr size_t MAX_COLLECTIONS = 128;

  CollectionStore() = default;

  friend class PersistableStore<CollectionStore>;

 public:
  static const char* getFilePath() { return "/.crosspoint/collections.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);
  bool loadFromFile();
  void ensureLoaded() const;
  void release();

  // Add a new collection
  bool addCollection(const Collection& collection);

  // Update an existing collection
  bool updateCollection(size_t index, const Collection& collection);

  // Remove a collection by index
  bool removeCollection(size_t index);

  // Remove a collection by ID
  bool removeCollectionById(const std::string& collectionId);

  // Get all collections
  const std::vector<Collection>& getCollections() const {
    ensureLoaded();
    return collections;
  }

  // Get collections for a specific library
  std::vector<Collection> getCollectionsByLibrary(const std::string& libraryId) const;

  // Get root-level collections (no parent) for a library
  std::vector<Collection> getRootCollectionsByLibrary(const std::string& libraryId) const;

  // Get child collections for a parent collection
  std::vector<Collection> getChildCollections(const std::string& parentId) const;

  // Get a collection by index
  const Collection* getCollection(size_t index) const;

  // Get a collection by ID
  const Collection* getCollectionById(const std::string& collectionId) const;

  // Get the number of collections
  size_t getCount() const {
    ensureLoaded();
    return collections.size();
  }

  // Check if any collections are configured
  bool hasCollections() const {
    ensureLoaded();
    return !collections.empty();
  }

  // Remove collections that reference a non-existent library
  bool pruneOrphanedCollections(const std::vector<std::string>& validLibraryIds);

  // Remove collections that have invalid parent references
  bool pruneInvalidCollections();
};

// Helper macro to access the collection store
#define COLLECTION_STORE CollectionStore::getInstance()
