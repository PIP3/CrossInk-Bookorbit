#include "CollectionStore.h"

#include <HalStorage.h>
#include <Logging.h>

#include "models/CatalogModels.h"

void CollectionStore::toJson(JsonDocument& doc) const {
  JsonArray arr = doc["collections"].to<JsonArray>();
  for (const auto& collection : collections) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = collection.id;
    obj["title"] = collection.title;
    obj["parentId"] = collection.parentId;
    obj["libraryId"] = collection.libraryId;
    obj["isSmartScope"] = collection.isSmartScope;
    obj["bookCount"] = collection.bookCount;
    obj["lastUpdated"] = collection.lastUpdated;
    if (!collection.icon.empty()) {
      obj["icon"] = collection.icon;
    }
    if (!collection.description.empty()) {
      obj["description"] = collection.description;
    }
  }
}

bool CollectionStore::fromJson(JsonVariantConst doc) {
  if (!doc.is<JsonArray>()) {
    LOG_ERR("COL", "Expected JSON array for collections");
    return false;
  }

  collections.clear();
  JsonArrayConst arr = doc.as<JsonArrayConst>();
  collections.reserve(std::min(arr.size(), MAX_COLLECTIONS));
  
  for (JsonObjectConst obj : arr) {
    Collection collection;
    collection.id = obj["id"] | "";
    collection.title = obj["title"] | "";
    collection.parentId = obj["parentId"] | "";
    collection.libraryId = obj["libraryId"] | "";
    collection.isSmartScope = obj["isSmartScope"] | false;
    collection.bookCount = obj["bookCount"] | 0;
    collection.lastUpdated = obj["lastUpdated"] | 0;
    collection.icon = obj["icon"] | "";
    collection.description = obj["description"] | "";
    
    if (collection.isValid()) {
      collections.push_back(std::move(collection));
    }
  }

  return true;
}

bool CollectionStore::loadFromFile() {
  collections.clear();
  loadAttempted_ = true;
  const bool hasStoreFile = Storage.exists(getFilePath());
  if (PersistableStore<CollectionStore>::loadFromFile()) {
    return true;
  }
  if (hasStoreFile) {
    return false;
  }
  return false;
}

void CollectionStore::ensureLoaded() const {
  if (loadAttempted_) return;
  const_cast<CollectionStore*>(this)->loadFromFile();
}

void CollectionStore::release() {
  collections.clear();
  loadAttempted_ = false;
}

bool CollectionStore::addCollection(const Collection& collection) {
  if (!collection.isValid()) {
    LOG_ERR("COL", "Cannot add invalid collection");
    return false;
  }
  
  if (collections.size() >= MAX_COLLECTIONS) {
    LOG_ERR("COL", "Maximum number of collections (%d) reached", MAX_COLLECTIONS);
    return false;
  }
  
  // Check for duplicate ID
  for (const auto& existing : collections) {
    if (existing.id == collection.id) {
      LOG_ERR("COL", "Collection with ID %s already exists", collection.id.c_str());
      return false;
    }
  }
  
  collections.push_back(collection);
  if (!saveToFile()) {
    LOG_ERR("COL", "Failed to save collections after adding %s", collection.id.c_str());
    collections.pop_back();
    return false;
  }
  
  LOG_INF("COL", "Added collection: %s", collection.id.c_str());
  return true;
}

bool CollectionStore::updateCollection(size_t index, const Collection& collection) {
  if (index >= collections.size()) {
    LOG_ERR("COL", "Invalid collection index: %d", index);
    return false;
  }
  
  if (!collection.isValid()) {
    LOG_ERR("COL", "Cannot update with invalid collection");
    return false;
  }
  
  collections[index] = collection;
  if (!saveToFile()) {
    LOG_ERR("COL", "Failed to save collections after updating index %d", index);
    return false;
  }
  
  LOG_INF("COL", "Updated collection at index %d: %s", index, collection.id.c_str());
  return true;
}

