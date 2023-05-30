#include "search_server.h"
#include "document.h"
#include "string_processing.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <numeric>
#include <string_view>
#include <deque>
#include <execution>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings)
{
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id");
    }
    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    set<string, less<>> document_words;

    for (const auto& word : words) {
        string word_str(word);
        document_words.insert(word_str);

        word_to_document_freqs_[static_cast<string_view>(*document_words.find(word_str))][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][static_cast<string_view>(*document_words.find(word_str))] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, move(document_words) });
    document_ids_.emplace(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const
{
    return FindTopDocuments(execution::seq, raw_query);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(execution::seq, raw_query, status);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    auto it = document_to_word_freqs_.find(document_id);
    if (it != document_to_word_freqs_.end()) {
        return it->second;
    }
    static map<string_view, double> empty;
    return empty;
}

set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

void SearchServer::RemoveDocument(int document_id)
{
    if (!document_ids_.count(document_id)) {
        return;
    }

    auto& words_for_erase = document_to_word_freqs_.at(document_id);
    for (auto& [word, freq] : words_for_erase) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(execution::sequenced_policy seq_police, int document_id)
{
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(execution::parallel_policy par_police, int document_id)
{
    if (!document_ids_.count(document_id)) {
        return;
    }
    auto& words_for_erase = document_to_word_freqs_.at(document_id);

    vector<string_view> words;
    words.reserve(words_for_erase.size());
    transform(par_police, words_for_erase.begin(), words_for_erase.end(), words.begin(), [](const auto& word) {
        return word.first;
    });

    for_each(par_police, words.begin(), words.end(), [&](const auto& word) {
        word_to_document_freqs_.at(word).erase(document_id);
    });

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

SearchServer::MatchResult SearchServer::MatchDocument(string_view raw_query, int document_id) const
{
    return MatchDocument(execution::seq, raw_query, document_id);
}

SearchServer::MatchResult SearchServer::MatchDocument(execution::sequenced_policy police, string_view raw_query, int document_id) const
{
    const auto query = ParseQuery(raw_query, true);

    const auto status_doc = documents_.at(document_id).status;

    for (const string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        auto& map = word_to_document_freqs_.at(word);
        if (map.count(document_id)) {
            return { {}, status_doc };
        }
    }
    vector<string_view> matched_words;
    for (const string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        auto& map = word_to_document_freqs_.at(word);
        if (map.count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

SearchServer::MatchResult SearchServer::MatchDocument(const execution::parallel_policy& police, string_view raw_query, int document_id) const
{
    auto query = ParseQuery(raw_query, false);

    sort(query.minus_words.begin(), query.minus_words.end());
    auto minus_words_end = unique(query.minus_words.begin(), query.minus_words.end());

    const auto& doc_data = documents_.at(document_id);

    vector<string_view> matched_words;
    if (any_of(query.minus_words.begin(), minus_words_end, [&](const auto& word) {
        return doc_data.words.count(word);
    })) {
        return { matched_words, documents_.at(document_id).status };
    }

    sort(query.plus_words.begin(), query.plus_words.end());
    auto plus_words_end = unique(query.plus_words.begin(), query.plus_words.end());

    matched_words.reserve(distance(query.plus_words.begin(), plus_words_end));
    for_each(query.plus_words.begin(), plus_words_end, [&](const auto& word) {
        if (doc_data.words.count(word)) {
            matched_words.push_back(word);
        }
    });

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(string_view word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word)
{
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const
{
    vector<string_view> words;
    for (const string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(move(word));
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings)
{
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view& word) const
{
    if (word.empty()) {
        throw invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(word) + " is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(string_view text, bool needUnique) const
{
    auto words = SplitIntoWords(text);
    Query result;
    result.minus_words.reserve(words.size());
    result.plus_words.reserve(words.size());

    for (string_view word : words) {
        auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(move(query_word.data));
            }
            else {
                result.plus_words.push_back(move(query_word.data));
            }
        }
    }
    if (needUnique) {
        sort(result.plus_words.begin(), result.plus_words.end());
        auto plus_words_end = unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(plus_words_end, result.plus_words.end());

        sort(result.minus_words.begin(), result.minus_words.end());
        auto minus_words_end = unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(minus_words_end, result.minus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer& search_server, int document_id, string_view document, DocumentStatus status, const vector<int>& ratings)
{
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception& e) {
        cout << "Error in adding document "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, string_view raw_query)
{
    cout << "Results for request: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const exception& e) {
        cout << "Error in searching: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, string_view query)
{
    try {
        cout << "Matching for request: "s << query << endl;
        for (auto document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const exception& e) {
        cout << "Error in matching request "s << query << ": "s << e.what() << endl;
    }
}
