#include <iostream>
#include <string>
#include <limits>
#include "linkedlist.h"


struct Robot {
    int id;
    std::string name;
    int battery;   // remaining
    int drain;     // per turn (Quantum)
    bool paused;

    Robot(int id_, std::string n, int b, int d = 1, bool p = false)
        : id(id_), name(std::move(n)), battery(b), drain(d), paused(p) {}
};


std::ostream& operator<<(std::ostream& os, const Robot& r) {
    os << "Robot(" << r.name << ", Battery=" << r.battery << ")";
    return os;
}

static void printMenu(const LinkedList<Robot>& ring, long long score, int quantum) {
    std::cout << "\n=== Robot Relay Ring ===\n";
    std::cout << "Robots: "  << ring.size() << "\n";
    std::cout << "Score: "   << score       << "\n";
    std::cout << "Quantum: " << quantum     << "\n";
    std::cout <<
        "1) Add robot\n"
        "2) Run 1 turn\n"
        "3) Run N turns\n"
        "4) Pause/Resume robot\n"
        "5) Display ring\n"
        "6) Split ring into two\n"
        "7) Merge rings\n"
        "8) Stats report\n"
        "0) Exit\n"
        "Choose: ";
}

// Add robot: prompt for name + battery; drain = quantum by default
static void addRobot(LinkedList<Robot>& ring, int& nextId, long long& score, int quantum) {
    std::string name; int bat;
    std::cout << "Robot name: "; std::cin >> name;
    std::cout << "Battery: ";    std::cin >> bat;
    ring.append(Robot(nextId++, name, bat, quantum));
    score += 2; // scoring per spec
}

// Display the ring
static void displayRing(const LinkedList<Robot>& ring) {
    ring.display();
}

// One turn of round-robin (Quantum battery drain per turn; paused => skip)
static int runOneTurn(LinkedList<Robot>& ring) {
    if (ring.size() == 0) { std::cout << "No robots.\n"; return 0; }

    Robot& cur = ring.front();             // head robot
    if (cur.paused) {                      // paused → skip and rotate
        std::cout << "Skipped (paused): " << cur << "\n";
        ring.rotate();
        return 1;                          // still a processed tick
    }

    int before = cur.battery;
    cur.battery -= cur.drain;
    std::cout << "Tick: " << cur.name << " battery " << before
              << " -> " << cur.battery << "\n";

    if (cur.battery <= 0) {                // removal occurs at head
        std::cout << "Removed: " << cur << " (returned to dock)\n";
        ring.pop_front();                   // do NOT rotate after removal
        return 1 + 3;                       // +1 tick, +3 removal bonus
    } else {
        ring.rotate();                      // alive → move to next
        return 1;
    }
}

// Pause/Resume by id (linear search in the circle)
static void togglePauseById(LinkedList<Robot>& ring, int id) {
    if (ring.size() == 0) { std::cout << "No robots.\n"; return; }
    bool found = false;
    for (std::size_t i = 0; i < ring.size(); ++i) {
        if (ring.front().id == id) {
            ring.front().paused = !ring.front().paused;
            std::cout << (ring.front().paused ? "Paused: " : "Resumed: ")
                      << ring.front() << "\n";
            found = true;
            break;
        }
        ring.rotate();
    }
    if (!found) std::cout << "Not found.\n";
}

// Stats report: count + average battery (simple baseline)
static void statsReport(const LinkedList<Robot>& ring, long long ticks, long long score) {
    int count = static_cast<int>(ring.size());
    long long sum = 0;
    ring.forEach([&](const Robot& r){ sum += r.battery; });
    double avg = (count ? static_cast<double>(sum) / count : 0.0);

    std::cout << "Robots: " << count << "\n";
    std::cout << "Avg battery: " << avg << "\n";
    std::cout << "Ticks: " << ticks << "\n";
    std::cout << "Score: " << score << "\n";
}

int main() {
    LinkedList<Robot> ring;        // main working ring
    LinkedList<Robot> a, b;        // for split/merge menu options
    int nextId = 1;
    long long score = 0;
    long long ticks = 0;
    const int quantum = 1;         // matches sample; used to init Robot::drain

    for (;;) {
        printMenu(ring, score, quantum);
        int choice;
        if (!(std::cin >> choice)) break;

        if (choice == 0) {
            std::cout << "Goodbye!\n";
            break;
        }
        else if (choice == 1) {
            addRobot(ring, nextId, score, quantum);
        }
        else if (choice == 2) {
            score += runOneTurn(ring);
            ++ticks;
        }
        else if (choice == 3) {
            int n; std::cout << "Turns: "; std::cin >> n;
            for (int i = 0; i < n && ring.size() > 0; ++i) {
                score += runOneTurn(ring);
                ++ticks;
            }
        }
        else if (choice == 4) {
            int id; std::cout << "Robot id: "; std::cin >> id;
            togglePauseById(ring, id);
        }
        else if (choice == 5) {
            displayRing(ring);
        }
        else if (choice == 6) {
            // Split current ring into A and B; ring becomes empty
            a.clear(); b.clear();
            ring.splitIntoTwo(a, b);
            std::cout << "Ring A:\n"; a.display();
            std::cout << "Ring B:\n"; b.display();
        }
        else if (choice == 7) {
            // Merge B into A, then A back into main ring for convenience
            a.mergeWith(b);        // after this, b is empty
            ring.mergeWith(a);     // ring takes ownership; a becomes empty
            std::cout << "Merged. Current ring:\n"; ring.display();
        }
        else if (choice == 8) {
            statsReport(ring, ticks, score);
        }
        else {
            std::cout << "Unknown option.\n";
            // flush bad input if any
            if (!std::cin.good()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
    }
    return 0;
}
