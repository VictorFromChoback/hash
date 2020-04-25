#include <vector>
#include <memory>
#include <iostream>

template <size_t chunkSize>
class FixedAllocator {
private:

    static const size_t sizeBound_ = 512;

    class Chunk_ {
    public:
        char memory[chunkSize];
        Chunk_* nextChunk;
        Chunk_() : nextChunk(nullptr) {}
        ~Chunk_() {}
    };

    Chunk_* firstFree_ = nullptr;

    std :: vector<Chunk_*> pool_;
    size_t size_;

    void getMemory_();

public:

    ~FixedAllocator();
    FixedAllocator();
    FixedAllocator(const FixedAllocator<chunkSize>& A);

    void* allocate();

    void deallocate(void* release);

    static FixedAllocator& use();
};


template<size_t chunkSize>
void FixedAllocator<chunkSize>::getMemory_() {
    firstFree_ = new Chunk_[size_];

    for (size_t i = 0; i + 1 < size_; ++i) {
        firstFree_[i].nextChunk = &firstFree_[i + 1];
    }

    firstFree_[size_ - 1].nextChunk = nullptr;

    pool_.push_back(firstFree_);

    if (size_ < sizeBound_) {
        size_ <<= 1;
    }
}

template<size_t chunkSize>
FixedAllocator<chunkSize>::~FixedAllocator() {
    while (!pool_.empty()) {
        Chunk_* ptr = pool_.back();
        pool_.pop_back();
        delete[] ptr;
    }
}

template<size_t chunkSize>
FixedAllocator<chunkSize>::FixedAllocator() {
    size_ = 64;
}

template<size_t chunkSize>
FixedAllocator<chunkSize>::FixedAllocator(const FixedAllocator<chunkSize>& A) {
    for (size_t i = 0; i < A.pool_.size(); ++i) {
        getMemory_();
    }

    pool_ = A.pool_;
}

template<size_t chunkSize>
void* FixedAllocator<chunkSize>::allocate() {
    if (!firstFree_) {
        getMemory_();
    }

    Chunk_* Return = firstFree_;
    firstFree_ = firstFree_ -> nextChunk;
    return static_cast<void*>(Return);
}

template<size_t chunkSize>
void FixedAllocator<chunkSize>::deallocate(void* release) {
    Chunk_* ChunkRelease = static_cast<Chunk_*>(release);
    ChunkRelease -> nextChunk = firstFree_;
    firstFree_ = ChunkRelease;
}


template<size_t chunkSize>
FixedAllocator<chunkSize>& FixedAllocator<chunkSize>::use() {
    static FixedAllocator<chunkSize> forUse;
    return forUse;
}

template<typename T>
class FastAllocator {
private:
    static const size_t memorySize_ = 32;
    FixedAllocator<sizeof(T) > alloc_;

public:
    using value_type = T;
    using pointer = T*;
    template<typename U>
    class rebind {
    public:
        using other = FastAllocator<U>;
    };

    FastAllocator() {}
    FastAllocator(const FastAllocator& A) : alloc_(A.alloc_) {}
    ~FastAllocator() {}

    pointer allocate(size_t n);

    void deallocate(pointer release, size_t n);

    void construct(pointer p, const T& make) const;

    void destroy(pointer p) const;

    static FastAllocator<T> select_on_container_copy_construction(const FastAllocator<T>& alloc);
};

template<typename T>
typename FastAllocator<T>::pointer FastAllocator<T>::allocate(size_t n) {
    if (n > 1 || sizeof(T) > memorySize_) {
        return std::allocator<T>().allocate(n);
    } else {
        return static_cast<pointer>(alloc_.allocate());
    }
}

template<typename T>
void FastAllocator<T>::deallocate(typename FastAllocator<T>::pointer release, size_t n) {
    if (n > 1 || sizeof(T) > memorySize_) {
        std::allocator<T>().deallocate(release, n);
    } else {
        alloc_.deallocate(static_cast<void*>(release));
    }
}

template<typename T>
void FastAllocator<T>::construct(typename FastAllocator<T>::pointer p, const T& make) const {
    new (p) T(make);
}

template<typename T>
void FastAllocator<T>::destroy(typename FastAllocator<T> :: pointer p) const {
    p->~T();
}

