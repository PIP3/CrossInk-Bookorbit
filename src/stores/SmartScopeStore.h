#pragma once

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <string>
#include <vector>

#include "models/CatalogModels.h"

/**
 * Singleton class for storing SmartScope configurations on the SD card.
 */
class SmartScopeStore : public PersistableStore<SmartScopeStore> {
 private:
  std::vector<SmartScope> smartScopes;
  bool loaded_ = false;

  static constexpr size_t MAX_SMART_SCOPES = 64;

  SmartScopeStore() = default;

  friend class PersistableStore<SmartScopeStore>;

 public:
  static const char* getFilePath() { return "/.crosspoint/smart_scopes.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);
  bool loadFromFile();
  void ensureLoaded() const;
  void release();

  // Add a new smart scope
  bool addSmartScope(const SmartScope& smartScope);

  // Update an existing smart scope
  bool updateSmartScope(size_t index, const SmartScope& smartScope);

  // Remove a smart scope by index
  bool removeSmartScope(size_t index);

  // Remove a smart scope by ID
  bool removeSmartScopeById(const std::string& smartScopeId);

  // Get all smart scopes
  const std::vector<SmartScope>& getSmartScopes() const {
    ensureLoaded();
    return smartScopes;
  }

  // Get smart scopes for a specific library
  std::vector<SmartScope> getSmartScopesByLibrary(const std::string& libraryId) const;

  // Get a smart scope by index
  const SmartScope* getSmartScope(size_t index) const;

  // Get a smart scope by ID
  const SmartScope* getSmartScopeById(const std::string& smartScopeId) const;

  // Get the number of smart scopes
  size_t getCount() const {
    ensureLoaded();
    return smartScopes.size();
  }

  // Check if any smart scopes are configured
  bool hasSmartScopes() const {
    ensureLoaded();
    return !smartScopes.empty();
  }

  // Remove smart scopes that reference a non-existent library
  bool pruneOrphanedSmartScopes(const std::vector<std::string>& validLibraryIds);

  // Validate all smart scope queries
  bool validateQueries();
};

// Helper macro to access the smart scope store
#define SMART_SCOPE_STORE SmartScopeStore::getInstance()
