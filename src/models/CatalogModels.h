#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

// Forward declaration for Book to avoid circular dependencies
struct Book;

/**
 * Represents a smart scope, which is a dynamic collection of books based on a query.
 * Examples: "Recently Added", "Most Read", "By Author: Tolkien".
 */
struct SmartScope {
  std::string id;              // Unique identifier (e.g., "recently_added")
  std::string title;           // Display name (e.g., "Recently Added")
  std::string query;           // Query string for filtering books (e.g., "sort:recently_added")
  std::string description;     // Optional description for UI
  std::string icon;            // Optional icon identifier for UI
  bool isDynamic = true;       // Always true for smart scopes
  std::string libraryId;       // ID of the library this scope belongs to
  size_t bookCount = 0;        // Cached count of books in this scope
  uint64_t lastUpdated = 0;    // Timestamp of last update (milliseconds since epoch)

  // Default smart scopes for BookOrbit
  static const std::vector<SmartScope> DEFAULT_BOOKORBIT_SMART_SCOPES;

  // Helper function to get default smart scopes
  static const std::vector<SmartScope>& getDefaultBookOrbitSmartScopes();

  // Check if this smart scope is valid (non-empty ID and title)
  bool isValid() const { return !id.empty() && !title.empty(); }

  // Equality operator
  bool operator==(const SmartScope& other) const { return id == other.id; }
};

/**
 * Represents a collection, which is a static or dynamic grouping of books.
 * Examples: "Fantasy", "Sci-Fi", "My Favorites".
 */
struct Collection {
  std::string id;              // Unique identifier
  std::string title;           // Display name
  std::string parentId;        // ID of parent collection (empty for root-level)
  std::string libraryId;       // ID of the library this collection belongs to
  bool isSmartScope = false;   // Whether this is a smart scope (dynamic)
  size_t bookCount = 0;        // Cached count of books in this collection
  uint64_t lastUpdated = 0;    // Timestamp of last update (milliseconds since epoch)
  std::string icon;            // Optional icon identifier for UI
  std::string description;     // Optional description for UI

  // Check if this collection is valid (non-empty ID and title)
  bool isValid() const { return !id.empty() && !title.empty(); }

  // Check if this is a root-level collection (no parent)
  bool isRootLevel() const { return parentId.empty(); }

  // Equality operator
  bool operator==(const Collection& other) const { return id == other.id; }
};

/**
 * Represents a library, which is a top-level container for collections and books.
 * Examples: "My BookOrbit Library", "Calibre Server", "Local OPDS Server".
 */
struct Library {
  enum class ProviderType : uint8_t {
    BOOKORBIT = 0,
    OPDS = 1,
    CALIBRE = 2,
    WEBDAV = 3,
    LOCAL = 4,  // Local SD card
  };

  std::string id;              // Unique identifier
  std::string name;            // Display name
  ProviderType providerType;  // Type of catalog provider
  std::string providerConfig;  // JSON or serialized config for the provider (e.g., server URL, credentials)
  bool isDefault = false;      // Whether this is the default library
  bool isEnabled = true;       // Whether this library is enabled
  uint64_t lastSynced = 0;     // Timestamp of last sync (milliseconds since epoch)
  std::string icon;            // Optional icon identifier for UI
  std::string description;     // Optional description for UI

  // Check if this library is valid (non-empty ID and name)
  bool isValid() const { return !id.empty() && !name.empty(); }

  // Get the provider type as a string
  std::string getProviderTypeString() const;

  // Equality operator
  bool operator==(const Library& other) const { return id == other.id; }
};

// Helper function to convert ProviderType to string
inline const char* providerTypeToString(Library::ProviderType type) {
  switch (type) {
    case Library::ProviderType::BOOKORBIT: return "BookOrbit";
    case Library::ProviderType::OPDS: return "OPDS";
    case Library::ProviderType::CALIBRE: return "Calibre";
    case Library::ProviderType::WEBDAV: return "WebDAV";
    case Library::ProviderType::LOCAL: return "Local";
    default: return "Unknown";
  }
}

// Helper function to convert string to ProviderType
inline std::optional<Library::ProviderType> stringToProviderType(const std::string& type) {
  if (type == "BookOrbit") return Library::ProviderType::BOOKORBIT;
  if (type == "OPDS") return Library::ProviderType::OPDS;
  if (type == "Calibre") return Library::ProviderType::CALIBRE;
  if (type == "WebDAV") return Library::ProviderType::WEBDAV;
  if (type == "Local") return Library::ProviderType::LOCAL;
  return std::nullopt;
}

// Implement getProviderTypeString for Library
inline std::string Library::getProviderTypeString() const {
  return providerTypeToString(providerType);
}

/**
 * Represents a book in a catalog, collection, or smart scope.
 * This extends the existing BookOrbitCatalogBook and OpdsEntry models.
 */
struct Book {
  int64_t id = 0;             // Unique identifier (0 if not available)
  std::string title;          // Book title
  std::string author;         // Author name (first author only for compact display)
  std::vector<std::string> authors;  // Full list of authors
  std::string description;    // Book description
  std::string coverUrl;       // URL for the book cover (optional)
  std::string devicePath;     // Server-provided path (e.g., "Series Name/01 - Title.epub")
  std::string localPath;      // Local path on the SD card (if downloaded)
  std::string language;       // Book language (e.g., "en")
  std::vector<std::string> tags;  // Book tags or categories
  std::vector<std::string> formats;  // Available formats (e.g., "epub", "pdf")
  size_t sizeBytes = 0;       // File size in bytes
  uint64_t publishedDate = 0; // Publication date (milliseconds since epoch)
  uint64_t addedDate = 0;     // Date added to the catalog (milliseconds since epoch)
  float rating = 0.0f;        // User rating (0.0 to 5.0)
  bool isRead = false;        // Whether the book has been read
  bool isFavorite = false;    // Whether the book is marked as a favorite

  // Check if this book is valid (non-empty title)
  bool isValid() const { return !title.empty(); }

  // Check if this book is available locally
  bool isLocal() const { return !localPath.empty(); }

  // Equality operator (based on ID and title)
  bool operator==(const Book& other) const {
    return id == other.id && title == other.title;
  }
};
