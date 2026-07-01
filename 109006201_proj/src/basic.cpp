// Basic version: no free transfers, no waiting.
//
// Rules (spec 2, basic): process users by (start_time, user_id); a request is
// served only from a bike physically available at the start station at
// start_time; among candidates pick the highest current price, ties broken by
// smallest bike id; reject otherwise. Fee = floor(price * distance), and the
// arrival time (start_time + distance) must be strictly before end_time.
#include "nthu_bike.h"

#include <cmath>
#include <sstream>

using namespace std;

namespace {
const long long TYPE_STRIDE = 1LL << 20;
inline long long key(int station, int type) { return (long long)station * TYPE_STRIDE + type; }

unordered_map<long long, MinHeap<Bike>> station_bikes;  // available, keyed (-price, id)
unordered_map<long long, MinHeap<Bike>> retired_bikes;  // retired, keyed by id
MinHeap<Bike> not_yet_bikes;                            // in transit, keyed by arrival_time

void place_bike(const Bike& b) {
    if (b.rental_count >= rental_count_limit)
        retired_bikes[key(b.station, b.type)].enqueue(b, (float)b.id, b.id);
    else
        station_bikes[key(b.station, b.type)].enqueue(b, -b.rental_price, b.id);
}

// move every bike that finishes its ride exactly at `time` back into inventory
void handle_bikes(int time) {
    Bike b;
    while (not_yet_bikes.peek(b) && b.arrival_time == time) {
        not_yet_bikes.dequeue(b);
        place_bike(b);
    }
}

void handle_user(const User& u) {
    int distance = dist(u.start_point, u.end_point);
    int window = u.end_time - u.start_time;

    // reject if the bike cannot arrive strictly before End_Time
    if (distance >= window) {
        f_users << "U" << u.id << " 0 0 0 0 0\n";
        return;
    }

    // pick the best available bike across all acceptable types
    bool found = false;
    Bike best;
    stringstream ss(u.bikes_type);
    string tok;
    while (getline(ss, tok, ',')) {
        int type = stoi(tok.substr(1));
        auto it = station_bikes.find(key(u.start_point, type));
        if (it == station_bikes.end()) continue;
        Bike cand;
        if (!it->second.peek(cand)) continue;
        if (!found || cand.rental_price > best.rental_price ||
            (cand.rental_price == best.rental_price && cand.id < best.id)) {
            best = cand;
            found = true;
        }
    }

    if (!found) {
        f_users << "U" << u.id << " 0 0 0 0 0\n";
        return;
    }

    // rent the chosen bike
    Bike b;
    station_bikes[key(u.start_point, best.type)].dequeue(b);
    int fee = (int)floor((double)distance * b.rental_price);
    int arrival = u.start_time + distance;

    f_users << "U" << u.id << " 1 " << b.id << ' ' << u.start_time << ' '
            << arrival << ' ' << fee << '\n';
    f_transfer << b.id << " S" << original_station_id(u.start_point) << " S"
               << original_station_id(u.end_point) << ' ' << u.start_time << ' '
               << arrival << " U" << u.id << '\n';

    b.station = u.end_point;
    b.rental_count++;
    b.rental_price -= depreciation_discount_price;
    b.arrival_time = arrival;
    not_yet_bikes.enqueue(b, (float)arrival, b.id);
}

// Final inventory: stations ascending, bikes within a station by ascending id.
void print_station_status() {
    for (int s = 0; s < station_count(); s++) {
        MinHeap<Bike> ordered;  // keyed by bike id
        for (int pass = 0; pass < 2; pass++) {
            auto& m = (pass == 0) ? station_bikes : retired_bikes;
            for (auto& kv : m) {
                if (kv.first / TYPE_STRIDE != s) continue;
                Bike b;
                while (kv.second.dequeue(b)) ordered.enqueue(b, (float)b.id, b.id);
            }
        }
        Bike b;
        while (ordered.dequeue(b))
            f_station << "S" << original_station_id(s) << ' ' << b.id << " B"
                      << b.type << ' ' << b.rental_price << ' ' << b.rental_count
                      << '\n';
    }
}
}  // namespace

void run_basic(const string& selected_case) {
    (void)selected_case;
    station_bikes.clear();
    retired_bikes.clear();

    for (const Bike& b : all_bikes()) place_bike(b);

    const vector<User>& users = all_users();
    size_t next = 0;
    for (int t = MIN_TIME; t <= MAX_TIME; t++) {
        handle_bikes(t);
        while (next < users.size() && users[next].start_time == t)
            handle_user(users[next++]);
    }

    print_station_status();
}
