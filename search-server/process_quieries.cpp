#include "process_queries.h"
#include <vector>
#include <string>
#include <algorithm>
#include <execution>
#include <iostream>
#include <functional>



std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<Document> results;
    std::vector<std::vector<Document>> itg(queries.size());
    transform(std::execution::par, queries.begin(), queries.end(), itg.begin(),     [&search_server](std::string string_for_find) {return search_server.FindTopDocuments(string_for_find); } );

return itg;
}