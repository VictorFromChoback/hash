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
    class Node {
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

private:
    template<bool isConst>
    class myIter {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename std::conditional<isConst, const T, T>::type;
        using difference_type = size_t;
        using pointer_type = typename std::conditional<isConst, const T*, T*>::type;
        using reference_type = typename std::conditional<isConst, const T&, T&>::type;

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
        myIter(Node* other) : currentNode(other) {}
        myIter(const myIter& other) : currentNode(other.currentNode) {}
        bool operator==(const myIter& other) {
            return currentNode = other.currentNode;
        }
        bool operator!=(const myIter& other) {
            return !this->operator==(other);
        }
        operator bool() const {
            return currentNode;
        }
        reference_type operator*() {
            return *(currentNode->data_);
        }

        pointer_type operator->() {
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
    bool notBuild = true;
    template<typename... Args>
    Node* requireNode(Args&& ...args);
    void removeNode(Node* ptr);
    template<typename... Args>
    void makeHeadTail(Args&& ...args);

public:

    class iterator;

    class const_iterator {
    public:
        const Node* currentNode = nullptr;
        const_iterator& operator++();
        const_iterator operator++(int);
        const_iterator(Node* other) : currentNode(other) {}
        const_iterator() : currentNode(nullptr) {}
        const_iterator(const iterator& other) : currentNode(other.currentNode) {}
        const_iterator& operator=(const_iterator other) {currentNode = other.currentNode; return *this;}
        bool operator==(const const_iterator& other);
        bool operator!=(const const_iterator& other);
        operator bool() const {return currentNode;}
        const T& operator*();
        const T* operator->() {return (currentNode->data_);}

    };

    class iterator {
    public:
        Node* currentNode = nullptr;
        iterator& operator++();
        iterator operator++(int);
        iterator(Node* other) : currentNode(other) {}
        iterator() : currentNode(nullptr) {}
        iterator(const const_iterator& other) : currentNode(other.currentNode) {}
        iterator& operator=(iterator other) {currentNode = other.currentNode; return *this;}
        T* operator->() {return (currentNode->data_);}
        bool operator==(const iterator& other);
        bool operator!=(const iterator& other);
        operator bool() const {return currentNode;}
        T& operator*();
    };

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
    template<typename U>
    void push_back(U&& value);
    template<typename U>
    void push_front(U&& value);
    void pop_back();
    void pop_front();
    void insert_before(Node* ptr, const T& value);
    void insert_after(Node* ptr, const T& value);
    void insert_after(Node* ptr, T&& value);
    void insert_before(Node* ptr, T&& value);
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
    A.notBuild = true;
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
typename List<T, Allocator>::iterator& List<T, Allocator>::iterator::operator++() {
    currentNode = currentNode->next_;
    return *this;
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::Node* List<T, Allocator>::emplace(Node* pos, Args&& ...args) {
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
typename List<T, Allocator>::iterator List<T, Allocator>::iterator::operator++(int) {
    iterator other = *this;
    ++(*this);
    return other;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator& List<T, Allocator>::const_iterator::operator++() {
    currentNode = currentNode->next_;
    return *this;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::const_iterator::operator++(int) {
    const_iterator other = *this;
    ++(*this);
    return other;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() {
    if (!head_) {
        return nullptr;
    } else {
        return iterator(head_->next_);
    }
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const {
    if (!head_) {
        return nullptr;
    } else {
        return const_iterator(head_->next_);
    }

}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::insert_after_iterator(typename List<T, Allocator>::iterator it, const T& value) {
    Node* ptr = it.currentNode;
    insert_after(ptr, value);
    return iterator(ptr->next_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::insert_after_iterator(typename List<T, Allocator>::iterator it, T&& value) {
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
bool List<T, Allocator>::const_iterator::operator==(const const_iterator& other) {
    return other.currentNode == currentNode;
}

template<typename T, typename Allocator>
bool List<T, Allocator>::const_iterator::operator!=(const const_iterator& other) {
    return !(*this == other);
}

template<typename T, typename Allocator>
bool List<T, Allocator>::iterator::operator==(const iterator& other) {
    return other.currentNode == currentNode;
}

template<typename T, typename Allocator>
bool List<T, Allocator>::iterator::operator!=(const iterator& other) {
    return !(*this == other);
}

template<typename T, typename Allocator>
T& List<T, Allocator>::iterator::operator*() {
    return *(currentNode->data_);
}

template<typename T, typename Allocator>
const T& List<T, Allocator>::const_iterator::operator*() {
    return *(currentNode->data_);
}

template<typename T, typename Allocator>
void List<T, Allocator>::removeNode(Node* ptr) {
    std::allocator_traits<Allocator>::destroy(alloc_, ptr->data_);
    std::allocator_traits<Allocator>::deallocate(alloc_, ptr->data_, 1);
    std::allocator_traits<additionalAllocator>::destroy(nodeAlloc_, ptr);
    std::allocator_traits<additionalAllocator>::deallocate(nodeAlloc_, ptr, 1);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& alloc) : alloc_(std::allocator_traits<Allocator>::select_on_container_copy_construction(alloc)) {
    head_ = new Node();
    tail_ = new Node();
}

template<typename T, typename Allocator>
template<typename... Args>
void List<T, Allocator>::makeHeadTail(Args&& ...args) {
    head_ = requireNode(std::forward<Args>(args)...);
    tail_ = requireNode(std::forward<Args>(args)...);
    head_->setSubsequent(tail_);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t count, const T& value, const Allocator& alloc) : List(alloc) {
    for (size_t i = 0; i < count; ++i) {
        push_back(value);
    }
}

template<typename T, typename Allocator>
template<typename U>
void List<T, Allocator>::push_back(U&& value) {
    insert_before(tail_, std::forward(value));
}

template<typename T, typename Allocator>
template<typename U>
void List<T, Allocator>::push_front(U&& value) {
    inseft_after(head_, std::forward(value));
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
void List<T, Allocator>::insert_before(Node* ptr, const T& value) {
    emplace(ptr->prev_, value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::insert_before(Node* ptr, T&& value) {
    emplace(ptr->prev_, std::move(value));
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
    void swap(UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) noexcept;

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

    delete[] ar;
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