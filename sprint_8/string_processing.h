#pragma once
#include <set>
#include <vector>
#include <string>
#include <string_view>
#include"search_server.h"

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str : strings)
    {
        if (!str.empty())
        {
            non_empty_strings.insert(std::string(str));
        }
    }
    return non_empty_strings;
}

std::vector<std::string> SplitIntoWords(const std::string& text);
std::vector<std::string_view> SplitIntoWords(const std::string_view text);
