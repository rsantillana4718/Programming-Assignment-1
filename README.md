1) Why a Circular Linked List fits the relay:
The relay needs fair round-robin turns. With a circular list, the “end” naturally wraps to the “start,” so moving to the next robot is just a **rotation**.
In my list I keep both `head_` and `tail_` and maintain the invariant:

if (!empty)  tail_->next == head_

This lets me do the important operations in constant time:

| Operation     | What I do (from `LinkedList.h`)                          | Cost |
| ------------- | -------------------------------------------------------- | ---- |
| `append(x)`   | link `tail_->next = new`, `new->next = head_`, move tail | O(1) |
| `rotate()`    | `head_ = head_->next; tail_ = tail_->next;`              | O(1) |
| `pop_front()` | move `head_` forward and repair `tail_->next = head_`    | O(1) |

I considered `std::vector` and a non-circular singly list. `vector` would require shifting to pop the front (O(n)), and a non-circular list needs extra checks to jump from last back to first. The circular structure matched the game’s “pass the baton” idea with the simplest code paths.

2) How the split works
Goal: split one circular list into two circular lists where:

first list has `ceil(n/2)` nodes
second list has `floor(n/2)` nodes
the original becomes empty (ownership moves)

I used the classic fast/slow pointers:
1. `slow = head_`, `fast = head_`.
2. While `fast->next != head_` and `fast->next->next != head_`:
   `slow = slow->next; fast = fast->next->next;`
3. If `fast->next->next == head_`, move `fast` one more step so it lands at the true tail for even sizes.
4. Now the cut is between `slow` and `slow->next`:

   * `head1 = head_`, `tail1 = slow`
   * `head2 = slow->next`, `tail2 = fast`
5. Close both rings (`tail1->next = head1; tail2->next = head2`) and assign sizes:
   `n1 = (sz_ + 1)/2`, `n2 = sz_ - n1`.
6. Set the source list’s `head_`/`tail_` to `nullptr` and `sz_ = 0`.

3) Example outputs:
Formatting is from `main.cpp` (`operator<<` prints `Robot(Name, Battery=X)`).

A. Append + Display
Input:
1
Alpha
5
1
Beta
3
5
0

Output excerpt:
=== Robot Relay Ring ===
Robots: 0
Score: 0
Quantum: 1
...
Choose: 1
Robot name: Alpha
Battery: 5
=== Robot Relay Ring ===
Robots: 1
Score: 2
Quantum: 1
...
Choose: 1
Robot name: Beta
Battery: 3
=== Robot Relay Ring ===
Robots: 2
Score: 4
Quantum: 1
...
Choose: 5
[Robot(Alpha, Battery=5) -> Robot(Beta, Battery=3)] (circular)
...
Choose: 0
Goodbye!

B. One turn with removal:
Start with:
[Robot(A, Battery=1) -> Robot(B, Battery=2)] (circular)

Run one turn (`2`):
Tick: A battery 1 -> 0
Removed: Robot(A, Battery=0) (returned to dock)

Display (`5`):
[Robot(B, Battery=2)] (circular)

C. Split into two (odd count = 5):

Before:
[A -> B -> C -> D -> E] (circular)

Choose `6`:

Ring A:
[A -> B -> C] (circular)
Ring B:
[D -> E] (circular)


D. Simple stats:
With three robots having batteries 5, 3, 2:

Choose: 8
Robots: 3
Avg battery: 3.33333
Ticks: 4
Score: 9

4) Debugging notes

Broken circle after edits:
Early on I forgot to relink `tail_->next = head_` after popping the head. The list printed fine for a couple nodes then crashed. I added `_checkInvariant()` (asserts) at the end of mutators while developing; that caught it quickly.

Display infinite loop:
My first `display()` used `while (cur != head_)` which fails on an empty list and could loop forever if the circle is broken. I switched to a strict `for (i = 0; i < sz_; ++i)` walk.

Single-node edge cases:
`pop_front()` and `rotate()` behave differently for size 1.

`pop_front()` sets `head_ = tail_ = nullptr; sz_ = 0`.
`rotate()` is a no-op when `head_ == tail_`.

Removal + rotation order
In the round-robin turn (runOneTurn), if the battery hits 0 I do not rotate after `pop_front()`. The next robot is already at the head. Rotating here skipped a robot and made the turns unfair.

Split midpoint:
I originally forgot to move `fast` one extra step for even lengths, which left the second list off by one. After adding:

if (fast->next->next == head_) fast = fast->next;

both halves matched `ceil(n/2)` / `floor(n/2)`.

Stats traversal:
To compute averages without mutating the ring, I added `forEach` in `LinkedList<T>` that walks exactly `size()` nodes. That kept the `statsReport` code simple and side-effect free.
