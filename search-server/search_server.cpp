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

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
    if (!IsValidWords(document)) // явное указание метода в условии                                        // добавлена проверка, что все слова в документе соответсвуют требованиям
        throw invalid_argument("invalid_argument");
    if (document_id < 0 || count(docs_indexes_.begin(), docs_indexes_.end(), document_id) != 0 || IsNoSpecialCharacters(document) == false)
        throw invalid_argument("invalid_argument");
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status });
    docs_indexes_.push_back(document_id);

}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}
    
int SearchServer::GetDocumentId(int index) const { // добавлена функция, выдающая индекса документа, с проверкой на релевантность данных 
    if (index < 0 || index >= GetDocumentCount()) { throw out_of_range("index out_of_range");
    } else { 
        return docs_indexes_.at(index); 
    } 
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
             continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return make_tuple(matched_words, documents_.at(document_id).status);
}
    
bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}
    
vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}
    
int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0); // добавлен accumulate
    return rating_sum / static_cast<int>(ratings.size());
}
    
    
    
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

bool SearchServer::IsValidWords(string text) const { // Основная функция, проверяющая пустоту, спец символы и другие требования к словам
    if (text.empty()) return false; // Также проверяем все остальные аспекты "правильного" слова
    if (IsNoSpecialCharacters(text) == false) return false;
    char ch = text.back();
    if (ch == '-') { return false; }  // строка не оканчивается на "-"
    for (size_t i = 0; i < text.size() - 1; ++i)
    {
        if (text[i] == '-' && (text[i + 1] == '-' || text[i + 1] == ' ')) return false;
    }
    return true;
}
    
SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query query;
    if (!IsValidWords(text))  // добавлена проверка ввода, произведена инкапсудяция проверки
        throw invalid_argument("invalid_argument");
    for (const string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}
    
// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
    
bool SearchServer::IsNoSpecialCharacters(const string& word) { // Проверка слова на спец символы
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}
