#include "document.h"

using namespace std;

Document::Document(int id, double relevance, int rating)
    : id(id),
     relevance(relevance), 
     rating(rating) {}

//Перегрузка оператора вывода для структуры Document
std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }";
    return out;
}

void PrintDocument(const Document& document) {
    cout << "{ "
         << "document_id = " << document.id << ", "
         << "relevance = " << document.relevance << ", "
         << "rating = " << document.rating << " }" << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string_view>& words, DocumentStatus status) {
    cout << "{ "
         << "document_id = " << document_id << ", "
         << "status = " << static_cast<int>(status) << ", "
         << "words =";
    for (const string_view& word : words) {
        cout << ' ' << word;
    }
}
