#pragma once
#include <vector>
#include <string>
#include <set>

std::list<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::string(str));
        }
    }
    return non_empty_strings;
}
