#include "search_server.h"

//using namespace std;

SearchServer::SearchServer()
{
}

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

std::string_view SearchServer::AddUniqueWord(const std::string& word)
{
	return std::string_view(*(unique_words.insert(word).first));
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
	if ((document_id < 0) || (documents_.count(document_id) > 0))
	{
		throw std::invalid_argument("Invalid document_id"s);
	}

	const auto words = SplitIntoWordsNoStop(document);

	std::unordered_set<std::string_view> document_words;

	const double inv_word_count = 1.0 / words.size();

	for (const auto& word : words)
	{
		const std::string_view current_word = AddUniqueWord(std::string(word));

		document_to_word_freqs_[document_id][word] += inv_word_count;
		word_to_document_freqs_[current_word][document_id] += inv_word_count;

		document_words.insert(current_word);
	}

	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, document_words });
	document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const
{
	return SearchServer::FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
		{ return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query, DocumentStatus status) const
{
	return SearchServer::FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
		{ return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query, DocumentStatus status) const
{
	return SearchServer::FindTopDocuments(std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
		{ return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
	return SearchServer::FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query) const
{
	return SearchServer::FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query) const
{
	return SearchServer::FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
	return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const
{
	return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
	return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static const std::map<std::string_view, double> empty;
	if (documents_.count(document_id) > 0)
	{
		return document_to_word_freqs_.at(document_id);
	}
	return empty;
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
	{
		if (documents_.count(document_id) == 0)
		{
			return;
		}

		std::map<std::string_view, double> word_freq = document_to_word_freqs_[document_id];
		std::vector<std::string_view> vs(word_freq.size());

		std::transform(
			std::execution::par,
			word_freq.begin(), word_freq.end(),
			vs.begin(),
			[&](auto word)
			{ return word.first; });

		std::for_each(
			std::execution::par,
			vs.begin(), vs.end(),
			[&document_id, this](std::string_view word)
			{
				word_to_document_freqs_.find(word)->second.erase(document_id);
			});

		documents_.erase(document_id);
		document_ids_.erase(document_id);
	}
}

void SearchServer::RemoveDocument(int document_id)
{
	RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id)
{
	if (documents_.count(document_id) == 0)
	{
		return;
	}

	std::for_each(
		document_to_word_freqs_.at(document_id).begin(),
		document_to_word_freqs_.at(document_id).end(),
		[this, document_id](const auto& word)
		{
			word_to_document_freqs_[word.first].erase(document_id);
		});

	documents_.erase(document_id);
	document_ids_.erase(document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{
	return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const
{
	const auto query = ParseQuery(raw_query);

	std::vector<std::string> plus_words{ query.plus_words.begin(), query.plus_words.end() };
	std::vector<std::string> minus_words{ query.minus_words.begin(), query.minus_words.end() };

	std::vector<std::string_view> matched_words;

	for (const std::string_view word : documents_.at(document_id).words)
	{
		if (std::find(minus_words.begin(), minus_words.end(), word) != minus_words.end())
		{
			return { matched_words, documents_.at(document_id).status };
		}
	}

	for (const std::string_view word : documents_.at(document_id).words)
	{
		if (std::find(plus_words.begin(), plus_words.end(), word) != plus_words.end())
		{
			matched_words.push_back(word);
		}
	}

	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const
{
	auto query = ParseQuery(raw_query);

	std::set<std::string_view> matched_words;

	for (const std::string_view word : query.plus_words)
	{
		if (documents_.at(document_id).words.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.insert(word);
		}
	}
	for (const std::string_view word : query.minus_words)
	{
		if (documents_.at(document_id).words.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.clear();
			break;
		}
	}

	std::vector<std::string_view> matched_words_vector(matched_words.size());

	std::transform(
		matched_words.begin(), matched_words.end(),
		matched_words_vector.begin(),
		[](auto word)
		{ return word; });

	return { matched_words_vector, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string& word) const
{
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word)
{
	return std::none_of(word.begin(), word.end(), [](char c)
		{ return c >= '\0' && c < ' '; });
}

std::deque<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
	std::deque<std::string_view> words;

	auto vec = SplitIntoWords(text);

	for (const std::string_view word : vec)
	{
		if (!IsValidWord(std::string(word)))
		{
			throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
		}
		if (!IsStopWord(std::string(word)))
		{
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
	if (ratings.empty()) {
        return 0;
    }
    auto rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const
{
	if (text.empty())
	{
		throw std::invalid_argument("Query word is empty"s);
	}

	auto word = text;

	bool is_minus = false;

	if (word[0] == '-')
	{
		is_minus = true;
		word = word.substr(1);
	}

	if (word.empty() || word[0] == '-' || !IsValidWord(std::string(word)))
	{
		throw std::invalid_argument("Query word "s + std::string(word) + " is invalid");
	}

	return { word, is_minus, IsStopWord(std::string(word)) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
	SearchServer::Query result;
	for (const auto word : SplitIntoWords(text))
	{
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop)
		{
			if (query_word.is_minus)
			{
				result.minus_words.push_back(query_word.data);
			}
			else
			{
				result.plus_words.push_back(query_word.data);
			}
		}
	}
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const
{
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}