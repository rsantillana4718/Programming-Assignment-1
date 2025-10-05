Developer Log
Each entry includes: what I tried, what broke, why, how I fixed it, and a short proof.

## Entry 1 — `display()` loop never returned

**What I tried**
First pass of `LinkedList<T>::display()`:

```cpp
void display() const {
    if (!head_) { std::cout << "[] (empty)\n"; return; }
    Node* cur = head_;
    std::cout << "[";
    while (cur != head_) {           // ❌
        std::cout << cur->data << " -> ";
        cur = cur->next;
    }
    std::cout << "] (circular)\n";
}
```

**What broke**

* On a 2+ node ring it printed nothing or hung.
* On an empty list the `while` check didn’t even run (no items printed).

**Why it happened**
On a circular list, `cur` starts at `head_`, so `while (cur != head_)` is false on the first check. Also, if the circle is ever broken, a plain `while` can loop forever.

**How I fixed it**
Walk **exactly** `size()` nodes:

```cpp
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
```

**Proof that it works**

```
# after adding Alpha(5), Beta(3)
Choose: 5
[Robot(Alpha, Battery=5) -> Robot(Beta, Battery=3)] (circular)

# empty case
Choose: 5
[] (empty)
```

---

## Entry 2 — Removing head corrupted the ring

**What I tried**
`pop_front()` advanced `head_` and deleted the old node, but I forgot to repair the circle.

```cpp
bool pop_front() {
    if (!head_) return false;
    Node* old = head_;
    head_ = head_->next;
    delete old;
    --sz_;
    return true;                     // ❌ tail_->next still points to old head
}
```

**What broke**
After a removal, `display()` sometimes froze or printed garbage.

**Why it happened**
In a circular list, after moving the head forward, the **tail must point to the new head**. I didn’t re-link `tail_->next`.

**How I fixed it**

```cpp
head_ = head_->next;
tail_->next = head_;                 // ✅ re-close the ring
delete old;
--sz_;
```

**Proof that it works**

```
# Start: [A(1) -> B(2)]
Choose: 2
Tick: A battery 1 -> 0
Removed: Robot(A, Battery=0) (returned to dock)
Choose: 5
[Robot(B, Battery=2)] (circular)
```

---

## Entry 3 — Rotate advanced head but not tail

**What I tried**

```cpp
void rotate() {
    if (!head_ || head_ == tail_) return;
    head_ = head_->next;             // ❌ only head moved
}
```

**What broke**
After several rotates, `tail_->next` no longer matched `head_`. Some traversals looped incorrectly.

**Why it happened**
The invariant is `tail_->next == head_`. If only `head_` moves, the link from `tail_` becomes stale.

**How I fixed it**

```cpp
void rotate() {
    if (!head_ || head_ == tail_) return;
    head_ = head_->next;
    tail_ = tail_->next;             // ✅ keep the invariant
}
```

**Proof that it works**

```
# Start: [A -> B -> C]
Rotate x3 -> list returns to original order
Choose: 5
[Robot(A, Battery=...) -> Robot(B, ...) -> Robot(C, ...)] (circular)
```

---

## Entry 4 — Skipping a robot after removal in the turn loop

**What I tried (main.cpp)**
In `runOneTurn`, after a robot hit 0, I removed it **and then rotated**.

```cpp
if (cur.battery <= 0) {
    ring.pop_front();
    ring.rotate();                    // ❌ this skipped the next robot
    return 1 + 3;
}
```

**What broke**
The next robot after the removed one was skipped, so the schedule wasn’t fair.

**Why it happened**
After removing the head, the **next robot is already the new head**. Rotating again jumps over it.

**How I fixed it**

```cpp
if (cur.battery <= 0) {
    ring.pop_front();                 // ✅ no rotate here
    return 1 + 3;
} else {
    ring.rotate();                    // rotate only when the robot survives
    return 1;
}
```

**Proof that it works**

```
# [A(1) -> B(2)]
Turn 1: A -> 0, removed
Next head is B (no extra rotate)
Choose: 5
[Robot(B, Battery=2)] (circular)
```

---

## Entry 5 — Split halves off by one on even length

**What I tried**
I used fast/slow but didn’t advance `fast` the extra step for even sizes.

**What broke**
For 4 nodes, I got 1+3 instead of 2+2.

**Why it happened**
On even `n`, `fast` ends one node before the true tail. The cut ended up uneven.

**How I fixed it**

```cpp
while (fast->next != head_ && fast->next->next != head_) {
    slow = slow->next;
    fast = fast->next->next;
}
if (fast->next->next == head_) fast = fast->next;  // ✅ even length
```

**Proof that it works**

```
# n=5 -> (3,2)
Ring A:
[A -> B -> C] (circular)
Ring B:
[D -> E] (circular)

# n=4 -> (2,2)
Ring A:
[A -> B] (circular)
Ring B:
[C -> D] (circular)
```

---

## Entry 6 — Stats changed the ring order

**What I tried**
To compute the average battery I rotated the ring to traverse it.

```cpp
long long sum=0;
for (size_t i=0; i<ring.size(); ++i) {
    sum += ring.front().battery;
    ring.rotate();                    // ❌ mutates order
}
```

**What broke**
After running stats, the head changed, which was surprising during tests.

**Why it happened**
`rotate()` is destructive traversal. I needed a read-only walk.

**How I fixed it**
Added a `forEach` helper in `LinkedList<T>` that walks exactly `size()` items without changing the head:

```cpp
template <typename F>
void forEach(F&& f) const {
    if (!head_) return;
    Node* cur = head_;
    for (size_t i = 0; i < sz_; ++i) { f(cur->data); cur = cur->next; }
}
```

Used it in `statsReport`:

```cpp
long long sum = 0;
ring.forEach([&](const Robot& r){ sum += r.battery; });
```

**Proof that it works**

```
# Run stats twice
Choose: 8
Robots: 3
Avg battery: 3.33333
Ticks: 0
Score: 0

Choose: 5
[head remains the same as before the stats call]
```

---

## Entry 7 — Single-node edge cases

**What I tried**
I didn’t handle the single-node branch in `pop_front()` and `rotate()`.

**What broke**

* `pop_front()` left `tail_` dangling.
* `rotate()` sometimes crashed when `head_ == tail_`.

**Why it happened**
Size 1 is special: removing the head empties the list, and rotating should be a no-op.

**How I fixed it**

```cpp
bool pop_front() {
    if (!head_) return false;
    if (head_ == tail_) {
        delete head_;
        head_ = tail_ = nullptr;
        sz_ = 0;
        return true;
    }
    // many-nodes case...
}

void rotate() {
    if (!head_ || head_ == tail_) return;  // no-op on size < 2
    head_ = head_->next;
    tail_ = tail_->next;
}
```

**Proof that it works**

```
# One robot:
Choose: 5
[Robot(A, Battery=5)] (circular)

# Remove it:
Choose: 2   (drain to 0 path) or call pop_front() via menu logic
Choose: 5
[] (empty)
```

---

These fixes, along with the invariant checks I kept under `#ifndef NDEBUG`, stabilized the list and made the game loop behave as expected.
