#include <gtest/gtest.h>

#include "models/CatalogModels.h"

// Test Library struct
TEST(CatalogModelsTest, LibraryStruct) {
  Library library;
  library.id = "lib1";
  library.name = "Test Library";
  library.providerType = Library::ProviderType::BOOKORBIT;
  library.providerConfig = "{\"url\": \"https://example.com\"}";
  library.isDefault = true;
  library.isEnabled = true;
  
  EXPECT_TRUE(library.isValid());
  EXPECT_EQ(library.getProviderTypeString(), "BookOrbit");
  
  Library library2;
  library2.id = "lib1";
  library2.name = "Test Library";
  EXPECT_TRUE(library == library2);
}

// Test Library::ProviderType
TEST(CatalogModelsTest, LibraryProviderType) {
  EXPECT_EQ(providerTypeToString(Library::ProviderType::BOOKORBIT), "BookOrbit");
  EXPECT_EQ(providerTypeToString(Library::ProviderType::OPDS), "OPDS");
  EXPECT_EQ(providerTypeToString(Library::ProviderType::CALIBRE), "Calibre");
  EXPECT_EQ(providerTypeToString(Library::ProviderType::WEBDAV), "WebDAV");
  EXPECT_EQ(providerTypeToString(Library::ProviderType::LOCAL), "Local");
  
  auto opt1 = stringToProviderType("BookOrbit");
  ASSERT_TRUE(opt1.has_value());
  EXPECT_EQ(*opt1, Library::ProviderType::BOOKORBIT);
  
  auto opt2 = stringToProviderType("Invalid");
  EXPECT_FALSE(opt2.has_value());
}

// Test Collection struct
TEST(CatalogModelsTest, CollectionStruct) {
  Collection collection;
  collection.id = "col1";
  collection.title = "Test Collection";
  collection.parentId = "";
  collection.libraryId = "lib1";
  collection.isSmartScope = false;
  collection.bookCount = 10;
  
  EXPECT_TRUE(collection.isValid());
  EXPECT_TRUE(collection.isRootLevel());
  
  Collection collection2;
  collection2.id = "col1";
  collection2.title = "Test Collection";
  EXPECT_TRUE(collection == collection2);
}

// Test SmartScope struct
TEST(CatalogModelsTest, SmartScopeStruct) {
  SmartScope scope;
  scope.id = "scope1";
  scope.title = "Test Smart Scope";
  scope.query = "sort:recently_added";
  scope.libraryId = "lib1";
  scope.isDynamic = true;
  
  EXPECT_TRUE(scope.isValid());
  
  SmartScope scope2;
  scope2.id = "scope1";
  scope2.title = "Test Smart Scope";
  EXPECT_TRUE(scope == scope2);
}

// Test Book struct
TEST(CatalogModelsTest, BookStruct) {
  Book book;
  book.id = 123;
  book.title = "Test Book";
  book.author = "Test Author";
  book.localPath = "/books/test.epub";
  
  EXPECT_TRUE(book.isValid());
  EXPECT_TRUE(book.isLocal());
  
  Book book2;
  book2.id = 123;
  book2.title = "Test Book";
  EXPECT_TRUE(book == book2);
}

// Test default smart scopes
TEST(CatalogModelsTest, DefaultSmartScopes) {
  const auto& scopes = SmartScope::getDefaultBookOrbitSmartScopes();
  EXPECT_FALSE(scopes.empty());
  
  // Check that all default scopes are valid
  for (const auto& scope : scopes) {
    EXPECT_TRUE(scope.isValid());
    EXPECT_TRUE(scope.isDynamic);
  }
  
  // Check for expected default scopes
  bool foundRecentlyAdded = false;
  bool foundContinueReading = false;
  bool foundAllBooks = false;
  bool foundFavorites = false;
  
  for (const auto& scope : scopes) {
    if (scope.id == "recently_added") foundRecentlyAdded = true;
    if (scope.id == "continue_reading") foundContinueReading = true;
    if (scope.id == "all_books") foundAllBooks = true;
    if (scope.id == "favorites") foundFavorites = true;
  }
  
  EXPECT_TRUE(foundRecentlyAdded);
  EXPECT_TRUE(foundContinueReading);
  EXPECT_TRUE(foundAllBooks);
  EXPECT_TRUE(foundFavorites);
}

// Test DEFAULT_BOOKORBIT_SMART_SCOPES
TEST(CatalogModelsTest, DefaultBookOrbitSmartScopes) {
  EXPECT_FALSE(SmartScope::DEFAULT_BOOKORBIT_SMART_SCOPES.empty());
  
  // Should be the same as getDefaultBookOrbitSmartScopes()
  const auto& scopes = SmartScope::getDefaultBookOrbitSmartScopes();
  EXPECT_EQ(SmartScope::DEFAULT_BOOKORBIT_SMART_SCOPES.size(), scopes.size());
}
