#include "string_processing.h"

using namespace std;
using namespace std::literals;

vector<string> SplitIntoWords(const string& text)
{
	vector<string> words;
	string word;
	for (const char c : text)
	{
		if (c == ' ')
		{
			if (!word.empty())
			{
				words.push_back(word);
				word.clear();
			}
		}
		else
		{
			word += c;
		}
	}
	if (!word.empty())
	{
		words.push_back(word);
	}

	return words;
}

vector<string_view> SplitIntoWords(const string_view text)
{

	vector<string_view> words;

	string word;
	auto pos = text.find(' ');
	size_t first = 0;
	while (pos != text.npos)
	{
		words.push_back(text.substr(first, pos - first));
		first = pos + 1;
		pos = text.find(' ', first);
	}
	words.push_back(text.substr(first, pos - first));
	return words;
}

