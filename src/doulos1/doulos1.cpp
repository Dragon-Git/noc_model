#define SC_INCLUDE_DYNAMIC_PROCESS

#include <systemc>
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

class Initiator : public sc_module {
public:
  SC_HAS_PROCESS(Initiator);
  tlm_utils::simple_initiator_socket<Initiator> socket;
  Initiator(const sc_module_name& name) : socket("socket"){
    SC_THREAD(thread_process);
  }

  void thread_process() {
    tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload;
    sc_time delay = sc_time(10, SC_NS);



    for (int i = 32; i < 96 ; i += 4) {
      ///...
      tlm::tlm_command cmd = static_cast<tlm::tlm_command>(rand() % 2);
      if (cmd == tlm::TLM_WRITE_COMMAND) {
        data = 0xFF000000 | i;
      }

      trans->set_command(cmd);
      trans->set_address(i);
      trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
      trans->set_data_length(4);
      trans->set_streaming_width(4);
      trans->set_byte_enable_ptr(0);
      trans->set_dmi_allowed(false);
      trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

      socket->b_transport(*trans, delay);

      if (trans->is_response_error()) {
        SC_REPORT_ERROR("TLM 2.0", "Response error from b_transport");
      }
      //...
      cout << "trans = { " << (cmd ? 'W' : 'R') << ", " << hex << i
           << " } , data = " << hex << data << " at time " << sc_time_stamp()
           << " delay = " << delay << endl;
      wait(delay);
    }
  }
private: 
  int data;
};

class Memory : public sc_module {
public:
  SC_HAS_PROCESS(Memory);
  tlm_utils::simple_target_socket<Memory>       socket;

  enum {SIZE = 256};

  Memory(const sc_module_name& name) : socket("socket") {
    socket.register_b_transport(this, &Memory::b_transport);

    for (int i = 0; i < SIZE; i++) {
      mem[i] = 0xAA000000 | (rand() % SIZE);
    }

  }

  virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay){
    tlm::tlm_command  cmd = trans.get_command();
    sc_dt::uint64     adr = trans.get_address() / 4;
    unsigned char *   ptr = trans.get_data_ptr();
    unsigned int      len = trans.get_data_length();
    unsigned char *   byt = trans.get_byte_enable_ptr();
    unsigned int      wid = trans.get_streaming_width();

    if (adr >= sc_dt::uint64(SIZE) || byt != 0 || len > 4 || wid < len) {
      SC_REPORT_ERROR("TLM 2.0", "Target does not support given generic payload transaction");
    }

    if (cmd == tlm::TLM_READ_COMMAND) {
      memcpy(ptr, &mem[adr], len);
    } else if (cmd == tlm::TLM_WRITE_COMMAND) {
      memcpy(&mem[adr], ptr, len);
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  };

private:
  int mem[SIZE];
};

class Top : public sc_module {
public:
  SC_HAS_PROCESS(Top);
  Initiator *initiator;
  Memory    *memory;

  Top(const sc_module_name& name){
    initiator = new Initiator("initiator");
    memory    = new Memory("memory");

    initiator->socket.bind( memory->socket );
  }
};

int sc_main(int argc, char **argv){
  Top top("top");
  sc_start();
  return 0;
}