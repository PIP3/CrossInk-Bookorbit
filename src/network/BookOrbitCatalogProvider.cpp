#include "BookOrbitCatalogProvider.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "Logging.h"

// Helper to trim whitespace from a string
static std::string trim(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) return "";
  size_t end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

// Helper to convert a BookOrbitCatalogBook to a Book
Book BookOrbitCatalogProvider::convertToBook(const BookOrbitCatalogBook& catalogBook) const {
  Book book;
  book.id = catalogBook.id;
  book.title = catalogBook.title;
  book.author = catalogBook.author;
  book.devicePath = catalogBook.devicePath;
  // TODO: Populate additional fields if available (e.g., from cache or detail fetch)
  return book;
}

// Helper to convert a BookOrbitBookDetail to a Book
Book BookOrbitCatalogProvider::convertToBook(const BookOrbitBookDetail& detail) const {
  Book book;
  book.id = detail.id;
  book.title = detail.title;
  book.author = detail.author;
  // Add all formats from the detail
  for (const auto& file : detail.files) {
    book.formats.push_back(file.format);
    if (file.format == "epub") {
      book.devicePath = file.devicePath;
      book.sizeBytes = file.sizeBytes;
    }
  }
  return book;
}

// Helper to convert a BookOrbitCatalogSection to a Collection
Collection BookOrbitCatalogProvider::convertToCollection(const BookOrbitCatalogSection& section) const {
  Collection collection;
  collection.id = section.id;
  collection.title = section.title;
  collection.libraryId = libraryId_;
  collection.isSmartScope = false;  // Sections are static collections
  return collection;
}



// Helper to build a BookOrbitBookQuery from a SmartScope
BookOrbitBookQuery BookOrbitCatalogProvider::buildQueryFromSmartScope(const SmartScope& smartScope) const {
  BookOrbitBookQuery query;
  // Parse the smart scope query into BookOrbitBookQuery fields
  // Example queries:
  // - "sort:recently_added" -> query.sort = "recently_added"
  // - "filter:favorites" -> query.query = "favorites"
  // - "author:Tolkien" -> query.author = "Tolkien"
  const std::string& q = smartScope.query;
  if (q.starts_with("sort:")) {
    query.sort = q.substr(5);
  } else if (q.starts_with("filter:")) {
    query.query = q.substr(7);
  } else if (q.starts_with("author:")) {
    query.author = q.substr(7);
  } else if (q.starts_with("series:")) {
    query.series = q.substr(7);
  } else if (q.starts_with("section:")) {
    // Handle section-specific queries (e.g., "section:recently_added")
    query.sort = q.substr(8);
  } else {
    // Default: treat as a free-text search
    query.query = q;
  }
  return query;
}

// Connect to the BookOrbit catalog
bool BookOrbitCatalogProvider::connect() {
  // BookOrbitCatalogClient is stateless, so we just check if we can reach the server
  // by attempting to fetch root sections
  std::vector<BookOrbitCatalogSection> sections;
  bool success = BookOrbitCatalogClient::fetchRootSections(sections);
  if (!success) {
    LOG_ERR("BOC", "Failed to connect to BookOrbit catalog");
    return false;
  }
  LOG_INF("BOC", "Connected to BookOrbit catalog");
  return true;
}

// Disconnect from the BookOrbit catalog
void BookOrbitCatalogProvider::disconnect() {
  // BookOrbitCatalogClient is stateless, so nothing to do here
  LOG_INF("BOC", "Disconnected from BookOrbit catalog");
}

// Check if the provider is connected
bool BookOrbitCatalogProvider::isConnected() const {
  // For BookOrbit, we consider it connected if we can fetch root sections
  // This is a simplified check; in practice, we might want to cache the connection state
  std::vector<BookOrbitCatalogSection> sections;
  return BookOrbitCatalogClient::fetchRootSections(sections);
}

// Fetch root collections (sections) from BookOrbit
bool BookOrbitCatalogProvider::fetchRootCollections(std::vector<Collection>& outCollections) {
  std::vector<BookOrbitCatalogSection> sections;
  if (!BookOrbitCatalogClient::fetchRootSections(sections)) {
    LOG_ERR("BOC", "Failed to fetch root sections");
    return false;
  }

  outCollections.clear();
  std::transform(sections.begin(), sections.end(), std::back_inserter(outCollections),
    [this](const BookOrbitCatalogSection& section) { return convertToCollection(section); });

  LOG_INF("BOC", "Fetched %d root collections", outCollections.size());
  return true;
}

