#pragma once

#include <vector>
#include <iostream>
#include "document.h"

template <typename Iterator>
class IteratorRange {
    public:
    
        explicit IteratorRange(const Iterator& first, const Iterator& last)
            : begin_(first), end_(last)
        {
            size_ = distance(first, last);
        }
        auto begin() const{
            return begin_;
        }
    
        auto end() const{
            return end_;
        }
    
        auto size() const{
            return size_;
        }
    
    
    private:
        Iterator begin_;
        Iterator end_;
        size_t size_;
};

template <typename Iterator>
class Paginator {
    public:

        // конструктор
        Paginator(Iterator begin, Iterator end, size_t page_size) {

            // Если умещается на одну страницу, то сразу добавялем в выводимые документы
            if (distance(begin, end) < page_size) {
                IteratorRange oneDocumentOnPage = IteratorRange(begin, end);
                DocumentsOnPage_.push_back(oneDocumentOnPage);
            } else {
                // Итератор идёт от begin до end, но принимает только значения в диапазоне page_size с помощью advance(it, page_size)
                for (auto it = begin; it != end; std::advance(it, page_size)) {
                    // Пушим документы в ветор документво на странице
                    // Случай когда документов больше чем на страницу
                    if (std::distance(it, end) >= page_size)
                        {
                            IteratorRange oneDocumentOnPage = IteratorRange(it, it + page_size);
                            DocumentsOnPage_.push_back(oneDocumentOnPage);
                        }
                        // Случай когда документов меньше чем на страницу
                        else
                        {
                            IteratorRange oneDocumentOnPage = IteratorRange(it, end);
                            DocumentsOnPage_.push_back(oneDocumentOnPage);
                            break;
                        }
                }
            }
        }

        //необходимый для вывода метод begin
        auto begin() const {
            return DocumentsOnPage_.begin();
        }

        //необходимый для вывода метод end
        auto end() const {
            return DocumentsOnPage_.end();
        }
            
    private:
    //Внутри объекта Paginator вы просто спрячете вектор таких вот IteratorRange и будете заполнять его в конструкторе объекта Paginator
        std::vector<IteratorRange<Iterator>> DocumentsOnPage_;
};

// Функция нужна только для создания paginator
// std::size_t is the unsigned integer type of the result of the sizeof operator
template <typename Container> 
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

//Перегрузка оператора вывода для класса IteratorRange
template <typename Iterator>
std::ostream &operator<<(std::ostream &out, const IteratorRange<Iterator>& It)
{
    for (auto i = It.begin(); i != It.end(); ++i)
        out << *i;
    return out;
}