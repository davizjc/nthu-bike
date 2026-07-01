// Advanced version: maximize revenue.
//
// Extra powers over basic (spec 2, advanced):
//   * free bike transfer service (company moves an idle bike along the shortest
//     path, rider = -1, no rental-count increase),
//   * users may WAIT for a bike, as long as they still arrive before End_Time,
//   * we may pick any acceptable bike or reject a request.
//
// Strategy (greedy, per request in the mandated (start_time, user_id) order):
// for each user consider every non-retired bike of an acceptable type anywhere
// in the network. A bike idle at station st, free at time `avail`, can be
// brought to the user's start point by transfer_time = dist(st, start); the user
// then departs at max(start_time, avail + transfer_time) and must arrive before
// End_Time. Among all feasible bikes we take the one with the highest fee
// (floor(price * distance)), so every request earns at least what basic would
// have earned from a local bike -- hence total revenue dominates basic.
#include "nthu_bike.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

namespace {
struct BState {
    int id;
    int type;
    int station;   // dense
    float price;
    int count;
    int avail;     // time the bike is next free (at `station`)
    bool retired;
};

vector<int> parse_types(const string& csv) {
    vector<int> out;
    stringstream ss(csv);
    string tok;
    while (getline(ss, tok, ',')) out.push_back(stoi(tok.substr(1)));
    return out;
}
}  // namespace

void run_advanced(const string& selected_case) {
    (void)selected_case;

    // per-bike mutable timeline, plus type -> bike indices for fast lookup
    vector<BState> bikes;
    bikes.reserve(all_bikes().size());
    unordered_map<int, vector<int>> by_type;
    for (const Bike& b : all_bikes()) {
        BState s{b.id, b.type, b.station, b.rental_price, b.rental_count, 0,
                 b.rental_count >= rental_count_limit};
        by_type[b.type].push_back((int)bikes.size());
        bikes.push_back(s);
    }

    for (const User& u : all_users()) {
        int travel = dist(u.start_point, u.end_point);
        // unreachable, zero-length, or window too small to ever finish in time
        if (travel <= 0 || travel >= MAX_DIST || travel >= (u.end_time - u.start_time)) {
            f_users << "U" << u.id << " 0 0 0 0 0\n";
            continue;
        }

        int best = -1, best_fee = 0, best_pickup = 0, best_tt = 0;
        for (int type : parse_types(u.bikes_type)) {
            auto it = by_type.find(type);
            if (it == by_type.end()) continue;
            for (int idx : it->second) {
                BState& b = bikes[idx];
                if (b.retired) continue;
                int tt = dist(b.station, u.start_point);   // 0 if already local
                if (tt >= MAX_DIST) continue;
                int ready = b.avail + tt;                  // bike reaches start point
                int pickup = max(u.start_time, ready);
                int arrival = pickup + travel;
                if (arrival >= u.end_time) continue;       // must arrive strictly before
                int fee = (int)floor((double)travel * b.price);
                if (fee <= 0) continue;                    // no revenue to be had
                // prefer higher fee, then a local bike (smaller transfer), then id
                if (best == -1 || fee > best_fee ||
                    (fee == best_fee && tt < best_tt) ||
                    (fee == best_fee && tt == best_tt && b.id < bikes[best].id)) {
                    best = idx; best_fee = fee; best_pickup = pickup; best_tt = tt;
                }
            }
        }

        if (best == -1) {
            f_users << "U" << u.id << " 0 0 0 0 0\n";
            continue;
        }

        BState& b = bikes[best];

        // free transfer to reposition the bike, if it is not already local
        if (b.station != u.start_point) {
            int t_start = b.avail;
            int t_end = b.avail + best_tt;
            f_transfer << b.id << " S" << original_station_id(b.station) << " S"
                       << original_station_id(u.start_point) << ' ' << t_start
                       << ' ' << t_end << " -1\n";
        }

        int arrival = best_pickup + travel;
        f_users << "U" << u.id << " 1 " << b.id << ' ' << best_pickup << ' '
                << arrival << ' ' << best_fee << '\n';
        f_transfer << b.id << " S" << original_station_id(u.start_point) << " S"
                   << original_station_id(u.end_point) << ' ' << best_pickup << ' '
                   << arrival << " U" << u.id << '\n';

        b.station = u.end_point;
        b.avail = arrival;
        b.count++;
        b.price -= depreciation_discount_price;
        if (b.count >= rental_count_limit) b.retired = true;
    }

    // final inventory: stations ascending, bikes within a station by ascending id
    vector<int> order(bikes.size());
    for (size_t i = 0; i < bikes.size(); i++) order[i] = (int)i;
    sort(order.begin(), order.end(), [&](int a, int c) {
        if (bikes[a].station != bikes[c].station)
            return original_station_id(bikes[a].station) < original_station_id(bikes[c].station);
        return bikes[a].id < bikes[c].id;
    });
    for (int i : order) {
        BState& b = bikes[i];
        f_station << "S" << original_station_id(b.station) << ' ' << b.id << " B"
                  << b.type << ' ' << b.price << ' ' << b.count << '\n';
    }
}