// Fetch books from a collection
bool BookOrbitCatalogProvider::fetchCollectionBooks(
    const Collection& collection, size_t offset, size_t limit,
    std::vector<Book>& outBooks, size_t& outTotal) {

  // For BookOrbit, collections are mapped to sections or queries
  // If the collection is a smart scope, use fetchSmartScopeBooks
  if (collection.isSmartScope) {
    // Find the corresponding smart scope
    std::vector<SmartScope> scopes;
    if (!fetchSmartScopes(scopes)) {
      return false;
    }
    auto it = std::find_if(scopes.begin(), scopes.end(),
      [&collection](const SmartScope& scope) { return scope.id == collection.id; });
    if (it != scopes.end()) {
      return fetchSmartScopeBooks(*it, offset, limit, outBooks, outTotal);
    }
    return false;
  }

  // For static collections (sections), fetch books using the section ID as a query
  BookOrbitBookQuery query;
  query.sort = collection.id;  // Use section ID as sort/filter

  int page = static_cast<int>(offset / limit) + 1;
  BookOrbitBookPage pageData;
  if (!BookOrbitCatalogClient::fetchBooks(query, page, pageData)) {
    LOG_ERR("BOC", "Failed to fetch books for collection: %s", collection.id.c_str());
    return false;
  }

  outBooks.clear();
  std::transform(pageData.books.begin(), pageData.books.end(), std::back_inserter(outBooks),
    [this](const BookOrbitCatalogBook& catalogBook) { return convertToBook(catalogBook); });
  outTotal = pageData.total;

  LOG_INF("BOC", "Fetched %d books for collection: %s (total: %d)",
           outBooks.size(), collection.id.c_str(), outTotal);
  return true;
}

// Fetch smart scopes from BookOrbit
bool BookOrbitCatalogProvider::fetchSmartScopes(std::vector<SmartScope>& outSmartScopes) {
  // For now, return the default BookOrbit smart scopes
  // In the future, this could fetch dynamic scopes from the server
  outSmartScopes = SmartScope::getDefaultBookOrbitSmartScopes();
  // Update library IDs
  for (auto& scope : outSmartScopes) {
    scope.libraryId = libraryId_;
  }
  return true;
}

// Fetch books from a smart scope
bool BookOrbitCatalogProvider::fetchSmartScopeBooks(
    const SmartScope& smartScope, size_t offset, size_t limit,
    std::vector<Book>& outBooks, size_t& outTotal) {

  BookOrbitBookQuery query = buildQueryFromSmartScope(smartScope);
  int page = static_cast<int>(offset / limit) + 1;

  BookOrbitBookPage pageData;
  if (!BookOrbitCatalogClient::fetchBooks(query, page, pageData)) {
    LOG_ERR("BOC", "Failed to fetch books for smart scope: %s", smartScope.id.c_str());
    return false;
  }

  outBooks.clear();
  std::transform(pageData.books.begin(), pageData.books.end(), std::back_inserter(outBooks),
    [this](const BookOrbitCatalogBook& catalogBook) { return convertToBook(catalogBook); });
  outTotal = pageData.total;

  LOG_INF("BOC", "Fetched %d books for smart scope: %s (total: %d)",
           outBooks.size(), smartScope.id.c_str(), outTotal);
  return true;
}

// Search for books in the catalog
bool BookOrbitCatalogProvider::search(
    const std::string& query, size_t offset, size_t limit,
    std::vector<Book>& outBooks, size_t& outTotal) {

  BookOrbitBookQuery bookQuery;
  bookQuery.query = trim(query);

  int page = static_cast<int>(offset / limit) + 1;
  BookOrbitBookPage pageData;
  if (!BookOrbitCatalogClient::fetchBooks(bookQuery, page, pageData)) {
    LOG_ERR("BOC", "Failed to search for query: %s", query.c_str());
    return false;
  }

  outBooks.clear();
  std::transform(pageData.books.begin(), pageData.books.end(), std::back_inserter(outBooks),
    [this](const BookOrbitCatalogBook& catalogBook) { return convertToBook(catalogBook); });
  outTotal = pageData.total;

  LOG_INF("BOC", "Searched for '%s': %d results (total: %d)",
           query.c_str(), outBooks.size(), outTotal);
  return true;
}

// Download a book
HttpDownloader::DownloadError BookOrbitCatalogProvider::downloadBook(
    const Book& book, const std::string& destPath,
    HttpDownloader::ProgressCallback progress,
    bool* cancelFlag,
    HttpDownloader::DownloadOptions options) {

  // Find the EPUB file in the book's formats
  int64_t fileId = 0;
  auto it = std::find(book.formats.begin(), book.formats.end(), "epub");
  if (it != book.formats.end()) {
    // In BookOrbit, the file ID is the same as the book ID for EPUB files
    fileId = book.id;
  }

  if (fileId == 0) {
    LOG_ERR("BOC", "No EPUB file found for book: %s", book.title.c_str());
    return HttpDownloader::DownloadError::FILE_ERROR;
  }

   // Use the device path if available
  if (!book.devicePath.empty()) {
    BookOrbitBookDetail detail;
    if (BookOrbitCatalogClient::fetchBookDetail(book.id, detail)) {
      auto fileIt = std::find_if(detail.files.begin(), detail.files.end(),
        [fileId](const BookOrbitCatalogFile& file) { return file.id == fileId; });
      if (fileIt != detail.files.end()) {
        return BookOrbitCatalogClient::downloadFileWithDetail(
            fileId, detail, destPath, progress, cancelFlag, options);
      }
    }
  }

  // Fallback: download using the file ID
  return BookOrbitCatalogClient::downloadFile(fileId, destPath, progress, cancelFlag, options);
}

// Get the name of this provider
std::string BookOrbitCatalogProvider::getName() const {
  return "BookOrbit";
}

// Get the type of this provider
std::string BookOrbitCatalogProvider::getType() const {
  return "BookOrbit";
}
