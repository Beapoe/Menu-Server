#include <future>
#include <any>
#include <queue>
#include <iostream>
#include <memory>
#include <sstream>

struct test_struct{
    std::promise<std::any> p;
    std::any val;

    test_struct()=default;
    test_struct(test_struct&& other){
        p = std::move(other.p);
    }

    test_struct& operator=(test_struct&& other){
        p = std::move(other.p);
        return *this;
    }

    test_struct& operator=(const test_struct& other)=delete;
    test_struct(const test_struct& other)=delete;
};

class test{
    public:
        test()=default;
        void exe(test_struct s);
};


int main(){
    test_struct s;
    std::future<std::any> f = s.p.get_future();
    std::unique_ptr<test> ptr = std::make_unique<test>();
    ptr->exe(std::move(s));
    std::stringstream ss;
    ss<<std::any_cast<int>(f.get());
    std::cout<<ss.str()<<std::endl;
    return 0;
}

void test::exe(test_struct s)
{
    s.p.set_value(1);
}
