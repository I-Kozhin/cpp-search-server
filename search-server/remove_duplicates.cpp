#include "search_server.h"
#include "remove_duplicates.h"
#include "document.h"
#include "string_processing.h"
#include <algorithm>
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

void RemoveDuplicates(SearchServer& search_server) {
	set<set<string>> id_document_structure; // создаём стуруктуру для слов для поиска по id
	set<int> to_erase;
	for (const int document_id : search_server) {
		set<string> key_words; // только слова документа без id
		map<string, double> id_freq = search_server.GetWordFrequencies(document_id); // частота слов по id
		for (auto&& [first, second] : id_freq) {
			key_words.insert(first);
		}
		// добавляю набор слов в итоговую структуру при условии уникальности набора
		if (id_document_structure.count(key_words) == 0) {
			id_document_structure.insert(key_words);
		}
		else {
			cout << "Found duplicate document id " << document_id << endl;
			to_erase.insert(document_id);
		}
	}

    //документы, состоящие в чёрном списке, удаляются
	for (auto document_id : to_erase) {
		search_server.RemoveDocument(document_id);
	}

}