template<typename T>
FastAllocator<T> FastAllocator<T>::select_on_container_copy_construction(const FastAllocator<T>& A) {
    FastAllocator<T> answer;
    return answer;
}

template<typename T, typename Allocator = std::allocator<T> >
class List
{
    template<typename U, typename AllocatorOut>
    friend std :: ostream& operator<<(std::ostream& out, const List<U, AllocatorOut>& A);

public:
    class Node
    {
    public:
        T data_;
        Node* next_ = nullptr;
        Node* prev_ = nullptr;

        Node(const T& value) : data_(value) {}
        Node(T&& value) : data_(std::move(value)) {}
        template<typename... Args>
        Node(Args&& ...args) : data_(std::forward<Args>(args)...) {}
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
        const T* operator->() {return &(currentNode->data_);}

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
        T* operator->() {return &(currentNode->data_);}
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
    size_t size() const;
    void push_back(const T& value);
    void push_front(const T& value);
    void push_front(T&& value);
    void pop_back();
    void pop_front();
    void insert_before(Node* ptr, const T& value);
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


};

template<typename T, typename Allocator>
List<T, Allocator>::List(List<T, Allocator>&& A) {
    head_ = A.head_;
    tail_ = A.tail_;
    nodeAlloc_ = std::move(A.nodeAlloc_);
    alloc_ = std::move(A.alloc_);
    notBuild = A.notBuild;
    size_ = A.size_;
    A.head_ = nullptr;
    A.tail_ = nullptr;
    A.notBuild = true;
    A.size_ = 0;

}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::Node* List<T, Allocator>::requireNode(Args&& ...args) {
    Node* ptr = std::allocator_traits<additionalAllocator>::allocate(nodeAlloc_, 1);
    std::allocator_traits<additionalAllocator>::construct(nodeAlloc_, ptr, std::forward<Args>(args)...);
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
    if (!pos) {
        if (notBuild) {
            makeHeadTail(std::forward<Args>(args)...);
            notBuild = false;
        }
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
    return currentNode->data_;
}

template<typename T, typename Allocator>
const T& List<T, Allocator>::const_iterator::operator*() {
    return currentNode->data_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::removeNode(Node* ptr) {
    std::allocator_traits<additionalAllocator>::destroy(nodeAlloc_, ptr);
    std::allocator_traits<additionalAllocator>::deallocate(nodeAlloc_, ptr, 1);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& alloc) : alloc_(std::allocator_traits<Allocator>::select_on_container_copy_construction(alloc)) {}

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
void List<T, Allocator>::push_back(const T& value) {
    if (notBuild) {
        makeHeadTail(value);
        notBuild = false;
    }

    Node* ptr = requireNode(value);
    Node* P = tail_->prev_;
    P->setSubsequent(ptr);
    ptr->setSubsequent(tail_);
    ++size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {

    if (notBuild) {
        makeHeadTail(value);
        notBuild = false;
    }

    Node* ptr = requireNode(value);
    Node* P = head_->next_;
    ptr->setSubsequent(P);
    head_->setSubsequent(ptr);
    ++size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(T&& value) {

    if (notBuild) {
        makeHeadTail(std::move(value));
        notBuild = false;
    }

    Node* ptr = requireNode(std::move(value));
    Node* P = head_->next_;
    ptr->setSubsequent(P);
    head_->setSubsequent(ptr);
    ++size_;
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
    Node* newNode = requireNode(value);
    Node* afterPtr = ptr->next_;
    ptr->setSubsequent(newNode);
    newNode->setSubsequent(afterPtr);
    ++size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::insert_after(Node* ptr, T&& value) {
    Node* newNode = requireNode(std::move(value));
    Node* afterPtr = ptr->next_;
    ptr->setSubsequent(newNode);
    newNode->setSubsequent(afterPtr);
    ++size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::insert_before(Node* ptr, const T& value) {
    Node* newNode = requireNode(value);
    Node* beforePtr = ptr->prev_;
    beforePtr->setSubsequent(newNode);
    newNode->setSubsequent(ptr);
    ++size_;
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
        push_back(it->data_);
    }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List<T, Allocator>& A) : List(A.alloc_) {
    concatenate(A);
}

template<typename T, typename Allocator>
List<T, Allocator>::~List() {
    clear();
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
        out << it->data_ << ' ';
    }
    return out;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::Node* List<T, Allocator>::next(Node* ptr) {
    return ptr->next_;
}
