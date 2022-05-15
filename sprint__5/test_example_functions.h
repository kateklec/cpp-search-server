#pragma once
#include "search_server.h"

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint);

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertEqualImpl(const string& file, const string& func, unsigned line, const string& str, const string& hint);

#define ASSERT(expr) if (!(expr)) AssertEqualImpl(__FILE__, __FUNCTION__, __LINE__, #expr, ""s)

#define ASSERT_HINT(expr, hint) if (!(expr)) AssertEqualImpl(__FILE__, __FUNCTION__, __LINE__, #expr, (hint))

template <typename T>
void RunTestImpl(const T & t, const string & t_str);

#define RUN_TEST(func) RunTestImpl((func), #func)

void TestAddDocument();
void TestExcludeStopWordsFromAddedDocumentContent();
void TestExcludeDocumentsWithMinusWords();
void TestMatchDocument();
void TestRelevanceDocument();
void TestAverageRatingDocument();
void TestPredicatFunction();
void TestChoiseOfStatusDocument();
void TestSearchServer();