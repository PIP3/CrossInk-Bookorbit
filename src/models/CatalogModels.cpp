#include "CatalogModels.h"

// Implement getDefaultBookOrbitSmartScopes
const std::vector<SmartScope>& SmartScope::getDefaultBookOrbitSmartScopes() {
  static const std::vector<SmartScope> scopes = []() {
    std::vector<SmartScope> result;
    SmartScope scope;
    
    scope.id = "recently_added";
    scope.title = "Recently Added";
    scope.query = "sort:recently_added";
    scope.description = "Books recently added to your library";
    scope.isDynamic = true;
    scope.libraryId = "";
    scope.bookCount = 0;
    scope.lastUpdated = 0;
    result.push_back(scope);
    
    scope.id = "continue_reading";
    scope.title = "Continue Reading";
    scope.query = "sort:recently_read";
    scope.description = "Books you are currently reading";
    result.push_back(scope);
    
    scope.id = "all_books";
    scope.title = "All Books";
    scope.query = "sort:title";
    scope.description = "All books in your library";
    result.push_back(scope);
    
    scope.id = "favorites";
    scope.title = "Favorites";
    scope.query = "filter:favorites";
    scope.description = "Your favorite books";
    result.push_back(scope);
    
    return result;
  }();
  return scopes;
}

// Define the static member
const std::vector<SmartScope> SmartScope::DEFAULT_BOOKORBIT_SMART_SCOPES = SmartScope::getDefaultBookOrbitSmartScopes();
