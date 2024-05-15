#include <systemc>
using namespace sc_core;

SC_MODULE(Hello_world) {
    SC_CTOR(Hello_world) {
        SC_METHOD(say);
    }

    void say(void) {
        std::cout << "Hello World" << std::endl;
    }
};

int sc_main(int, char* []) {
    Hello_world hello_World("helloworld");
    sc_start();
    return 0;
}