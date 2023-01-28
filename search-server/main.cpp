#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <cassert>

#include "search_server.h"

using namespace std;


// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

//1. Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddDocument() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    vector<Document> document = server.FindTopDocuments("белый кот и модный ошейник"s);

    ASSERT_EQUAL(server.FindTopDocuments("белый кот и модный ошейник"s)[0].id , 0);
    ASSERT_EQUAL(server.FindTopDocuments("белый кот и модный ошейник"s)[0].relevance , 0);
    ASSERT(server.FindTopDocuments("белый кот и модный ошейник"s)[0].rating == 2);

}

//2. Поддержка стоп-слов. Стоп-слова исключаются из текста документов
void TestExcludeStopWord() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и в на модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    vector<Document> document = server.FindTopDocuments("и в на"s);

    ASSERT_EQUAL(document.size() , 0);
    ASSERT_EQUAL_HINT(document.size(), 0, " EXPR MUST BE EQUAL "s);
    ASSERT(document.size() == 0);
}

//3. Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestMinusWord() {
    SearchServer server;

    server.AddDocument(0, "белый кот модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "черный пес бульдог"s, DocumentStatus::ACTUAL, { 8, -7 });
    tuple<vector<string>, DocumentStatus> match_data = server.MatchDocument("белый кот модный -ошейник"s, 0);
    tuple<vector<string>, DocumentStatus> match_data_2 = server.MatchDocument("черный пес -бульдог"s, 1);
    int size_vec_1 = get<0>(match_data).size();
    int size_vec_2 = get<0>(match_data_2).size();

    ASSERT_EQUAL(size_vec_1 , 0);
    ASSERT_EQUAL_HINT(size_vec_1, 0, " EXPR MUST BE EQUAL "s);

    ASSERT_EQUAL(size_vec_2, 0);
    ASSERT_EQUAL_HINT(size_vec_2, 0, " EXPR MUST BE EQUAL "s);

    ASSERT(size_vec_1 == 0);
    ASSERT(size_vec_2 == 0);

    ASSERT_HINT(size_vec_1 == 0, " EXPR MUST BE EQUAL ");
    ASSERT_HINT(size_vec_2 == 0, " EXPR MUST BE EQUAL ");
}

// 4. Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
//    присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов. 
void TestMatchDoc() {
    SearchServer server;

    server.AddDocument(0, "белый кот модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "черный пес бульдог"s, DocumentStatus::ACTUAL, { 8, -7 });
    tuple<vector<string>, DocumentStatus> match_data = server.MatchDocument("белый кот модный ошейник"s, 0);

    tuple<vector<string>, DocumentStatus> match_data_2 = server.MatchDocument("черный пес -бульдог"s, 1);
    string result_string = ""s;
    int size_vec_2 = get<0>(match_data_2).size();
    ASSERT_EQUAL(size_vec_2, 0);
    ASSERT_EQUAL_HINT(size_vec_2, 0, " EXPR MUST BE EQUAL "s);
    ASSERT(size_vec_2 == 0);
    ASSERT_HINT(size_vec_2 == 0, " EXPR MUST BE EQUAL ");
}

// 5. Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке 
//    убывания релевантности.
void TestSortDocument() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "белый пес модный"s, DocumentStatus::ACTUAL, { 8, -7 });
    vector<Document> document = server.FindTopDocuments("белый кот и модный ошейник"s);
    ASSERT(server.FindTopDocuments("белый кот и модный ошейник"s)[0].relevance > server.FindTopDocuments("белый кот и модный ошейник"s)[1].relevance);
    ASSERT_HINT(server.FindTopDocuments("белый кот и модный ошейник"s)[0].relevance > server.FindTopDocuments("белый кот и модный ошейник"s)[1].relevance, " First EXPR MUST BE Larger ");
}

