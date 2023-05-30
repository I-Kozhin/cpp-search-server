#include "string_processing.h"
#include <vector>
#include <string>
#include <string_view>

using namespace std;

list<string_view> SplitIntoWords(string_view text) {
    list<string_view> words;
    
    int i = 0;
    int len = 0;
    for_each(text.begin(), text.end(), [&](const char& c) {
        if (c == ' ') {
            if (len) {
                words.push_back(text.substr(i, len));
                i += len;
                len = 0;
            }
            ++i;
        } else {
            ++len;
        }
    });
    
    if (len) {
        words.push_back(text.substr(i, len));
    }
    
    return words;
}
