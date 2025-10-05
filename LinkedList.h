// linkedlist.h
#pragma once
#include <cstddef>
#include <iostream>
#include <utility>
#include <cassert>

template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        explicit Node(const T& v) : data(v), next(nullptr) {}
        explicit Node(T&& v) : data(std::move(v)), next(nullptr) {}
    };

    Node* head_ = nullptr;          // nullptr when empty
    Node* tail_ = nullptr;          // nullptr when empty; else tail_->next == head_
    std::size_t sz_ = 0;            // cached size

    void make_single(Node* n) {
        n->next = n;
        head_ = tail_ = n;
        sz_ = 1;
    }

#ifndef NDEBUG
    void _checkInvariant() const {
        if (!head_) { assert(tail_ == nullptr && sz_ == 0); return; }
        assert(tail_ && tail_->next == head_ && "broken circular invariant");
        const Node* cur = head_;
        for (std::size_t i = 0; i < sz_; ++i) cur = cur->next;
        assert(cur == head_ && "walk sz_ steps must wrap to head");
    }
#endif

public:
    LinkedList() = default;
    ~LinkedList() { clear(); }

    bool empty() const { return head_ == nullptr; }
    std::size_t size() const { return sz_; }

    // ---- core operations ----

    // O(1) append using tail_ pointer; preserves tail_->next == head_
    void append(const T& value) {
        Node* n = new Node(value);
        if (!head_) { make_single(n); return; }
        n->next = head_;
        tail_->next = n;
        tail_ = n;
        ++sz_;
#ifndef NDEBUG
        _checkInvariant();
#endif
    }
    void append(T&& value) {
        Node* n = new Node(std::move(value));
        if (!head_) { make_single(n); return; }
        n->next = head_;
        tail_->next = n;
        tail_ = n;
        ++sz_;
#ifndef NDEBUG
        _checkInvariant();
#endif
    }

    // Remove head safely; empty/single/many handled
    bool pop_front() {
        if (!head_) return false;                  // empty
        Node* old = head_;
        if (head_ == tail_) {                      // single node
            delete old;
            head_ = tail_ = nullptr;
            sz_ = 0;
#ifndef NDEBUG
            _checkInvariant();
#endif
            return true;
        }
        head_ = head_->next;                       // advance head
        tail_->next = head_;                       // re-close ring
        delete old;
        --sz_;
#ifndef NDEBUG
        _checkInvariant();
#endif
        return true;
    }

    // Rotate one step: advance both head and tail if size >= 2
    void rotate() {
        if (!head_ || head_ == tail_) return;
        head_ = head_->next;
        tail_ = tail_->next;
#ifndef NDEBUG
        _checkInvariant();
#endif
    }

    // Access head element (assumes non-empty)
    T& front() { return head_->data; }
    const T& front() const { return head_->data; }

    // Print exactly sz_ nodes to avoid infinite loop
    void display() const {
        if (!head_) { std::cout << "[] (empty)\n"; return; }
        std::cout << "[";
        Node* cur = head_;
        for (std::size_t i = 0; i < sz_; ++i) {
            std::cout << cur->data;
            if (i + 1 < sz_) std::cout << " -> ";
            cur = cur->next;
        }
        std::cout << "] (circular)\n";
    }

    // Iterate helper (const) – handy for stats/reporting
    template <typename F>
    void forEach(F&& f) const {
        if (!head_) return;
        Node* cur = head_;
        for (std::size_t i = 0; i < sz_; ++i) {
            f(cur->data);
            cur = cur->next;
        }
    }

    // Clear all nodes
    void clear() {
        if (!head_) return;
        Node* cur = head_;
        for (std::size_t i = 0; i < sz_; ++i) {
            Node* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
        head_ = tail_ = nullptr;
        sz_ = 0;
    }

    // ---- split & (optional) merge ----

    // Split into two circular lists.
    // first gets ceil(n/2), second gets floor(n/2). This list becomes empty.
    void splitIntoTwo(LinkedList<T>& first, LinkedList<T>& second) {
        first.clear(); second.clear();
        if (!head_) return;

        if (sz_ == 1) {
            first.head_ = head_; first.tail_ = tail_; first.sz_ = 1;
            first.tail_->next = first.head_;
            head_ = tail_ = nullptr; sz_ = 0;
            return;
        }

        Node* slow = head_;
        Node* fast = head_;
        while (fast->next != head_ && fast->next->next != head_) {
            slow = slow->next;
            fast = fast->next->next;
        }
        if (fast->next->next == head_) fast = fast->next; // even length → true tail

        Node* head1 = head_;       Node* tail1 = slow;
        Node* head2 = slow->next;  Node* tail2 = fast;

        tail1->next = head1;
        tail2->next = head2;

        std::size_t n1 = (sz_ + 1) / 2;
        std::size_t n2 = sz_ - n1;

        first.head_ = head1; first.tail_ = tail1; first.sz_ = n1;
        second.head_ = head2; second.tail_ = tail2; second.sz_ = n2;

        head_ = tail_ = nullptr; sz_ = 0;
    }

    // Optional: splice another circle after this one in O(1); 'other' becomes empty.
    void mergeWith(LinkedList<T>& other) {
        if (other.empty()) return;
        if (empty()) {
            head_ = other.head_; tail_ = other.tail_; sz_ = other.sz_;
            other.head_ = other.tail_ = nullptr; other.sz_ = 0;
            return;
        }
        Node* aHead = head_;
        Node* bHead = other.head_;
        tail_->next = bHead;
        other.tail_->next = aHead;
        tail_ = other.tail_;
        sz_ += other.sz_;
        other.head_ = other.tail_ = nullptr; other.sz_ = 0;
#ifndef NDEBUG
        _checkInvariant();
#endif
    }
};
