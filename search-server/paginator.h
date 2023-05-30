#pragma once

#include <vector>
#include <iostream>
#include "document.h"

template <typename Iterator>
class IteratorRange {
    public:
        IteratorRange(Iterator begin, Iterator end)
            : begin_(begin), end_(end), size_(std::distance(begin_, end_))
        {
        }

        Iterator begin() const
        {
            return begin_;
        }

        Iterator end() const
        {
            return end_;
        }

        size_t size() const
        {
            return size_;
        }

    private:
        Iterator begin_, end_;
        size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range)
{
    for (Iterator it = range.begin(); it != range.end(); ++it)
    {
        out << *it;
    }
    return out;
}

template <typename Iterator>
class Paginator {
    public:
        Paginator(Iterator begin, Iterator end, size_t page_size)
        {
            for (size_t left = std::distance(begin, end); left > 0;)
            {
                const size_t current_page_size = std::min(page_size, left);
                const Iterator current_page_end = std::next(begin, current_page_size);
                pages_.push_back({ begin, current_page_end });
                left -= current_page_size;
                begin = current_page_end;
            }
        }

        auto begin() const
        {
            return pages_.begin();
        }

        auto end() const
        {
            return pages_.end();
        }

        size_t size() const
        {
            return pages_.size();
        }

    private:
        std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
    return Paginator(std::begin(c), std::end(c), page_size);
}
