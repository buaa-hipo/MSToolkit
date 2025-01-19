#include <iostream>
#include <thread>
#include <chrono>

int main() {
    int countdown = 30;  // 初始倒计时时间，单位为秒
    while (countdown >= 0) {
        std::cout << countdown << " countdowm remaining..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        countdown--;
    }
    std::cout << "Time's up!" << std::endl;
    return 0;
}