bool CollectionStore::removeCollection(size_t index) {
  if (index >= collections.size()) {
    LOG_ERR("COL", "Invalid collection index: %d", index);
    return false;
  }
  
  std::string removedId = collections[index].id;
  collections.erase(collections.begin() + index);
  
  if (!saveToFile()) {
    LOG_ERR("COL", "Failed to save collections after removing index %d", index);
    return false;
  }
  
  LOG_INF("COL", "Removed collection at index %d: %s", index, removedId.c_str());
  return true;
}

bool CollectionStore::removeCollectionById(const std::string& collectionId) {
  for (size_t i = 0; i < collections.size(); ++i) {
    if (collections[i].id == collectionId) {
      return removeCollection(i);
    }
  }
  LOG_ERR("COL", "Collection with ID %s not found", collectionId.c_str());
  return false;
}

const Collection* CollectionStore::getCollection(size_t index) const {
  ensureLoaded();
  if (index >= collections.size()) {
    return nullptr;
  }
  return &collections[index];
}

const Collection* CollectionStore::getCollectionById(const std::string& collectionId) const {
  ensureLoaded();
  for (const auto& collection : collections) {
    if (collection.id == collectionId) {
      return &collection;
    }
  }
  return nullptr;
}

std::vector<Collection> CollectionStore::getCollectionsByLibrary(const std::string& libraryId) const {
  ensureLoaded();
  std::vector<Collection> result;
  for (const auto& collection : collections) {
    if (collection.libraryId == libraryId) {
      result.push_back(collection);
    }
  }
  return result;
}

std::vector<Collection> CollectionStore::getRootCollectionsByLibrary(const std::string& libraryId) const {
  ensureLoaded();
  std::vector<Collection> result;
  for (const auto& collection : collections) {
    if (collection.libraryId == libraryId && collection.isRootLevel()) {
      result.push_back(collection);
    }
  }
  return result;
}

std::vector<Collection> CollectionStore::getChildCollections(const std::string& parentId) const {
  ensureLoaded();
  std::vector<Collection> result;
  for (const auto& collection : collections) {
    if (collection.parentId == parentId) {
      result.push_back(collection);
    }
  }
  return result;
}

bool CollectionStore::pruneOrphanedCollections(const std::vector<std::string>& validLibraryIds) {
  bool removed = false;
  std::vector<Collection> newCollections;
  
  for (const auto& collection : collections) {
    bool valid = false;
    for (const auto& libraryId : validLibraryIds) {
      if (collection.libraryId == libraryId) {
        valid = true;
        break;
      }
    }
    if (valid) {
      newCollections.push_back(collection);
    } else {
      LOG_INF("COL", "Pruning orphaned collection: %s (library: %s)",
               collection.id.c_str(), collection.libraryId.c_str());
      removed = true;
    }
  }
  
  if (removed) {
    collections = std::move(newCollections);
    if (!saveToFile()) {
      LOG_ERR("COL", "Failed to save collections after pruning");
      return false;
    }
  }
  
  return true;
}

bool CollectionStore::pruneInvalidCollections() {
  bool removed = false;
  std::vector<Collection> newCollections;
  
  // Build a set of valid parent IDs
  std::vector<std::string> validParentIds;
  for (const auto& collection : collections) {
    validParentIds.push_back(collection.id);
  }
  validParentIds.push_back("");  // Root-level collections have empty parentId
  
  for (const auto& collection : collections) {
    bool valid = false;
    for (const auto& parentId : validParentIds) {
      if (collection.parentId == parentId) {
        valid = true;
        break;
      }
    }
    if (valid) {
      newCollections.push_back(collection);
    } else {
      LOG_INF("COL", "Pruning invalid collection: %s (parent: %s)",
               collection.id.c_str(), collection.parentId.c_str());
      removed = true;
    }
  }
  
  if (removed) {
    collections = std::move(newCollections);
    if (!saveToFile()) {
      LOG_ERR("COL", "Failed to save collections after pruning invalid");
      return false;
    }
  }
  
  return true;
}
