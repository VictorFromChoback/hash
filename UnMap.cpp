#include "ListAndAlloc.h"
#include <utility>
#include <iterator>

template<typename T, typename U>
std::ostream& operator<<(std::ostream& out, const std::pair<T, U>& p) {
    out << p.first << ' ' << p.second;
    return out;
}


template<
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename Equal = std::equal_to<Key>,
    typename Alloc = std::allocator<std::pair<const Key, Value> > >
class UnorderedMap {
public:
    using NodeType = std::pair<const Key, Value>;
    template<typename T, typename U, typename H, typename E, typename A>
    friend std::ostream& operator&(std::ostream& out, UnorderedMap<T, U, H, E, A>& Table);

private:
    using NodePtr = typename List<NodeType, Alloc>::Node*;
    using subIterator = typename List<NodeType, Alloc>::iterator;
    using subConstIterator = typename List<NodeType, Alloc>::const_iterator;

    subIterator* dataArray_;
    double maxLoadFactor_;
    Hash hashFunction_;
    Equal equalityFunction_;
    Alloc alloc_;
    static constexpr double baseMaxLoadFactor_ = 0.5;
    static const size_t baseSize_ = 10;
    static const size_t resizeMultiply = 4;
    size_t size_ = 0;
    size_t capacity_;
    List<NodeType, Alloc> listOfNodes;
    static bool theSameHash_(subIterator it, const Key& value);
    static bool theSameHash_(subConstIterator it, const Key& value);
    void rehash_(size_t newSize = 0);
    void checkLoad_();
    template<typename... Args>
    NodeType* allocateNode_(Args&& ...args);
    void deallocateNode_(NodeType* ptr);


public:

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = NodeType;
        using pointer = NodeType*;
        using difference_type = size_t;
        using reference = NodeType&;
        subIterator data;

        iterator& operator++();
        iterator operator++(int) {return iterator(data++);}
        iterator& operator+=(size_t k);
        iterator operator+(size_t k) {iterator it = *this; return it += k;}
        iterator& operator=(const iterator& it) {data = it.data; return *this;}
        iterator(const subIterator& other) : data(other) {}
        value_type* operator->() {return data.operator->();}
        bool operator==(const iterator& other) {return data == other.data;}
        bool operator!=(const iterator& other) {return !(*this == other);}
        value_type& operator*() {return *data;}

    };

    class const_iterator {
    public:
        using value_type = const NodeType;
        using pointer = value_type*;
        using difference_type = size_t;
        subConstIterator data;

        const_iterator& operator=(const const_iterator& it) {data = it.data; return *this;}
        const_iterator& operator++();
        const_iterator operator++(int) {return const_iterator(data++);}
        const_iterator& operator+=(size_t k);
        const_iterator operator+(size_t k) {const_iterator it = *this; return it += k;}
        const_iterator(const subConstIterator& other) : data(other) {}
        const_iterator(const iterator& other) : data(other.data) {}
        bool operator==(const const_iterator& other) {return data == other.data;}
        bool operator!=(const const_iterator& other) {return !(*this == other);}
        value_type& operator*() {return *data;}
        const value_type* operator->() {return data.operator->();}
    };

    UnorderedMap();
    UnorderedMap(const UnorderedMap& other);
    UnorderedMap(UnorderedMap&& other);
    UnorderedMap& operator=(const UnorderedMap& other);
    UnorderedMap& operator=(UnorderedMap&& other);
    ~UnorderedMap();
    double load_factor() const;
    iterator begin();
    const_iterator cbegin() const;
    const_iterator cend() const;
    iterator end();
    Value& operator[](const Key& key);
    Value& at(const Key& key);
    size_t capacity() const {return capacity_;}
    size_t size() const {return size_;}
    template<typename Iter>
    void insert(Iter first, Iter second);
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args);
    std::pair<iterator, bool> insert(const NodeType& x);
    template<typename U>
    std::pair<iterator, bool> insert(U&& x);
    void erase(iterator it);
    void erase(iterator first, iterator second);
    iterator find(const Key& key);
    void max_load_factor(double alpha);
    void reserve(size_t count);

};

