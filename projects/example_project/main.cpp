// projects/example_project/main.cpp
#include <iostream>
#include <vector>
#include <chrono>

// برنامج بسيط لحساب مجموع الأرقام
int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // حساب بطيء متعمد (سيتم تحسينه بواسطة DARV)
    long long sum = 0;
    for (int i = 0; i < 10000000; i++) {
        sum += i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "المجموع: " << sum << std::endl;
    std::cout << "الوقت المستغرق: " << elapsed.count() << " ms" << std::endl;
    
    return 0;
}