//6. Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок
void TestRatingDocument() {
    SearchServer server;
    server.AddDocument(1, "белый пес модный"s, DocumentStatus::ACTUAL, { 8, 3 });
    vector<Document> document = server.FindTopDocuments("белый пес модный"s);

    ASSERT_EQUAL(server.FindTopDocuments("белый кот и модный ошейник"s)[0].rating , 5);
    ASSERT_EQUAL_HINT(server.FindTopDocuments("белый кот и модный ошейник"s)[0].rating , 5, " EXPR MUST BE EQUAL "s);
    ASSERT(server.FindTopDocuments("белый кот и модный ошейник"s)[0].rating == 5);
    ASSERT_HINT(server.FindTopDocuments("белый кот и модный ошейник"s)[0].rating == 5, " EXPR MUST BE EQUAL ");

}

//7. Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestFiltrDocument() {
    SearchServer server;
    server.AddDocument(0, "белый пес модный"s, DocumentStatus::ACTUAL, { 8, 3 });
    server.AddDocument(1, "белый пес старомодный"s, DocumentStatus::ACTUAL, { 8, 14 });
    server.AddDocument(2, "полосатый пес в крапинку со шнурками на ботинках"s, DocumentStatus::ACTUAL, { 8, 14 });
    server.AddDocument(3, "черный пес с пятном белым"s, DocumentStatus::IRRELEVANT, { 8, 4 });
    server.AddDocument(4, "желтый кот с серым ошейником"s, DocumentStatus::BANNED, { 8, 8 });
    vector<Document> document = server.FindTopDocuments("пушистый ухоженный пес"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });

    ASSERT_EQUAL(document.size() , 2);
    ASSERT_EQUAL_HINT(document.size(), 2, " EXPR MUST BE EQUAL "s);
    ASSERT(document.size() == 2);
    ASSERT_HINT(document.size() == 2, " EXPR MUST BE EQUAL ");
}

//8 Поиск документов, имеющих заданный статус.
void TestFiltrSTATYSDocument() {
    SearchServer server;
    server.AddDocument(0, "белый пес модный"s, DocumentStatus::ACTUAL, { 8, 3 });
    server.AddDocument(1, "белый пес старомодный"s, DocumentStatus::ACTUAL, { 8, 14 });
    server.AddDocument(2, "полосатый пес в крапинку со шнурками на ботинках"s, DocumentStatus::ACTUAL, { 8, 14 });
    server.AddDocument(3, "черный пес с пятном белым"s, DocumentStatus::IRRELEVANT, { 8, 4 });
    server.AddDocument(4, "желтый кот с серым ошейником"s, DocumentStatus::BANNED, { 8, 8 });
    vector<Document> document = server.FindTopDocuments("пушистый ухоженный пес"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
    vector<Document> document_2 = server.FindTopDocuments("пушистый ухоженный пес"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });


    ASSERT(document.size() == 1 && document_2.size() == 3);
    ASSERT_HINT(document.size() == 1 && document_2.size() == 3, " EXPR MUST BE EQUAL ");

}


//9 Корректное вычисление релевантности найденных документов.
void TestRelevanceDocument() {
    SearchServer server;
    server.AddDocument(0, "белый пес модный"s, DocumentStatus::ACTUAL, { 8, 3 });
    server.AddDocument(1, "белый пес старомодный"s, DocumentStatus::ACTUAL, { 8, 2 });

    vector<Document> document = server.FindTopDocuments("белый пес модный"s);

    ASSERT((server.FindTopDocuments("белый пес модный"s)[0].relevance - 0.231049) < 0.00001);
    ASSERT_HINT((server.FindTopDocuments("белый пес модный"s)[0].relevance - 0.231049) < 0.00001, " EXPR MUST BE EQUAL ");
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    TestAddDocument();
    TestExcludeStopWord();
    TestMinusWord();
    TestMatchDoc();
    TestSortDocument();
    TestRatingDocument();
    TestFiltrSTATYSDocument();
    TestFiltrDocument();
    TestRelevanceDocument();

}

// --------- Окончание модульных тестов поисковой системы -----------