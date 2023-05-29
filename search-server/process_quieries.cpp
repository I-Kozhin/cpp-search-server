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

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    //size_t total_size = 0u;
    auto matched_docs = ProcessQueries(search_server, queries);
    /*for (auto& query_res : matched_docs) {
        total_size += query_res.size();
    }*/
    std::vector<Document> result;
    //result.reserve(total_size);
    for (auto& query_res : matched_docs) {
        result.insert(result.end(), query_res.begin(), query_res.end());
    }
    
    return result;
}
