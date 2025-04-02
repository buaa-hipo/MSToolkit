#include <iostream>
#include <unordered_map>
#include <memory>

using namespace std;

class test {
    public:
    unordered_map<int, unique_ptr<int>> map;

    test() {
        map[0] = unique_ptr<int>(new int(1));
    }

    class sub_test {
    public:
        unique_ptr<int> *val;
        sub_test(unique_ptr<int> *i, int j): val(i) {}
    };

    sub_test get(int index) {
        return sub_test(&map.at(index), 0);
    }
};

int main() {
    test t;
    for (int i = 0; i < 3; ++i) {
        try {
            auto _ = t.get(i);
        } catch (const std::exception& e) {
            cerr << e.what() << endl;
            auto _ = t.get(i + 1);
            continue;
        }
    }
    return 0;
}