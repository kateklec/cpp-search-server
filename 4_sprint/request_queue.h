#pragma once
#include"search_server.h"

#include<deque>
#include <string>
#include <vector>


class RequestQueue {
public:

    const SearchServer& search_server_;

    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;

private:
    struct QueryResult {/*она не особо нужна*/};
    std::deque<int> requests_;
    const static int sec_in_day_ = 1440;
    int seconds = 0;
};

//РЕАЛИЗАЦИЯ ШАБЛОНА

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> Doc_ = search_server_.FindTopDocuments(raw_query, document_predicate);

    if (Doc_.size() == 0)
        requests_.push_back(0);
    else requests_.push_back(1);
    seconds++;
    if (seconds > sec_in_day_) {
        requests_.pop_front();
        seconds--;
    }
    return Doc_;
}
