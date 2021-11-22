#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
    }

    template<typename Function>
    vector<Document> FindTopDocuments(const string& raw_query, Function function) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, function);
        const auto err = 1e-6; // вынесла погрешность в константу
        sort(matched_documents.begin(), matched_documents.end(),
            [err](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < err) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_) const {
        return FindTopDocuments(raw_query, [status_](int document_id, DocumentStatus status, int rating) { return status == status_; });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
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
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        auto rating_sum = accumulate(ratings.begin(), ratings.end(), 0); // использовала std::accumulate
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Function>
    vector<Document> FindAllDocuments(const Query& query, Function function) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (function(document_id, documents_.at(document_id).status, documents_.at(document_id).rating) == true) { document_to_relevance[document_id] += term_freq * inverse_document_freq; }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


void AssertEqualImpl(const string& file, const string& func, unsigned line, const string& str, const string& hint) {
    cout << file << "("s << line << "): "s << func << ": "s;
    cout << "ASSERT("s << str << ") failed."s;
    if (!hint.empty()) {
        cout << " Hint: "s << hint;
    }
    cout << endl;
    abort();
}

#define ASSERT(expr) if (!(expr)) AssertEqualImpl(__FILE__, __FUNCTION__, __LINE__, #expr, ""s)

#define ASSERT_HINT(expr, hint) if (!(expr)) AssertEqualImpl(__FILE__, __FUNCTION__, __LINE__, #expr, (hint))

template <typename T>
void RunTestImpl(const T& t, const string& t_str) {
    try {
        t();
    }
    catch (runtime_error& e) {
        cerr << t_str << " fail: " << e.what() << endl;
    }
    cerr << t_str << " OK"s << endl;
}


#define RUN_TEST(func) RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

void TestAddDocument() {
    const int doc_id = 67;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);

        const auto found_docs_1 = server.FindTopDocuments(""s);
        ASSERT_EQUAL(found_docs_1.size(), 0);

        server.AddDocument(1, " you don't -love love me", DocumentStatus::ACTUAL, { 4 });
        server.AddDocument(2, "i love you ", DocumentStatus::ACTUAL, { 4 });
        ASSERT_EQUAL(server.GetDocumentCount(), 3);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("my dog is perfect"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
}
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
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
// Тест проверяет, что поисковая система исключает документы, содержащие минус-слова, из результатов поиска
void TestExcludeDocumentsWithMinusWords() {
    const int doc_id = 44;
    const string str = "cat in the house"s;
    const vector<int> ratings = { 5, 7, 3 };

    SearchServer server;
    server.AddDocument(doc_id, str, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("house cat -house"s);
    ASSERT(found_docs.empty());
}
// Тест проверяет, что поисковая система находит все слова из поискового запроса в документе
void TestMatchDocument() {
    {
        SearchServer server;
        server.AddDocument(2, "happy dog lucky cats"s, DocumentStatus::ACTUAL, { 7, 8, -2, 1 });
        vector<string> words;
        DocumentStatus status;
        const set<string> query_words{ "dog"s, "happy"s };
        tie(words, status) = server.MatchDocument("happy cat dog always"s, 2);
        ASSERT_EQUAL(words.size(), 2u);
        set<string> words_set(words.begin(), words.end());
        ASSERT(words_set == query_words);
    }

    //При соответствии хотя бы по одному минус-слову, должен возвращаться пустой список слов
    {
        SearchServer server;
        server.AddDocument(3, "happy out dog cat"s, DocumentStatus::ACTUAL, { 6 });
        vector<string> words;
        DocumentStatus status;
        tie(words, status) = server.MatchDocument("-happy happy dog cat"s, 3);
        ASSERT_EQUAL(words.size(), 0);
    }
}
// Тест проверяет вычисление релевантности документа
void TestRelevanceDocument() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT(found_docs[0].relevance - .866434 < 1e-6);
    ASSERT(found_docs[1].relevance - .173287 < 1e-6);
    ASSERT(found_docs[2].relevance - .173287 < 1e-6);

    found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT(found_docs[0].relevance - .231049 < 1e-6);
}
// Тест проверяет расчет среднего рейтинга документа
void TestAverageRatingDocument() {
    {
        SearchServer server;
        server.AddDocument(6, "happy lucky dog cat"s, DocumentStatus::ACTUAL, { 6 });
        auto found_docs = server.FindTopDocuments("lucky"s);
        ASSERT_EQUAL(found_docs[0].rating, 6u);
    }

    {
        SearchServer server;
        server.AddDocument(8, "happy dogs"s, DocumentStatus::ACTUAL, { 5, -8, 2 });
        auto found_docs = server.FindTopDocuments("dogs"s);
        ASSERT_EQUAL(found_docs[0].rating, 0);
    }
}
void TestPredicatFunction() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
    Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, 3u);

    found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1; });
    for (size_t i = 0; i + 1 < found_docs.size(); ++i) {
        ASSERT_EQUAL(found_docs[i].id % 2, 1u);
    }
}
// Тест проверяет отбор по статусу документа
void TestChoiseOfStatusDocument() {

    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::IRRELEVANT, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::REMOVED, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    server.AddDocument(4, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, { -12, 2, 1 });
    server.AddDocument(5, "ухоженный пёс выразительные глаза"s, DocumentStatus::REMOVED, { 5, 2, 1 });

    {auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL(found_docs.size(), 2u);
    Document& doc0 = found_docs[0];
    Document& doc1 = found_docs[1];
    ASSERT((doc0.id == 0 && doc1.id == 4) || (doc0.id == 4 && doc1.id == 0)); }

    {auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
    Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, 1u); }

    {auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::REMOVED);
    ASSERT(found_docs.size() == 2);
    Document& doc0 = found_docs[0];
    Document& doc1 = found_docs[1];
    ASSERT((doc0.id == 2 && doc1.id == 5) || (doc0.id == 5 && doc1.id == 2)); }

    {auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, 3u); }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestAverageRatingDocument);
    RUN_TEST(TestRelevanceDocument);
    RUN_TEST(TestChoiseOfStatusDocument);
    RUN_TEST(TestPredicatFunction);
    RUN_TEST(TestAddDocument);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}