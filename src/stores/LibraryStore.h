#pragma once

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <string>
#include <vector>

#include "models/CatalogModels.h"

/**
 * Singleton class for storing Library configurations on the SD card.
 */
class LibraryStore : public PersistableStore<LibraryStore> {
 private:
  std::vector<Library> libraries;
  bool loaded_ = false;

  static constexpr size_t MAX_LIBRARIES = 16;

  LibraryStore() = default;
  bool migrateFromLegacy();  // Migrate from old settings if needed

  friend class PersistableStore<LibraryStore>;

 public:
  static const char* getFilePath() { return "/.crosspoint/libraries.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);
  bool loadFromFile();
  void ensureLoaded() const;
  void release();

  // Add a new library
  bool addLibrary(const Library& library);

  // Update an existing library
  bool updateLibrary(size_t index, const Library& library);

  // Remove a library by index
  bool removeLibrary(size_t index);

  // Remove a library by ID
  bool removeLibraryById(const std::string& libraryId);

  // Get all libraries
  const std::vector<Library>& getLibraries() const {
    ensureLoaded();
    return libraries;
  }

  // Get a library by index
  const Library* getLibrary(size_t index) const;

  // Get a library by ID
  const Library* getLibraryById(const std::string& libraryId) const;

  // Get the default library
  const Library* getDefaultLibrary() const;

  // Set the default library by ID
  bool setDefaultLibrary(const std::string& libraryId);

  // Get the number of libraries
  size_t getCount() const {
    ensureLoaded();
    return libraries.size();
  }

  // Check if any libraries are configured
  bool hasLibraries() const {
    ensureLoaded();
    return !libraries.empty();
  }

  // Get libraries of a specific provider type
  std::vector<Library> getLibrariesByType(Library::ProviderType type) const;
};

// Helper macro to access the library store
#define LIBRARY_STORE LibraryStore::getInstance()
