#include "SmartScopeStore.h"

#include <HalStorage.h>
#include <Logging.h>

#include "models/CatalogModels.h"

void SmartScopeStore::toJson(JsonDocument& doc) const {
  JsonArray arr = doc["smartScopes"].to<JsonArray>();
  for (const auto& smartScope : smartScopes) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = smartScope.id;
    obj["title"] = smartScope.title;
    obj["query"] = smartScope.query;
    obj["libraryId"] = smartScope.libraryId;
    obj["isDynamic"] = smartScope.isDynamic;
    obj["bookCount"] = smartScope.bookCount;
    obj["lastUpdated"] = smartScope.lastUpdated;
    if (!smartScope.icon.empty()) {
      obj["icon"] = smartScope.icon;
    }
    if (!smartScope.description.empty()) {
      obj["description"] = smartScope.description;
    }
  }
}

bool SmartScopeStore::fromJson(JsonVariantConst doc) {
  if (!doc.is<JsonArray>()) {
    LOG_ERR("SS", "Expected JSON array for smart scopes");
    return false;
  }

  smartScopes.clear();
  JsonArrayConst arr = doc.as<JsonArrayConst>();
  smartScopes.reserve(std::min(arr.size(), MAX_SMART_SCOPES));
  
  for (JsonObjectConst obj : arr) {
    SmartScope smartScope;
    smartScope.id = obj["id"] | "";
    smartScope.title = obj["title"] | "";
    smartScope.query = obj["query"] | "";
    smartScope.libraryId = obj["libraryId"] | "";
    smartScope.isDynamic = obj["isDynamic"] | true;
    smartScope.bookCount = obj["bookCount"] | 0;
    smartScope.lastUpdated = obj["lastUpdated"] | 0;
    smartScope.icon = obj["icon"] | "";
    smartScope.description = obj["description"] | "";
    
    if (smartScope.isValid()) {
      smartScopes.push_back(std::move(smartScope));
    }
  }

  return true;
}

bool SmartScopeStore::loadFromFile() {
  smartScopes.clear();
  loadAttempted_ = true;
  const bool hasStoreFile = Storage.exists(getFilePath());
  if (PersistableStore<SmartScopeStore>::loadFromFile()) {
    return true;
  }
  if (hasStoreFile) {
    return false;
  }
  return false;
}

void SmartScopeStore::ensureLoaded() const {
  if (loadAttempted_) return;
  const_cast<SmartScopeStore*>(this)->loadFromFile();
}

void SmartScopeStore::release() {
  smartScopes.clear();
  loadAttempted_ = false;
}

bool SmartScopeStore::addSmartScope(const SmartScope& smartScope) {
  if (!smartScope.isValid()) {
    LOG_ERR("SS", "Cannot add invalid smart scope");
    return false;
  }
  
  if (smartScopes.size() >= MAX_SMART_SCOPES) {
    LOG_ERR("SS", "Maximum number of smart scopes (%d) reached", MAX_SMART_SCOPES);
    return false;
  }
  
  // Check for duplicate ID
  for (const auto& existing : smartScopes) {
    if (existing.id == smartScope.id) {
      LOG_ERR("SS", "Smart scope with ID %s already exists", smartScope.id.c_str());
      return false;
    }
  }
  
  smartScopes.push_back(smartScope);
  if (!saveToFile()) {
    LOG_ERR("SS", "Failed to save smart scopes after adding %s", smartScope.id.c_str());
    smartScopes.pop_back();
    return false;
  }
  
  LOG_INF("SS", "Added smart scope: %s", smartScope.id.c_str());
  return true;
}

bool SmartScopeStore::updateSmartScope(size_t index, const SmartScope& smartScope) {
  if (index >= smartScopes.size()) {
    LOG_ERR("SS", "Invalid smart scope index: %d", index);
    return false;
  }
  
  if (!smartScope.isValid()) {
    LOG_ERR("SS", "Cannot update with invalid smart scope");
    return false;
  }
  
  smartScopes[index] = smartScope;
  if (!saveToFile()) {
    LOG_ERR("SS", "Failed to save smart scopes after updating index %d", index);
    return false;
  }
  
  LOG_INF("SS", "Updated smart scope at index %d: %s", index, smartScope.id.c_str());
  return true;
}

bool SmartScopeStore::removeSmartScope(size_t index) {
  if (index >= smartScopes.size()) {
    LOG_ERR("SS", "Invalid smart scope index: %d", index);
    return false;
  }
  
  std::string removedId = smartScopes[index].id;
  smartScopes.erase(smartScopes.begin() + index);
  
  if (!saveToFile()) {
    LOG_ERR("SS", "Failed to save smart scopes after removing index %d", index);
    return false;
  }
  
  LOG_INF("SS", "Removed smart scope at index %d: %s", index, removedId.c_str());
  return true;
}

bool SmartScopeStore::removeSmartScopeById(const std::string& smartScopeId) {
  for (size_t i = 0; i < smartScopes.size(); ++i) {
    if (smartScopes[i].id == smartScopeId) {
      return removeSmartScope(i);
    }
  }
  LOG_ERR("SS", "Smart scope with ID %s not found", smartScopeId.c_str());
  return false;
}

const SmartScope* SmartScopeStore::getSmartScope(size_t index) const {
  ensureLoaded();
  if (index >= smartScopes.size()) {
    return nullptr;
  }
  return &smartScopes[index];
}

const SmartScope* SmartScopeStore::getSmartScopeById(const std::string& smartScopeId) const {
  ensureLoaded();
  for (const auto& smartScope : smartScopes) {
    if (smartScope.id == smartScopeId) {
      return &smartScope;
    }
  }
  return nullptr;
}

std::vector<SmartScope> SmartScopeStore::getSmartScopesByLibrary(const std::string& libraryId) const {
  ensureLoaded();
  std::vector<SmartScope> result;
  for (const auto& smartScope : smartScopes) {
    if (smartScope.libraryId == libraryId) {
      result.push_back(smartScope);
    }
  }
  return result;
}

bool SmartScopeStore::pruneOrphanedSmartScopes(const std::vector<std::string>& validLibraryIds) {
  bool removed = false;
  std::vector<SmartScope> newSmartScopes;
  
  for (const auto& smartScope : smartScopes) {
    bool valid = false;
    for (const auto& libraryId : validLibraryIds) {
      if (smartScope.libraryId == libraryId) {
        valid = true;
        break;
      }
    }
    if (valid) {
      newSmartScopes.push_back(smartScope);
    } else {
      LOG_INF("SS", "Pruning orphaned smart scope: %s (library: %s)",
               smartScope.id.c_str(), smartScope.libraryId.c_str());
      removed = true;
    }
  }
  
  if (removed) {
    smartScopes = std::move(newSmartScopes);
    if (!saveToFile()) {
      LOG_ERR("SS", "Failed to save smart scopes after pruning");
      return false;
    }
  }
  
  return true;
}

bool SmartScopeStore::validateQueries() {
  bool allValid = true;
  for (const auto& smartScope : smartScopes) {
    // Basic validation: query should not be empty for dynamic scopes
    if (smartScope.isDynamic && smartScope.query.empty()) {
      LOG_ERR("SS", "Smart scope %s has empty query", smartScope.id.c_str());
      allValid = false;
    }
  }
  return allValid;
}
