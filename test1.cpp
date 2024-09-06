#include <future>
#include <any>
#include <queue>

struct test{
    std::promise<std::any> p;
    std::any val;

    test()=default;
    test(test&& other){
        p = std::move(other.p);
    }

    test& operator=(test&& other){
        p = std::move(other.p);
        return *this;
    }

    test& operator=(const test& other)=delete;
    test(const test& other)=delete;
};

int getVal(std::queue<test> tests){
    test test = std::move(tests.front());
    test.val = 5;
    tests.pop();
    return std::any_cast<int>(test.val);
}


int main(){
    test t;
    std::queue<test> tests;
    int val = getVal(tests);
    return 0;
}