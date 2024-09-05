#include <iostream>
#include <future>
#include <any>
#include <thread>

struct test{
    std::promise<test> p;
    int val;
};

void setVal(test &t){
    t.val = 1;
    t.p.set_value(t);
}

int main(){
    test t;
    std::promise<test> p;
    std::future<test> f = p.get_future();
    t.p = std::move(p);
    std::thread t1(setVal, std::ref(t));
    t1.join();
    std::cout << f.get().val << std::endl;
    return 0;
}