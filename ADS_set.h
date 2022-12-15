#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <initializer_list>
#include <vector>

template <typename Key, size_t N = 7>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using const_iterator = Iterator;
    using iterator = Iterator;
    using key_compare = std::less<key_type>;                         // B+-Tree
    using key_equal = std::equal_to<key_type>;                       // Hashing
    using hasher = std::hash<key_type>;                              // Hashing
private:
    struct element {
        enum class ElementMode { free, used, end };
        key_type key;
        ElementMode mode{ ElementMode::free };
        element* next{ nullptr };

        ~element() {
            delete next;
        }
    };

    element* table{ nullptr };
    float max_lf{ 2 };
    size_type table_size{ 0 }, curr_size{ 0 };

    //Methods
    size_type h(const key_type& key) const {
        return hasher{}(key) % table_size;
    }


    void rehash(size_type n) {
        size_type new_table_size{ std::max(N, std::max(n,size_type(curr_size / max_lf))) };
        element *new_table {new element[new_table_size + 1]};
        new_table[new_table_size].mode = element::ElementMode::end;
        size_type old_table_size{ table_size };
        element* old_table{ table };
        curr_size = 0;
        table = new_table;
        table_size = new_table_size;
        for (size_type idx{ 0 }; idx < old_table_size; ++idx) {
            if (old_table[idx].mode == element::ElementMode::used) {
                insert_(old_table[idx].key);
                element* collision{ old_table[idx].next };
                for (; collision != nullptr; collision = collision->next) {
                    insert_(collision->key);
                }
            }
        }
        delete[] old_table;
        // table[table_size].mode = element::ElementMode::end;
    }


    void reserve(size_type n) {
        if (n > table_size * max_lf) {
            size_type new_table_size{ table_size };
            do {
                new_table_size = new_table_size * 2 + 1;
            } while (n > new_table_size * 2);
            rehash(new_table_size);
        }
    }


    std::pair<element*, element*> insert_(const key_type& key) {
        size_type index = h(key);

        if (table[index].mode == element::ElementMode::used) {
            element* temp = new element();

            element* collision{ table + index };
            for (; collision->next != nullptr; collision = collision->next) {}

            collision->next = temp;
            collision->next->key = key;
            collision->next->mode = element::ElementMode::used;
            curr_size++;
            return {table + index, collision->next};
        }
        else {
            table[index].key = key;
            table[index].mode = element::ElementMode::used;
            curr_size++;
            return {table + index, nullptr};
        }
    }


    std::pair<element*, element*> find_(const key_type& key) const {
        size_type index = h(key);
        if (table[index].mode == element::ElementMode::free) {
            return {nullptr, nullptr};
        }
        if (key_equal{}(table[index].key, key)) {
            return {table + index, nullptr};
        }
        else {
            element* collision{ table[index].next };
            for (; collision != nullptr; collision = collision->next) {
                if (key_equal{}(collision->key, key)) {
                    return {table + index, collision};
                }
            }
            return {nullptr, nullptr};
        }
    }
