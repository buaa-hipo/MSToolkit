#include <atomic>
#include <iostream>
int main()
{
    int a = 0;
    std::atomic_ref<int> ref(a);
    ref.store(1);
    std::cout << ref.load() << std::endl;
    std::cout << a << std::endl;
    std::cout << ref.is_lock_free() << std::endl;
    return 0;
}