template<typename T, typename U, typename H, typename E, typename A>
std::ostream& operator<<(std::ostream& out, UnorderedMap<T, U, H, E, A>& Table) {
    out << std::endl << "This is my table" << std::endl;
    for (typename UnorderedMap<T, U, H, E, A>::iterator it = Table.begin(); it != Table.end(); ++it) {
        out << *it << std::endl;
    }
    out << "End of my table" << std::endl;
    return out;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
bool UnorderedMap<Key, Value, Hash, Equal, Alloc>::theSameHash_(subIterator it, const Key& value) {
    Hash hashF;
    size_t curHash = hashF((*it).first);
    size_t valueHash = hashF(value);
    return curHash == valueHash;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
bool UnorderedMap<Key, Value, Hash, Equal, Alloc>::theSameHash_(subConstIterator it, const Key& value) {
    Hash hashF;
    size_t curHash = hashF((*it).first);
    size_t valueHash = hashF(value);
    return curHash == valueHash;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::checkLoad_() {
    if (load_factor() >= maxLoadFactor_) {
        rehash_();
    }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
double UnorderedMap<Key, Value, Hash, Equal, Alloc>::load_factor() const {
    double x = static_cast<double>(size_);
    double y = static_cast<double>(capacity_);
    return x / y;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator& UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator::operator++() {
    ++data;
    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::max_load_factor(double alpha) {
    maxLoadFactor_ = alpha;
    checkLoad_();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::reserve(size_t count) {
    size_t newCount = static_cast<size_t>((static_cast<double>(count) / maxLoadFactor_)) + 1;
    rehash_(newCount);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator& UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator::operator++() {
    ++data;
    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator& UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator::operator+=(size_t k) {
    for (size_t i = 0; i < k; ++i) {
        this->operator++();
    }
    return *this;
}


template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator& UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator::operator+=(size_t k) {
    for (size_t i = 0; i < k; ++i) {
        this->operator++();
    }
    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() {
    return UnorderedMap::iterator(listOfNodes.begin());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::cbegin() const {
    return UnorderedMap::const_iterator(listOfNodes.cbegin());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::cend() const {
    return UnorderedMap::const_iterator(listOfNodes.cend());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::end() {
    return UnorderedMap::iterator(listOfNodes.end());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap() {
    dataArray_ = new subIterator [baseSize_];
    capacity_ = baseSize_;
    size_ = 0;
    maxLoadFactor_ = baseMaxLoadFactor_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(const UnorderedMap& other) {
    dataArray_ = other.dataArray_;
    listOfNodes = other.listOfNodes;
    capacity_ = other.capacity_;
    size_ = other.size_;
    maxLoadFactor_ = other.maxLoadFactor_;
}


template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(UnorderedMap&& other) {
    dataArray_ = other.dataArray_;
    listOfNodes = std::move(other.listOfNodes);
    capacity_ = other.capacity_;
    size_ = other.size_;
    maxLoadFactor_ = other.maxLoadFactor_;
    other.dataArray_ = nullptr;
    other.size_ = 0;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) {
    dataArray_ = other.dataArray_;
    listOfNodes = other.listOfNodes;
    capacity_ = other.capacity_;
    size_ = other.size_;
    maxLoadFactor_ = other.maxLoadFactor_;
    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(UnorderedMap<Key, Value, Hash, Equal, Alloc>&& other) {
    dataArray_ = other.dataArray_;
    listOfNodes = std::move(other.listOfNodes);
    capacity_ = other.capacity_;
    size_ = other.size_;
    maxLoadFactor_ = other.maxLoadFactor_;
    other.dataArray_ = nullptr;
    other.size_ = 0;
    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::~UnorderedMap() {
    delete[] dataArray_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool> UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(const NodeType& x) {
    checkLoad_();
    size_t indHash = hashFunction_(x.first) % capacity_;
    subIterator currentNode = dataArray_[indHash];

    if (!currentNode) {
        listOfNodes.push_front(x);
        ++size_;
        dataArray_[indHash] = listOfNodes.begin();
        return {listOfNodes.begin(), true};
    }

    for (subIterator it = currentNode; it != listOfNodes.end() && theSameHash_(it, x.first); ++it) {
        NodeType& cur = *it;
        if (equalityFunction_(x.first, cur.first)) {
            return {iterator(it), false};
        }
    }

    ++size_;
    subIterator answer = listOfNodes.insert_after_iterator(currentNode, x);
    return {answer, true};

}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename U>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool> UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(U&& x) {
    checkLoad_();
    size_t indHash = hashFunction_(x.first) % capacity_;
    subIterator currentNode = dataArray_[indHash];

    if (!currentNode) {
        subIterator bad(nullptr);
        listOfNodes.emplace(bad, std::forward<U>(x));
        ++size_;
        dataArray_[indHash] = listOfNodes.begin();
        return {listOfNodes.begin(), true};
    }

    for (subIterator it = currentNode; it != listOfNodes.end() && theSameHash_(it, x.first); ++it) {
        NodeType& cur = *it;
        if (equalityFunction_(x.first, cur.first)) {
            return {iterator(it), false};
        }
    }

    ++size_;
    subIterator answer = listOfNodes.emplace(currentNode, std::forward<U>(x));
    return {answer, true};

}


template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(iterator it) {
    --size_;
    size_t curHash = hashFunction_((*it).first) % capacity_;
    subIterator mainIter = dataArray_[curHash];
    if (mainIter != it.data) {
        listOfNodes.erase(it.data);
    } else {
        iterator anotherIt = it + 1;
        if (anotherIt.data == listOfNodes.end() || !theSameHash_(anotherIt.data, (*it).first)) {
            dataArray_[curHash] = subIterator(nullptr);
            listOfNodes.erase(mainIter);
        } else {
            dataArray_[curHash] = anotherIt.data;
            listOfNodes.erase(mainIter);
        }
    }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(iterator first, iterator second) {

    for (; first != second; ++first) {
        erase(first);
    }

}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename Iter>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(Iter first, Iter second) {
    for (Iter it = first; it != second; ++it) {
        insert(*it);
    }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::rehash_(size_t newSize) {
    if (!newSize) {
        capacity_ *= resizeMultiply;
    } else {
        if (newSize < capacity_) {
            return;
        }
        capacity_ = newSize;
    }

    List<subIterator>* ar = new List<subIterator> [capacity_];
    for (iterator it = begin(); it != end(); ++it) {
        size_t tempHash = hashFunction_((*it).first);
        ar[tempHash % capacity_].push_back(it.data);
    }

    delete[] dataArray_;
    dataArray_ = new subIterator [capacity_];

    for (size_t i = 0; i < capacity_; ++i) {
        if (!ar[i].begin()) {
            continue;
        }
        dataArray_[i] = *ar[i].begin();
    }

    typename List<NodeType, Alloc>::Node* v = listOfNodes.first();
    for (size_t i = 0; i < capacity_; ++i) {
        for (typename List<subIterator>::iterator j = ar[i].begin(); j != ar[i].end(); ++j) {
            v->setSubsequent((*j).currentNode);
            v = (*j).currentNode;
        }
    }

    if (v) {
        v->setSubsequent(listOfNodes.end().currentNode);
    }

}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) {
    checkLoad_();
    size_t curHash = hashFunction_(key) % capacity_;
    subIterator mainIter = dataArray_[curHash];

    if (!mainIter) {
        return end();
    }

    for (subIterator it = mainIter; it != listOfNodes.end() && theSameHash_(it, key); ++it) {
        NodeType& cur = *it;
        if (equalityFunction_(cur.first, key)) {
            return iterator(it);
        }
    }

    return end();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator[](const Key& key) {
    checkLoad_();
    size_t curHash = hashFunction_(key) % capacity_;
    subIterator mainIter = dataArray_[curHash];

    if (!mainIter) {
        listOfNodes.push_front({key, Value()});
        ++size_;
        dataArray_[curHash] = listOfNodes.begin();
        return (*dataArray_[curHash]).second;
    }

    for (subIterator it = mainIter; it != listOfNodes.end() && theSameHash_(it, key); ++it) {
        NodeType& cur = *it;
        if (equalityFunction_(cur.first, key)) {
            return cur.second;
        }
    }

    ++size_;
    subIterator it = listOfNodes.insert_after_iterator(mainIter, {key, Value()});
    return (*it).second;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) {
    iterator it = find(key);

    if (it == end()) {
        throw std::out_of_range("No Key");
    }

    return (*it).second;

}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename... Args>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool> UnorderedMap<Key, Value, Hash, Equal, Alloc>::emplace(Args&&... args) {

    checkLoad_();
    NodeType* x = allocateNode_(std::forward<Args>(args)...);
    size_t indHash = hashFunction_(x->first);
    subIterator currentNode = dataArray_[indHash % capacity_];
    subIterator badIter = subIterator(nullptr);

    if (!currentNode) {
        listOfNodes.emplace(badIter, std::forward<Args>(args)...);
        ++size_;
        dataArray_[indHash % capacity_] = listOfNodes.begin();
        return {listOfNodes.begin(), true};
    }

    for (subIterator it = currentNode; it != listOfNodes.end() && theSameHash_(it, x->first); ++it) {
        NodeType& cur = *it;
        if (equalityFunction_(x->first, cur.first)) {
            return {iterator(it), false};
        }
    }

    deallocateNode_(x);
    ++size_;
    subIterator answer = listOfNodes.emplace(currentNode, std::forward<Args>(args)...);
    return {iterator(answer), true};
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename... Args>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::NodeType* UnorderedMap<Key, Value, Hash, Equal, Alloc>::allocateNode_(Args&& ...args) {
    NodeType* ptr = std::allocator_traits<Alloc>::allocate(alloc_, 1);
    std::allocator_traits<Alloc>::construct(alloc_, ptr, std::forward<Args>(args)...);
    return ptr;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::deallocateNode_(NodeType* ptr) {
    std::allocator_traits<Alloc>::destroy(alloc_, ptr);
    std::allocator_traits<Alloc>::deallocate(alloc_, ptr, 1);
}


