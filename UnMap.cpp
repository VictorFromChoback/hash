#include <memory>
#include <iostream>
#include <utility>
#include <iterator>


template<typename T, typename Allocator = std::allocator<T> >
class List
{
    template<typename U, typename AllocatorOut>
    friend std :: ostream& operator<<(std::ostream& out, const List<U, AllocatorOut>& A);

public:
    class Node
    {
    public:
        T* data_;
        Node* next_ = nullptr;
        Node* prev_ = nullptr;

        template<typename... Args>
        Node(Allocator& alloc, Args&& ...args) {
            data_ = std::allocator_traits<Allocator>::allocate(alloc, 1);
            std::allocator_traits<Allocator>::construct(alloc, data_, std::forward<Args>(args)...);
        }
        Node() : data_(nullptr) {}
        ~Node() {}
        void setSubsequent(Node* ptr)
        {
            this->next_ = ptr;
            if (ptr) {
                ptr->prev_ = this;
            }
        }
    };

public:
    template<bool isConst>
    class myIter {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename std::conditional<isConst, const T, T>::type;
        using difference_type = size_t;
        using pointer = typename std::conditional<isConst, const T*, T*>::type;
        using reference = typename std::conditional<isConst, const T&, T&>::type;

        Node* currentNode = nullptr;
        myIter& operator++() {
            currentNode = currentNode->next_;
            return *this;
        }
        myIter operator++(int) {
            myIter answer = *this;
            ++(*this);
            return answer;
        }
        myIter() : currentNode(nullptr) {}
        myIter(Node* other) : currentNode(other) {}
        template<bool otherConst>
        myIter(const myIter<otherConst>& other) : currentNode(other.currentNode) {}
        bool operator==(const myIter& other) {
            return currentNode == other.currentNode;
        }
        bool operator!=(const myIter& other) {
            return !this->operator==(other);
        }
        operator bool() const {
            return currentNode;
        }
        reference operator*() {
            return *(currentNode->data_);
        }

        pointer operator->() {
            return currentNode->data_;
        }
    };

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    Allocator alloc_;
    using additionalAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    additionalAllocator nodeAlloc_;
    size_t size_ = 0;
    template<typename... Args>
    Node* requireNode(Args&& ...args);
    void removeNode(Node* ptr);
    void makeHeadTail();

public:

    using iterator = myIter<false>;
    using const_iterator = myIter<true>;

    bool headEmpty() {return !head_;}
    explicit List(const Allocator& alloc = Allocator());
    List(size_t count, const T& value = T(), const Allocator& alloc = Allocator());
    List(const List<T, Allocator>& A);
    List(List<T, Allocator>&& A);
    ~List();
    iterator begin();
    iterator end();
    const_iterator cbegin() const;
    const_iterator cend() const;
    List<T, Allocator>& operator=(const List<T, Allocator>& A);
    List<T, Allocator>& operator=(List<T, Allocator>&& A);
    size_t size() const;
    void push_back(const T& value);
    void push_back(T&& value);
    void push_front(const T& value);
    void push_front(T&& value);
    void pop_back();
    void pop_front();
    void insert_after(Node* ptr, const T& value);
    void insert_after(Node* ptr, T&& value);
    iterator insert_after_iterator(iterator it, const T& value);
    iterator insert_after_iterator(iterator it, T&& value);
    Node* erase(Node* ptr);
    iterator erase(iterator it);  // Returns subsequent iterator
    const_iterator erase(const_iterator it);
    void clear();
    Node* first() {return head_;}
    template<typename sideAllocator = Allocator>
    void concatenate(const List<T, sideAllocator>& A);
    Node* next(Node* ptr);
    template<typename... Args>
    Node* emplace(Node* pos, Args&& ...args);
    template<typename... Args>
    iterator emplace(iterator pos, Args&& ...args);
    void swap(List<T, Allocator>& other) noexcept;

};

