#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries)
{
    vector<vector<Document>> result(queries.size());
    transform(
        execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [&search_server](const string& query)
        { return search_server.FindTopDocuments(query); });
    return result;
}

list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const vector<string>& queries)
{
    vector<vector<Document>> midResult = ProcessQueries(search_server, queries);
    int count_midResult = 0;
    for (auto i : midResult)
    {
        count_midResult += i.size();
    }
    list<Document> result(count_midResult);
    auto iter = result.begin();
    for (auto i : midResult)
    {
        move(i.begin(), i.end(), iter);
        iter = next(iter, i.size());
    }
    return result;
}