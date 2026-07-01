// Shared parsing, graph, and output plumbing for both tool versions.
#include "nthu_bike.h"

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;

// ---- shared simulation parameters ----
float depreciation_discount_price = 0.0f;
int rental_count_limit = 0;

// ---- output streams ----
ofstream f_station;
ofstream f_users;
ofstream f_transfer;

// ---- internal shared state ----
namespace {
int N = 0;                              // number of distinct stations
vector<int> dense_to_orig;              // dense index -> original station id
unordered_map<int, int> orig_to_dense;  // original station id -> dense index
vector<vector<int>> D;                  // NxN shortest-path distance matrix
vector<User> users_vec;                 // sorted by (start_time, id)
vector<Bike> bikes_vec;                 // initial inventory (station = dense idx)

// pull the integer out of a token like "S12", "B3", "U7"
int extract_int(const string& tok) { return stoi(tok.substr(1)); }

int dense_of(int orig) {
    auto it = orig_to_dense.find(orig);
    return it == orig_to_dense.end() ? -1 : it->second;
}
}  // namespace

int station_count() { return N; }
int original_station_id(int dense) { return dense_to_orig[dense]; }
int dist(int a, int b) { return D[a][b]; }
const vector<User>& all_users() { return users_vec; }
const vector<Bike>& all_bikes() { return bikes_vec; }

// First pass: collect every station id that appears anywhere and assign dense
// indices in ascending order of the original id (so station output is naturally
// sorted). Station ids are arbitrary integers, so we must not index by them.
static bool collect_stations(const string& dir) {
    vector<int> ids;
    auto add = [&](int id) { ids.push_back(id); };

    // map.txt: "S<a> S<b> <d>"
    {
        ifstream fin(dir + "/map.txt");
        if (!fin.is_open()) { cerr << "Error importing map.txt\n"; return false; }
        string a, b; int d;
        while (fin >> a >> b >> d) { add(extract_int(a)); add(extract_int(b)); }
    }
    // bike.txt: "B<type> <id> S<station> <price> <count>"
    {
        ifstream fin(dir + "/bike.txt");
        if (!fin.is_open()) { cerr << "Error importing bike.txt\n"; return false; }
        string type, station; int id, count; float price;
        while (fin >> type >> id >> station >> price >> count) add(extract_int(station));
    }
    // user.txt: "U<id> <types> <st> <et> S<start> S<end>"
    {
        ifstream fin(dir + "/user.txt");
        if (!fin.is_open()) { cerr << "Error importing user.txt\n"; return false; }
        string u, types, s1, s2; int st, et;
        while (fin >> u >> types >> st >> et >> s1 >> s2) {
            add(extract_int(s1)); add(extract_int(s2));
        }
    }

    sort(ids.begin(), ids.end());
    ids.erase(unique(ids.begin(), ids.end()), ids.end());
    N = (int)ids.size();
    dense_to_orig = ids;
    for (int i = 0; i < N; i++) orig_to_dense[ids[i]] = i;
    return true;
}

static bool input_map(const string& file) {
    D.assign(N, vector<int>(N, MAX_DIST));
    for (int i = 0; i < N; i++) D[i][i] = 0;

    ifstream fin(file);
    if (!fin.is_open()) return false;
    string a, b; int d;
    while (fin >> a >> b >> d) {
        int i = dense_of(extract_int(a));
        int j = dense_of(extract_int(b));
        if (i < 0 || j < 0) continue;
        if (d < D[i][j]) { D[i][j] = d; D[j][i] = d; }  // keep the shortest parallel edge
    }
    return true;
}

static bool input_bike_info(const string& file) {
    ifstream fin(file);
    if (!fin.is_open()) return false;
    fin >> depreciation_discount_price >> rental_count_limit;
    string b; float price;
    while (fin >> b >> price) { /* per-type initial price: informational only */ }
    return true;
}

static bool input_users(const string& file) {
    ifstream fin(file);
    if (!fin.is_open()) return false;
    string u, types, s1, s2; int st, et;
    while (fin >> u >> types >> st >> et >> s1 >> s2) {
        User usr;
        usr.id = extract_int(u);
        usr.bikes_type = types;
        usr.start_time = st;
        usr.end_time = et;
        usr.start_point = dense_of(extract_int(s1));
        usr.end_point = dense_of(extract_int(s2));
        users_vec.push_back(usr);
    }
    // spec: process by ascending Start_Time, then ascending User_ID
    sort(users_vec.begin(), users_vec.end(), [](const User& x, const User& y) {
        if (x.start_time != y.start_time) return x.start_time < y.start_time;
        return x.id < y.id;
    });
    return true;
}

static bool input_bikes(const string& file) {
    ifstream fin(file);
    if (!fin.is_open()) return false;
    string type, station; int id, count; float price;
    while (fin >> type >> id >> station >> price >> count) {
        Bike b;
        b.type = extract_int(type);
        b.id = id;
        b.station = dense_of(extract_int(station));
        b.rental_price = price;
        b.rental_count = count;
        b.arrival_time = 0;
        bikes_vec.push_back(b);
    }
    return true;
}

bool import_files(const string& selected_case) {
    string dir = "./testcases/" + selected_case;
    if (!collect_stations(dir)) return false;
    if (!input_map(dir + "/map.txt")) { cerr << "Error importing map.txt\n"; return false; }
    if (!input_bike_info(dir + "/bike_info.txt")) { cerr << "Error importing bike_info.txt\n"; return false; }
    if (!input_users(dir + "/user.txt")) { cerr << "Error importing user.txt\n"; return false; }
    if (!input_bikes(dir + "/bike.txt")) { cerr << "Error importing bike.txt\n"; return false; }
    return true;
}

// Floyd-Warshall over the compact station set (N is tiny: <= a few dozen).
void shortest_paths() {
    for (int k = 0; k < N; k++)
        for (int i = 0; i < N; i++) {
            if (D[i][k] >= MAX_DIST) continue;
            for (int j = 0; j < N; j++)
                if (D[k][j] < MAX_DIST && D[i][k] + D[k][j] < D[i][j])
                    D[i][j] = D[i][k] + D[k][j];
        }
}

void open_outputs(const string& selected_case) {
    string base = "./result/" + selected_case;
    f_station.open(base + "/station_status.txt");
    f_users.open(base + "/user_result.txt");
    f_transfer.open(base + "/transfer_log.txt");
}

void close_outputs() {
    f_station.close();
    f_users.close();
    f_transfer.close();
}