template<typename T, typename Allocator>
void List<T, Allocator>::swap(List<T, Allocator>& other) noexcept {
    std::swap(head_,other.head_);
    std::swap(tail_, other.tail_);
    std::swap(nodeAlloc_, other.nodeAlloc_);
    std::swap(alloc_, other.alloc_);
    std::swap(size_, other.size_);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(List<T, Allocator>&& A) {
    head_ = A.head_;
    tail_ = A.tail_;
    nodeAlloc_ = std::move(A.nodeAlloc_);
    alloc_ = std::move(A.alloc_);
    size_ = A.size_;
    A.head_ = nullptr;
    A.tail_ = nullptr;
    A.size_ = 0;

}

template<typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(List<T, Allocator>&& A) {
    clear();
    if (this != &A) {
        List<T, Allocator>(std::move(A)).swap(*this);
    }
    return *this;
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::Node* List<T, Allocator>::requireNode(Args&& ...args) {
    Node* ptr = std::allocator_traits<additionalAllocator>::allocate(nodeAlloc_, 1);
    std::allocator_traits<additionalAllocator>::construct(nodeAlloc_, ptr, alloc_, std::forward<Args>(args)...);
    return ptr;
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::Node* List<T, Allocator>::emplace(Node* pos, Args&& ...args) {
    if (!pos) {
        pos = head_;
    }

    Node* newNode = requireNode(std::forward<Args>(args)...);
    Node* afterPtr = pos->next_;
    pos->setSubsequent(newNode);
    newNode->setSubsequent(afterPtr);
    ++size_;
    return newNode;
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::iterator List<T, Allocator>::emplace(iterator pos, Args&&... args) {
    return iterator(emplace(pos.currentNode, std::forward<Args>(args)...));
}


template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() {
    return iterator(head_->next_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const {
    return const_iterator(head_->next_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::insert_after_iterator(
                            typename List<T, Allocator>::iterator it, const T& value) {
    Node* ptr = it.currentNode;
    insert_after(ptr, value);
    return iterator(ptr->next_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::insert_after_iterator(
                            typename List<T, Allocator>::iterator it, T&& value) {

    Node* ptr = it.currentNode;
    insert_after(ptr, std::move(value));
    return iterator(ptr->next_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() {
    return iterator(tail_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cend() const {
    return const_iterator(tail_);
}

template<typename T, typename Allocator>
void List<T, Allocator>::removeNode(Node* ptr) {
    std::allocator_traits<Allocator>::destroy(alloc_, ptr->data_);
    std::allocator_traits<Allocator>::deallocate(alloc_, ptr->data_, 1);
    std::allocator_traits<additionalAllocator>::destroy(nodeAlloc_, ptr);
    std::allocator_traits<additionalAllocator>::deallocate(nodeAlloc_, ptr, 1);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& alloc) :
                    alloc_(std::allocator_traits<Allocator>::select_on_container_copy_construction(alloc)) {
    makeHeadTail();
}

template<typename T, typename Allocator>
void List<T, Allocator>::makeHeadTail() {
    head_ = new Node();
    tail_ = new Node();
    head_->setSubsequent(tail_);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t count, const T& value, const Allocator& alloc) : List(alloc) {
    for (size_t i = 0; i < count; ++i) {
        push_back(value);
    }
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
    insert_after(tail_->prev_, value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(T&& value) {
    insert_after(tail_->prev_, std::move(value));
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
    insert_after(head_, value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(T&& value) {
    insert_after(head_, std::move(value));
}


template<typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
    Node* ptr = tail_->prev_;
    (ptr->prev_)->setSubsequent(tail_);
    removeNode(ptr);
    --size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
    Node* ptr = head_->next_;
    head_->setSubsequent(ptr->next_);
    removeNode(ptr);
    --size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::insert_after(Node* ptr, const T& value) {
    emplace(ptr, value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::insert_after(Node* ptr, T&& value) {
    emplace(ptr, std::move(value));
}

template<typename T, typename Allocator>
size_t List<T, Allocator>::size() const {
    return size_;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::Node* List<T, Allocator>::erase(Node* ptr) {
    (ptr->prev_)->setSubsequent(ptr->next_);
    Node* answer = ptr->next_;
    --size_;
    removeNode(ptr);
    return answer;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::erase(const_iterator it) {
    return const_iterator(erase(it.currentNode));
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::erase(iterator it) {
    return iterator(erase(it.currentNode));
}

template<typename T, typename Allocator>
void List<T, Allocator>::clear() {
    while (size()) {
        pop_back();
    }
}

template<typename T, typename Allocator>
template<typename sideAllocator>
void List<T, Allocator>::concatenate(const List<T, sideAllocator>& A) {
    for (Node* it = A.head_->next_; it != A.tail_; it = it->next_) {
        push_back(*(it->data_));
    }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List<T, Allocator>& A) : List(A.alloc_) {
    concatenate(A);
}

template<typename T, typename Allocator>
List<T, Allocator>::~List() {
    clear();
    delete head_;
    delete tail_;
}

template<typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List<T, Allocator>& A) {
    clear();
    alloc_ = std::allocator_traits<Allocator>::select_on_container_copy_construction(A.alloc_);
    nodeAlloc_ = std::allocator_traits<additionalAllocator>::select_on_container_copy_construction(A.nodeAlloc_);
    concatenate(A);
    return (*this);
}


template<typename U, typename AllocatorOut>
std::ostream& operator<<(std::ostream& out, const List<U, AllocatorOut>& A) {
    for (typename List<U, AllocatorOut>::Node* it = A.head_->next_; it != A.tail_; it = it -> next_) {
        out << *(it->data_) << ' ';
    }
    return out;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::Node* List<T, Allocator>::next(Node* ptr) {
    return ptr->next_;
}

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
    static constexpr double baseMaxLoadFactor_ = 5;
    static const size_t baseSize_ = 10;
    static constexpr double resizeMultiply = 2;

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
    template<bool isConst>
    class myIter {
    public:
        using difference_type = size_t;
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename std::conditional<isConst, const NodeType, NodeType>::type;
        using pointer = value_type*;
        using reference = value_type&;

        subIterator data;
        myIter& operator++() {
            ++data;
            return *this;
        }

        myIter operator++(int) {
            myIter copy = *this;
            ++(*this);
            return copy;
        }

        myIter& operator+=(size_t k) {
            for (size_t i = 0; i < k; ++i) {
                ++(*this);
            }
            return *this;
        }

        myIter operator+(size_t k) {
            myIter copy = *this;
            return (copy += k);
        }

        myIter& operator=(const myIter& other) {
            data = other.data;
            return *this;
        }

        template<bool otherConst>
        myIter(const myIter<otherConst>& other) : data(other.data) {}

        myIter(const subIterator& other) : data(other) {}

        myIter(const subConstIterator& other) : data(other) {}

        pointer operator->() {
            return data.operator->();
        }

        reference operator*() {
            return *data;
        }

        bool operator==(const myIter& other) {
            return data == other.data;
        }

        bool operator!=(const myIter& other) {
            return !(*this == other);
        }

        operator bool() const {
            return data;
        }

    };

public:

    using iterator = myIter<false>;
    using const_iterator = myIter<true>;

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
    void swap(UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) noexcept;

private:
    template<typename... Args>
    std::pair<iterator, bool> insertNode_(const NodeType& value, Args&& ...args);

};

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::swap(UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) noexcept {

    std::swap(dataArray_, other.dataArray_);
    std::swap(listOfNodes, other.listOfNodes);
    std::swap(capacity_, other.capacity_);
    std::swap(size_, other.size_);
    std::swap(maxLoadFactor_, other.maxLoadFactor_);
}


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
template<typename... Args>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
                                UnorderedMap<Key, Value, Hash, Equal, Alloc>::insertNode_(
                                                                                        const NodeType& value,
                                                                                        Args&& ...args) {

    checkLoad_();
    size_t indHash = hashFunction_(value.first) % capacity_;
    subIterator currentNode = dataArray_[indHash];

    if (!currentNode) {
        listOfNodes.emplace(subIterator(nullptr), std::forward<Args>(args)...);
        ++size_;
        dataArray_[indHash] = listOfNodes.begin();
        return {listOfNodes.begin(), true};
    }

    for (subIterator it = currentNode; it != listOfNodes.end() && theSameHash_(it, value.first); ++it) {
        NodeType& cur = *it;
        if (equalityFunction_(value.first, cur.first)) {
            return {iterator(it), false};
        }
    }

    ++size_;
    subIterator answer = listOfNodes.emplace(currentNode, std::forward<Args>(args)...);
    return {answer, true};
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
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() {
    return UnorderedMap::iterator(listOfNodes.begin());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::
                                                                                                    cbegin() const {
    return UnorderedMap::const_iterator(listOfNodes.cbegin());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::
                                                                                                        cend() const {
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
    capacity_ = other.capacity_;
    dataArray_ = new subIterator [capacity_];
    listOfNodes = other.listOfNodes;
    size_ = other.size_;
    maxLoadFactor_ = other.maxLoadFactor_;

    size_t last = capacity_;

    for (subIterator it = listOfNodes.begin(); it != listOfNodes.end(); ++it) {
        size_t h = hashFunction_(it->first) % capacity_;
        if (h != last) {
            dataArray_[h] = it;
        }
        last = h;
    }

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
UnorderedMap<Key, Value, Hash, Equal, Alloc>& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(
                                            const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) {

    if (this != &other) {
        UnorderedMap<Key, Value, Hash, Key, Alloc>(other).swap(*this);
    }

    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(
                                            UnorderedMap<Key, Value, Hash, Equal, Alloc>&& other) {

    if (this != &other) {
        UnorderedMap<Key, Value, Hash, Equal, Alloc>(std::move(other)).swap(*this);
    }

    return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::~UnorderedMap() {
    delete[] dataArray_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
                                UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(const NodeType& x) {
    checkLoad_();
    return insertNode_(x, x);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename U>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
                                UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(U&& x) {
    checkLoad_();
    return insertNode_(std::forward<U>(x), std::forward<U>(x));
}


template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(iterator it) {
    --size_;
    size_t curHash = hashFunction_((*it).first) % capacity_;
    subIterator mainIter = dataArray_[curHash];
    if (mainIter != it.data) {
        listOfNodes.erase(it.data);
    } else {
        iterator anotherIt = it + static_cast<size_t>(1);
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
        capacity_ = static_cast<size_t>(static_cast<double>(capacity_) * resizeMultiply);
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
        if (!ar[i].size()) {
            continue;
        }
        dataArray_[i] = *(ar[i].begin());

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

    delete[] ar;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator UnorderedMap<Key, Value, Hash, Equal, Alloc>::
                                                                                        find(const Key& key) {
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
    iterator resFind = find(key);

    if (resFind == end()) {
        auto answer = insertNode_({key, Value()}, key, std::move(Value()));
        return answer.first->second;
    } else {
        return resFind->second;
    }

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
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
                                UnorderedMap<Key, Value, Hash, Equal, Alloc>::emplace(Args&&... args) {

    checkLoad_();
    NodeType* x = allocateNode_(std::forward<Args>(args)...);
    auto answer = insertNode_(*x, std::forward<Args>(args)...);

    deallocateNode_(x);
    return answer;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename... Args>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::NodeType*
                    UnorderedMap<Key, Value, Hash, Equal, Alloc>::allocateNode_(Args&& ...args) {
    NodeType* ptr = std::allocator_traits<Alloc>::allocate(alloc_, 1);
    std::allocator_traits<Alloc>::construct(alloc_, ptr, std::forward<Args>(args)...);
    return ptr;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::deallocateNode_(NodeType* ptr) {
    std::allocator_traits<Alloc>::destroy(alloc_, ptr);
    std::allocator_traits<Alloc>::deallocate(alloc_, ptr, 1);
}
