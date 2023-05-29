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

/*
Следующие методы теперь должны позволять принять string_view вместо строки:
конструктор;
AddDocument;
FindTopDocuments;
MatchDocument.
Эти методы должны возвращать string_view вместо строк:
MatchDocument;
GetWordFrequencies.
*/

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(
		SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
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
		word_freqs_[document_id][word] += inv_word_count; // добавил заполнене частов для id документа в разбивке           //по словам

	}
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	document_ids_.insert(document_id);

}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
	return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return static_cast<int>(documents_.size());
}

int SearchServer::GetDocumentId(int index) const {
	//return document_ids_.at(index);
	std::set<int>::iterator it = document_ids_.begin();
	std::advance(it, index);
	return *it;
}

std::set<int>::const_iterator SearchServer::begin() const
{
	return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
	return document_ids_.end();
}

std::set<int>::iterator SearchServer::begin()
{
	return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end()
{
	return document_ids_.end();
}


std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {

	const auto query = ParseQuery(raw_query);
	std::vector<std::string> matched_words;
	for (const std::string& word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}
	for (const std::string& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}
	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string& raw_query, int document_id) const {
	return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, const std::string& raw_query, int document_id) const {
        const auto query = ParseQuery(true, raw_query);
        std::vector<std::string> matched_words(query.plus_words.size());
 
        if(any_of(policy, query.minus_words.begin(), query.minus_words.end(), [&](const auto& word){
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                return true;
            }
            return false;
        }) == true) return {matched_words, documents_.at(document_id).status};
 
        auto end = copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](const auto& word){
            if (word_to_document_freqs_.count(word) == 0) {
        return false;
      }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                return true;
            }
            return false;
    });
 
        sort(policy, matched_words.begin(), end);
        auto it = unique(policy, matched_words.begin(), end);
        matched_words.erase(it, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string& word) const {
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
	// A valid word must not contain special characters
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
		});
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("Word " + word + " is invalid");
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
	return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
	if (text.empty()) {
		throw std::invalid_argument("Query word is empty");
	}
	std::string word = text;
	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw std::invalid_argument("Query word " + text + " is invalid");
	}

	return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
	Query result;
	for (const std::string& word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				//result.minus_words.insert(query_word.data);
				result.minus_words.push_back(query_word.data);
			}
			else {
				//result.plus_words.insert(query_word.data);
				result.plus_words.push_back(query_word.data);
			}
		}
	}
	// Избавляюь от дубликатов в плюс и минус словах в последовательной версии 
	std::sort(result.minus_words.begin(), result.minus_words.end());
	auto last = std::unique(result.minus_words.begin(), result.minus_words.end());
	result.minus_words.erase(last, result.minus_words.end());
	std::sort(result.plus_words.begin(), result.plus_words.end());
	auto last_p = std::unique(result.plus_words.begin(), result.plus_words.end());
	result.plus_words.erase(last_p, result.plus_words.end());
	return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy&, const std::string& text) const { return SearchServer::ParseQuery(text); }

// параллельная версия 
SearchServer::Query SearchServer::ParseQuery(bool flag, const std::string& text) const {
	Query result;
	for (const std::string& word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				result.minus_words.push_back(query_word.data);
			}
			else {
				result.plus_words.push_back(query_word.data);
			}
		}
	}
	return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
	static std::map<std::string, double> word_freqs;
	if (word_freqs_.count(document_id) == 0) {
		return word_freqs;
	}
	return word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
	document_ids_.erase(document_id);
	documents_.erase(document_id);
	word_freqs_.erase(document_id);
	for (auto& [key, value] : word_to_document_freqs_) {
		value.erase(document_id);
	}
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
	document_ids_.erase(document_id);
	documents_.erase(document_id);
	std::vector<std::string*> qwe(word_freqs_.at(document_id).size(), nullptr);
	transform(std::execution::par, word_freqs_.at(document_id).begin(), word_freqs_.at(document_id).end(), qwe.begin(), [](auto& t) {
		return new std::string(t.first);
		});

	auto p = [this, document_id](auto t) {

		word_to_document_freqs_.at(*t).erase(document_id);
	};
	word_freqs_.erase(document_id);

	for_each(std::execution::par, qwe.begin(), qwe.end(), p);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
	document_ids_.erase(document_id);
	documents_.erase(document_id);
	std::vector<std::string> qwe(word_freqs_.at(document_id).size());
	transform(std::execution::seq, word_freqs_.at(document_id).begin(), word_freqs_.at(document_id).end(), qwe.begin(), [](auto& t) {
		return t.first;
		});

	auto p = [this, document_id](auto t) {

		word_to_document_freqs_.at(t).erase(document_id);
	};
	word_freqs_.erase(document_id);
	//for_each(std::execution::par, qwe.begin(),qwe.end(),p);
	for_each(std::execution::seq, qwe.begin(), qwe.end(), p);
}