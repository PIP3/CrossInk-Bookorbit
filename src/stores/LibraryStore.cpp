#include "LibraryStore.h"

#include <HalStorage.h>
#include <Logging.h>

#include "models/CatalogModels.h"

void LibraryStore::toJson(JsonDocument& doc) const {
  JsonArray arr = doc["libraries"].to<JsonArray>();
  for (const auto& library : libraries) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = library.id;
    obj["name"] = library.name;
    obj["providerType"] = library.getProviderTypeString();
    obj["providerConfig"] = library.providerConfig;
    obj["isDefault"] = library.isDefault;
    obj["isEnabled"] = library.isEnabled;
    obj["lastSynced"] = library.lastSynced;
    if (!library.icon.empty()) {
      obj["icon"] = library.icon;
    }
    if (!library.description.empty()) {
      obj["description"] = library.description;
    }
  }
}

bool LibraryStore::fromJson(JsonVariantConst doc) {
  if (!doc.is<JsonArray>()) {
    LOG_ERR("LIB", "Expected JSON array for libraries");
    return false;
  }

  libraries.clear();
  JsonArrayConst arr = doc.as<JsonArrayConst>();
  libraries.reserve(std::min(arr.size(), MAX_LIBRARIES));
  
  for (JsonObjectConst obj : arr) {
    Library library;
    library.id = obj["id"] | "";
    library.name = obj["name"] | "";
    
    // Parse provider type
    std::string providerTypeStr = obj["providerType"] | "";
    auto providerTypeOpt = stringToProviderType(providerTypeStr);
    if (!providerTypeOpt) {
      LOG_ERR("LIB", "Unknown provider type: %s", providerTypeStr.c_str());
      continue;  // Skip invalid entries
    }
    library.providerType = *providerTypeOpt;
    
    library.providerConfig = obj["providerConfig"] | "";
    library.isDefault = obj["isDefault"] | false;
    library.isEnabled = obj["isEnabled"] | true;
    library.lastSynced = obj["lastSynced"] | 0;
    library.icon = obj["icon"] | "";
    library.description = obj["description"] | "";
    
    if (library.isValid()) {
      libraries.push_back(std::move(library));
    }
  }

  return true;
}

bool LibraryStore::loadFromFile() {
  libraries.clear();
  loaded_ = true;
  const bool hasStoreFile = Storage.exists(getFilePath());
  if (PersistableStore<LibraryStore>::loadFromFile()) {
    return true;
  }
  if (hasStoreFile) {
    return false;
  }
  return false;
}

void LibraryStore::ensureLoaded() const {
  if (loadAttempted_) return;
  const_cast<LibraryStore*>(this)->loadFromFile();
}

void LibraryStore::release() {
  libraries.clear();
  loadAttempted_ = false;
}

bool LibraryStore::addLibrary(const Library& library) {
  if (!library.isValid()) {
    LOG_ERR("LIB", "Cannot add invalid library");
    return false;
  }
  
  if (libraries.size() >= MAX_LIBRARIES) {
    LOG_ERR("LIB", "Maximum number of libraries (%d) reached", MAX_LIBRARIES);
    return false;
  }
  
  // Check for duplicate ID
  for (const auto& existing : libraries) {
    if (existing.id == library.id) {
      LOG_ERR("LIB", "Library with ID %s already exists", library.id.c_str());
      return false;
    }
  }
  
  libraries.push_back(library);
  if (!saveToFile()) {
    LOG_ERR("LIB", "Failed to save libraries after adding %s", library.id.c_str());
    libraries.pop_back();
    return false;
  }
  
  LOG_INF("LIB", "Added library: %s", library.id.c_str());
  return true;
}

bool LibraryStore::updateLibrary(size_t index, const Library& library) {
  if (index >= libraries.size()) {
    LOG_ERR("LIB", "Invalid library index: %d", index);
    return false;
  }
  
  if (!library.isValid()) {
    LOG_ERR("LIB", "Cannot update with invalid library");
    return false;
  }
  
  libraries[index] = library;
  if (!saveToFile()) {
    LOG_ERR("LIB", "Failed to save libraries after updating index %d", index);
    return false;
  }
  
  LOG_INF("LIB", "Updated library at index %d: %s", index, library.id.c_str());
  return true;
}

bool LibraryStore::removeLibrary(size_t index) {
  if (index >= libraries.size()) {
    LOG_ERR("LIB", "Invalid library index: %d", index);
    return false;
  }
  
  std::string removedId = libraries[index].id;
  libraries.erase(libraries.begin() + index);
  
  if (!saveToFile()) {
    LOG_ERR("LIB", "Failed to save libraries after removing index %d", index);
    return false;
  }
  
  LOG_INF("LIB", "Removed library at index %d: %s", index, removedId.c_str());
  return true;
}

bool LibraryStore::removeLibraryById(const std::string& libraryId) {
  for (size_t i = 0; i < libraries.size(); ++i) {
    if (libraries[i].id == libraryId) {
      return removeLibrary(i);
    }
  }
  LOG_ERR("LIB", "Library with ID %s not found", libraryId.c_str());
  return false;
}

const Library* LibraryStore::getLibrary(size_t index) const {
  ensureLoaded();
  if (index >= libraries.size()) {
    return nullptr;
  }
  return &libraries[index];
}

const Library* LibraryStore::getLibraryById(const std::string& libraryId) const {
  ensureLoaded();
  for (const auto& library : libraries) {
    if (library.id == libraryId) {
      return &library;
    }
  }
  return nullptr;
}

const Library* LibraryStore::getDefaultLibrary() const {
  ensureLoaded();
  for (const auto& library : libraries) {
    if (library.isDefault) {
      return &library;
    }
  }
  // Return the first library if no default is set
  if (!libraries.empty()) {
    return &libraries[0];
  }
  return nullptr;
}

bool LibraryStore::setDefaultLibrary(const std::string& libraryId) {
  ensureLoaded();
  bool found = false;
  for (auto& library : libraries) {
    library.isDefault = (library.id == libraryId);
    if (library.id == libraryId) {
      found = true;
    }
  }
  
  if (!found) {
    LOG_ERR("LIB", "Library with ID %s not found", libraryId.c_str());
    return false;
  }
  
  if (!saveToFile()) {
    LOG_ERR("LIB", "Failed to save libraries after setting default to %s", libraryId.c_str());
    return false;
  }
  
  LOG_INF("LIB", "Set default library: %s", libraryId.c_str());
  return true;
}

std::vector<Library> LibraryStore::getLibrariesByType(Library::ProviderType type) const {
  ensureLoaded();
  std::vector<Library> result;
  for (const auto& library : libraries) {
    if (library.providerType == type) {
      result.push_back(library);
    }
  }
  return result;
}

bool LibraryStore::migrateFromLegacy() {
  // TODO: Implement migration from legacy settings (e.g., OpdsServerStore)
  // For now, return true to indicate no migration is needed
  return true;
}
