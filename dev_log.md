## 1) `display()` didn’t show nodes (sometimes hung)

**What I tried**
First version of `LinkedList::display()` used a loop that stopped when the cursor returned to `head_`.

```cpp
// first attempt
while (cur != head_) {                 // starts at head_, so condition is false
    std::cout << cur->data << " -> ";
    cur = cur->next;
}
```

**What broke**

* For non-empty rings it printed nothing.
* After some edits it could loop forever if the circle was broken.

**Why it happened**
In a circular list `cur` starts at `head_`, so `cur != head_` is false on the first check. Also, relying on a pointer sentinel is risky if the invariant is off.

**How I fixed it**
Walk **exactly** `size()` nodes. This cannot hang and prints every element.

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
# after: Add Alpha(5), then Beta(3)
Choose: 5
[Robot(Alpha, Battery=5) -> Robot(Beta, Battery=3)] (circular)

# empty ring
Choose: 5
[] (empty)
```

## 2) Removal + rotate skipped a robot

**What I tried**
In `runOneTurn` (main.cpp) I removed the head when its battery hit 0 **and then rotated**.

```cpp
if (cur.battery <= 0) {
    ring.pop_front();
    ring.rotate();            // <- I did this at first
}
```

**What broke**
The robot after the removed one never got a turn. The schedule was unfair.

**Why it happened**
After a head removal, the **next robot is already the new head**. Rotating again jumps over it.

**How I fixed it**
Do **not** rotate after a removal; only rotate when the robot survives.

```cpp
if (cur.battery <= 0) {
    ring.pop_front();         // no rotate here
    return 1 + 3;
} else {
    ring.rotate();            // rotate on survive
    return 1;
}
```

**Proof that it works**

```
Start: [A(bat=1) -> B(bat=2)]
Choose: 2
Tick: A battery 1 -> 0
Removed: Robot(A, Battery=0) (returned to dock)
Choose: 5
[Robot(B, Battery=2)] (circular)     # B correctly at head
```

## 3) `splitIntoTwo` wrong sizes on even n

**What I tried**
Used fast/slow pointers but forgot the extra step for even lengths.

```cpp
while (fast->next != head_ && fast->next->next != head_) {
    slow = slow->next;
    fast = fast->next->next;
}
// missing: adjust fast for even size
```

**What broke**
For 4 nodes I got a 1+3 split instead of 2+2.

**Why it happened**
On even `n`, `fast` stops one node before the real tail. The cut lands too early.

**How I fixed it**
Advance `fast` one more step when `fast->next->next == head_`, then form the two circles and transfer ownership.

```cpp
if (fast->next->next == head_) fast = fast->next;   // even length adjust
Node* head1 = head_;  Node* tail1 = slow;
Node* head2 = slow->next; Node* tail2 = fast;

tail1->next = head1;
tail2->next = head2;

std::size_t n1 = (sz_ + 1) / 2;
std::size_t n2 = sz_ - n1;
```

**Proof that it works**

```
# n = 5
Choose: 6
Ring A:
[A -> B -> C] (circular)
Ring B:
[D -> E] (circular)

# n = 4
Choose: 6
Ring A:
[A -> B] (circular)
Ring B:
[C -> D] (circular)
```
