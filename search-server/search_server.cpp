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

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw std::invalid_argument("Invalid document_id");
	}
	const auto words = SplitIntoWordsNoStop(document);

	const double inv_word_count = 1.0 / words.size();
	for (const std::string& word : words) {
		word_to_document_freqs_[word][document_id] += inv_word_count;
        word_freqs_[document_id][word] += inv_word_count; // заполнение частоты по id

	}
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	document_ids_.insert(document_id);

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

//1.Откажитесь от метода GetDocumentId(int index) и вместо него определите методы begin и end.Они вернут итераторы.Итератор даст доступ к id всех документов,
//хранящихся в поисковом сервере.Вы можете не разрабатывать собственный итератор, а применить готовый константный итератор удобного контейнера.

set<int>::const_iterator SearchServer::begin() const {
	return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
	return document_ids_.end();
}

set<int>::iterator SearchServer::begin() {
	return document_ids_.begin();
}

set<int>::iterator SearchServer::end() {
	return document_ids_.end();
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
		if (!IsValidWords(word)) {
			throw invalid_argument("Word " + word + " is invalid");
		}
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

//2.Разработайте метод получения частот слов по id документа: 
//Если документа не существует, возвратите ссылку на пустой map
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
	static std::map<std::string, double> word_freqs;
	if (word_freqs_.count(document_id) == 0) {
		return word_freqs;
	}
	return word_freqs_.at(document_id);
}

// Версия без политики
/*void SearchServer::RemoveDocument(int document_id) {
  word_freqs_.erase(document_id);
  for (auto &word: SearchServer:: word_to_document_freqs_) {
        bool is_document_present = word.second.count(document_id);
        if (is_document_present) {
            word.second.erase(document_id);
        }
    }  
  
  documents_.erase(document_id);
  document_ids_.erase(document_id);
}*/

void SearchServer::RemoveDocument(int document_id){
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    word_freqs_.erase(document_id);
    for(auto &[key, value]:word_to_document_freqs_){
        value.erase(document_id);
    }
 }
 
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id){
     document_ids_.erase(document_id);
     documents_.erase(document_id);
     std::vector<std::string*> qwe(word_freqs_.at(document_id).size(),nullptr);
            transform(std::execution::par,word_freqs_.at(document_id).begin(),word_freqs_.at(document_id).end(),qwe.begin(),[](auto& t){
        return new std::string(t.first);
    });
      
      auto p=[this,document_id](auto t){
          
          word_to_document_freqs_.at(*t).erase(document_id);
      };
         word_freqs_.erase(document_id);
    
    for_each(std::execution::par, qwe.begin(),qwe.end(),p);
}
 
void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id){
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    std::vector<std::string> qwe(word_freqs_.at(document_id).size());
            transform(std::execution::seq,word_freqs_.at(document_id).begin(),word_freqs_.at(document_id).end(),qwe.begin(),[](auto& t){
        return t.first;
    });
      
      auto p=[this,document_id](auto t){
          
          word_to_document_freqs_.at(t).erase(document_id);
      };
         word_freqs_.erase(document_id);
    //for_each(std::execution::par, qwe.begin(),qwe.end(),p);
    for_each(std::execution::seq, qwe.begin(),qwe.end(),p);
}