public:
    ADS_set() {
        rehash(N);
    }


    ADS_set(std::initializer_list<key_type> ilist) : ADS_set() {
        insert(ilist);
    }


    template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set() {
        insert(first, last);
    }


    ADS_set(const ADS_set &other): ADS_set{} {
        reserve(other.curr_size);
        for (const auto &k: other) {
            insert_(k);
        }
    }


    ADS_set &operator=(const ADS_set &other) {
        if (this == &other) return *this;
        ADS_set tmp{other};
        swap(tmp);
        return *this;
    }


    ADS_set &operator=(std::initializer_list<key_type> ilist) {
        ADS_set tmp{ilist};
        swap(tmp);
        return *this;
    }


    ~ADS_set() {
        delete[] table;
    }


    size_type size() const {
        return curr_size;
    }


    bool empty() const {
        return curr_size == 0;
    }


    void insert(std::initializer_list<key_type> ilist) {
        insert(std::begin(ilist), std::end(ilist));
    }


    template<typename InputIt> void insert(InputIt first, InputIt last)
    {
        for (auto it{ first }; it != last; ++it) {
            if (!count(*it)) {
                reserve(curr_size + 1);
                insert_(*it);
            }
        }
    }


    size_type count(const key_type& key) const {
        // return find_(key) != nullptr;
        return !!find_(key).first;
    };


    void clear() {
        ADS_set tmp;
        swap(tmp);
    }


    void swap(ADS_set &other) {
        using std::swap;
        swap(table, other.table);
        swap(curr_size, other.curr_size);
        swap(table_size, other.table_size);
        swap(max_lf, other.max_lf);
    }


    size_type erase (const key_type& key) {
        if (count(key)) {
            size_type index = h(key);
            element *current = table + index;

            if(current->next == nullptr) {
                current->mode = element::ElementMode::free;
            } else {
                if(key_equal{}(current->key, key)) {
                    std::swap(current->key, current->next->key);
                }

                element *ref = nullptr;
                for (element *collision = table + index; collision != nullptr; ref = collision, collision = collision->next) {
                    if (key_equal{}(collision->key, key)) {
                        ref->next = collision->next;
                        collision->next = nullptr;
                        delete collision;
                        break;
                    }
                }
            }
            --curr_size;
            return 1;
        }
        return 0;
    }


    void dump(std::ostream& o = std::cerr) const {
        o << "curr_size = " << curr_size << ", table_size = " << table_size << "\n";
        for (size_type index{ 0 }; index < table_size; ++index) {
            o << index << ": ";
            if (table[index].mode == element::ElementMode::free) {
                o << "--free";
            }
            else {
                o << table[index].key;
                element* collision{ table[index].next };
                for (; collision != nullptr; collision = collision->next) {
                    o << " -- " << collision->key;
                }
            }
            o << "\n";
        }
    }


    const_iterator begin() const { return const_iterator{table}; }
    const_iterator end() const { return const_iterator{table + table_size}; }

    friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
        if (lhs.curr_size != rhs.curr_size) return false;
        for (const auto &k: rhs) if (!lhs.count(k)) return false;
        return true;
    }
    friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {
        return !(lhs == rhs);
    }


    std::pair<iterator,bool> insert(const key_type &key) {
        std::pair<element*,element*>  find_p {find_(key)};
        if (find_p.first != nullptr) {
            return {iterator{find_p.first, find_p.second},false};
        }
        reserve(curr_size+1);
        std::pair<element*,element*> in_p {insert_(key)};
        return {iterator{in_p.first, in_p.second},true};
    }

    iterator find(const key_type &key) const {
        auto p {find_(key)};
        if (p.first) return iterator{p.first, p.second};
        return end();
    }
};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::forward_iterator_tag;
private:
    element* rowpos;
    element* colpos;
    void skip() {
        while (rowpos->mode == element::ElementMode::free && rowpos->mode != element::ElementMode::end) {
            ++rowpos;
        }
    }
public:
    explicit Iterator(element *rowpos = nullptr, element *colpos = nullptr): rowpos{rowpos}, colpos{colpos} {
        if(rowpos) {
            skip();
        }
    };
    reference operator*() const {
        if(colpos != nullptr) {
            return colpos->key;
        }
        return rowpos->key;
    }
    pointer operator->() const {
        if(colpos != nullptr) {
            return &colpos->key;
        }
        return &rowpos->key;
    }
    Iterator &operator++() {
        if(colpos == nullptr) {
            if(rowpos->next != nullptr) {
                colpos = rowpos->next;
                // std::cout << "it1\n";
                return *this;
            } else {
                colpos = nullptr;
                ++rowpos;
                skip();
                // std::cout << "it2\n";
            }
        } else {
            if(colpos->next != nullptr) {
                colpos = colpos->next;
                // std::cout << "it3\n";
            } else {
                colpos = nullptr;
                ++rowpos;
                skip();
                // std::cout << "it4\n";
            }
        }
        return *this;
    }
    Iterator operator++(int) { auto rc {*this}; ++*this; return rc; }
    friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
        if(lhs.rowpos != rhs.rowpos) {
            return false;
        } else {
            return lhs.colpos == rhs.colpos;
        }
    }
    friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { return !(lhs == rhs); }
};

template <typename Key, size_t N> void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) {
    lhs.swap(rhs);
}

#endif // ADS_SET_H