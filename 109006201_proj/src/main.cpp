// Entry point: parse args, import the case, run the requested version.
//
// Usage: ./bin/main <testcase> <version>
//   <testcase>  name of a folder under ./testcases (e.g. case1)
//   <version>   "basic" or "advanced" (also accepts "advance")
#include "nthu_bike.h"

#include <chrono>
#include <ctime>
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <testcase> <version:basic|advanced>\n";
        return 1;
    }

    string selected_case = argv[1];
    string version = argv[2];

    cout << "You have set " << selected_case << " as your testcase.\n";
    cout << "running " << version << " version\n";
    cout << "-------------------------------------------\n";

    auto start = chrono::system_clock::now();

    if (!import_files(selected_case)) {
        cerr << "Couldn't import files for " << selected_case << "\n";
        return 1;
    }
    shortest_paths();
    open_outputs(selected_case);

    if (version == "advanced" || version == "advance")
        run_advanced(selected_case);
    else
        run_basic(selected_case);

    close_outputs();

    auto end = chrono::system_clock::now();
    chrono::duration<double> elapsed = end - start;
    time_t end_time = chrono::system_clock::to_time_t(end);
    cout << "-------------------------------------------\n";
    cout << "finished computation at " << ctime(&end_time)
         << "elapsed time: " << elapsed.count() << "s\n";
    return 0;
}
