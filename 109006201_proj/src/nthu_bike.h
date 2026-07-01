#ifndef NTHU_BIKE_H
#define NTHU_BIKE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

// ---- domain limits (per project spec) ----
#define MIN_TIME 0
#define MAX_TIME 1440          // a day has 1440 minutes
#define MAX_DIST 1000000000    // "infinity" for shortest paths

// ---------------------------------------------------------------------------
// A hand-rolled binary min-heap priority queue (the data-structures exercise).
//
// Ordering: smaller priority1 first; ties broken by smaller priority2. Callers
// encode "highest rental price" by enqueuing with priority1 = -price, so the
// most expensive bike sits at the top, and priority2 = bike id for the
// smallest-id tie-break.
//
// Backed by a manually grown array. Copy is deleted and move is provided so the
// queue can live inside STL containers (e.g. unordered_map) without the shallow
// pointer copy / double-free the original code was vulnerable to.
// ---------------------------------------------------------------------------
template <typename T>
struct Node {
    T element;
    float priority1;
    int priority2;
};

template <typename T>
class MinHeap {
private:
    int heap_size = 0;
    int capacity;
    Node<T>* heap;

    void grow() {
        if (heap_size < capacity) return;
        capacity *= 2;
        Node<T>* nh = new Node<T>[capacity + 1];
        for (int i = 1; i <= heap_size; i++) nh[i] = heap[i];
        delete[] heap;
        heap = nh;
    }

    void heapify_up(int child) {
        if (child == 1) return;
        int parent = child / 2;
        if (heap[parent].priority1 > heap[child].priority1 ||
            (heap[parent].priority1 == heap[child].priority1 &&
             heap[parent].priority2 > heap[child].priority2)) {
            Node<T> tmp = heap[child];
            heap[child] = heap[parent];
            heap[parent] = tmp;
            heapify_up(parent);
        }
    }

    void heapify_down(int parent) {
        int smallest = parent;
        int l = 2 * parent, r = l + 1;
        if (l <= heap_size &&
            (heap[l].priority1 < heap[smallest].priority1 ||
             (heap[l].priority1 == heap[smallest].priority1 &&
              heap[l].priority2 < heap[smallest].priority2)))
            smallest = l;
        if (r <= heap_size &&
            (heap[r].priority1 < heap[smallest].priority1 ||
             (heap[r].priority1 == heap[smallest].priority1 &&
              heap[r].priority2 < heap[smallest].priority2)))
            smallest = r;
        if (smallest != parent) {
            Node<T> tmp = heap[smallest];
            heap[smallest] = heap[parent];
            heap[parent] = tmp;
            heapify_down(smallest);
        }
    }

public:
    MinHeap() : capacity(1) { heap = new Node<T>[capacity + 1]; }

    ~MinHeap() { delete[] heap; }

    // no copying (raw owning pointer)
    MinHeap(const MinHeap&) = delete;
    MinHeap& operator=(const MinHeap&) = delete;

    // moving is fine and lets us store queues in STL maps
    MinHeap(MinHeap&& o) noexcept
        : heap_size(o.heap_size), capacity(o.capacity), heap(o.heap) {
        o.heap = nullptr;
        o.heap_size = 0;
        o.capacity = 0;
    }
    MinHeap& operator=(MinHeap&& o) noexcept {
        if (this != &o) {
            delete[] heap;
            heap_size = o.heap_size;
            capacity = o.capacity;
            heap = o.heap;
            o.heap = nullptr;
            o.heap_size = 0;
            o.capacity = 0;
        }
        return *this;
    }

    void enqueue(const T& element, float priority1, int priority2 = 0) {
        grow();
        heap[++heap_size] = {element, priority1, priority2};
        heapify_up(heap_size);
    }

    bool peek(T& element) const {
        if (heap_size <= 0) return false;
        element = heap[1].element;
        return true;
    }

    bool dequeue(T& element) {
        if (!peek(element)) return false;
        heap[1] = heap[heap_size--];
        heapify_down(1);
        return true;
    }

    bool isEmpty() const { return heap_size == 0; }
    int size() const { return heap_size; }
};

// ---- domain records ----
struct Bike {
    int type;
    int id;
    int station;          // dense station index (see common.cpp)
    float rental_price = 0.0f;
    int rental_count = 0;
    int arrival_time = 0; // used by the basic time-driven simulation
};

struct User {
    int id;
    std::string bikes_type; // raw "B0,B1" accept list
    int start_time;
    int end_time;
    int start_point;        // dense station index
    int end_point;          // dense station index
};

// ---- shared simulation parameters (defined in common.cpp) ----
extern float depreciation_discount_price;
extern int rental_count_limit;

// ---- shared I/O + graph helpers (common.cpp) ----
bool import_files(const std::string& selected_case);
void shortest_paths();
int dist(int a, int b);                 // shortest travel time between dense stations
int station_count();
int original_station_id(int dense);     // dense index -> id printed as "S<id>"
const std::vector<User>& all_users();   // sorted by (start_time, id)
const std::vector<Bike>& all_bikes();    // initial bike inventory (station = dense idx)

void open_outputs(const std::string& selected_case);
void close_outputs();
extern std::ofstream f_station;
extern std::ofstream f_users;
extern std::ofstream f_transfer;

// ---- the two tool versions ----
void run_basic(const std::string& selected_case);
void run_advanced(const std::string& selected_case);

#endif // NTHU_BIKE_H
