#include "vm/cp0.h"
#include "td/utils/logging.h"
#include "StringLog.h"
#include "common.h"
#include <thread>

using namespace std::literals::string_literals;

auto memLog = new StringLog();

extern "C" char *vm_exec(int len, char *_data) {
  // Init logging
  td::log_interface = memLog;
  SET_VERBOSITY_LEVEL(verbosity_DEBUG);
  memLog->clear();

  std::string config(_data, len);

  auto res = vm_exec_from_config(config, []() -> std::string { return memLog->get_string(); });

  if (res.is_error()) {
    std::string result;
    result += "{";
    result +=  R"("ok": false,)";
    result +=  R"("error": ")" + res.move_as_error().message().str() + R"(")";
    result += "}";

    return strdup(result.c_str());
  }
  return strdup(res.move_as_ok().c_str());
}

void threadFunction() {
  std::string config(R"({"function_selector":92067,"init_stack":[{"type":"int","value":"976"}],"code":"te6cckECFgEAArYAART/APSkE/S88sgLAQIBYgsCAgEgBgMCASAFBAAjuiF+1E0PpA0z/U1NT6QDBsUYACu5Bb7UTQ+kDTP9TU1PpAMF8D0NQwWIAgEgCgcCASAJCAAztPR9qJofSBpn+pqan0gGAgSr4L4AjgA+ALAANbXa/aiaH0gaZ/qamp9IBgKr4LoaYfph/0gGEABDuLXTHtRND6QNM/1NTU+kAwEDVfBdDUMNBxyMsHAc8WzMmAICzREMAgEgDg0Ag0UFTwBHAh8AUC0IIYLmpzb27IWM8WyyfJyMzJyFAGzxYVzCPPFlADzxbJd4AYyMsFUATPFlj6AhLLaxLMzMlx+wCAIBIBAPABs+QB0yMsCEsoHy//J0IAAtAHIyz/4KM8WyXAgyMsBE/QA9ADLAMmAE99EGOASK3wAOhpgYC42Eit8H0gGHaiaH0gaZ/qamp9IBgD6Y/pn8EINJ6cqCkYXUcVmBioK6+C6AlBCFRlgFa4QAhkZYKoAueLEn0BCmW1CeWP5Z+A54tkwCB9gHBBCB61w+OpGF1xgRiou2OC+XDIkGAA8YEQYAFxgRqCQVFBMSAEbAA44YBPpAMEQ1yFAGzxYUyz8SzMzMAc8Wye1U4F8GhA/y8AC2MHAG1DCOO4BA9JZvpSCOLQmkIIEA+r6T8sGP3oEBkyGgUyi78vQC+gDU+kAwI1E4QTMu8AYmupMFpAXeB5JsIeKz5jA1EDVVEshQBs8WFMs/EszMzAHPFsntVAB6MDIzA9M/UxK78uGSUxK6AfoA1PpAMFQkY1R1OfAGAY4YAqRFREMTyFAGzxYUyz8SzMzMAc8Wye1Ukl8G4gCMbGJSJMcF8uGTAtQwIPsE0O0e7VNwghBcuzqOVQJtgEBwgBDIywVQB88WUAX6AhXLahLLH8s/Im6zlFjPFwGRMuIByQH7ABUQGFw=","data":"te6cckECGwEABDMAA5WAADDwSC1LkiUbxtayDrG9enuDC8tUOxHJClfvnQPK7VUgAAAAAAABOJADrLq+X6jKZNHAScgghh0h1iog3StK71zn8dcmrOj8jPYZAgEASwAFAGSAADDwSC1LkiUbxtayDrG9enuDC8tUOxHJClfvnQPK7VUwART/APSkE/S88sgLAwIBYg0EAgFYBgUAEbohfwAhAoXwiAIBIAgHAA20Xr4ATZAwAgFICgkADa/n+AEvgkACASAMCwAQqUbwAhA4XwgADqp98AIYXwgCAs4RDgIBIBAPAD0yFAEzxZYzxbLBwHPFskEyMs/UAPPFgHPFszMye1UgAFs7UTQ0z/6QCDXScIAjhL6QNTUMND6QPpA0wf6QDB/VXDgMHBtbW1tJBBXEFZtgAvNDIhxwCSXwPg0NMD+kAw8AIIs45SN18ENDVSIscF8uGVAvpA1PpA+kAwcMjLA8nQJBB4Q4jwA3AgghBQdW5rbXFwgBDIywVQB88WUAX6AhXLahLLH8s/Im6zlFjPFwGRMuIByQH7AOAK0x8hwADjAApxsJJfDOAJ0z+BgSBP6CEF/MPRRSsLqOjTo6EFkQSBA3XjIQNFjgMzuCEC/LJqJSkLqOPV8EMjMzcIIQi3cXNQPIy/9QBM8WFIBAcIAQyMsFUAfPFlAF+gIVy2oSyx/LPyJus5RYzxcBkTLiAckB+wDgghD1MNhnUpC64wJQll8GghA9a4fHE7rjAl8EFhUUEwAIhA/y8ACIUgLHBfLhkwHUMCD7BNDtHu1TcIIQXLs6jlUCbYBAcIAQyMsFUAfPFlAF+gIVy2oSyx/LPyJus5RYzxcBkTLiAckB+wAAlDj6QDAkUWQGEEUQNBAjSarwA3CCEK9XPYQEyMv/UAPPFl4hcXCAEMjLBVAHzxZQBfoCFctqEssfyz8ibrOUWM8XAZEy4gHJAfsAAfYwUUbHBfLhkQH6QPpA0gAx+gCCEAQsHYAcoSGhIMIA8uGSIY48ghAFE42RyCrPFlANzxZxJQROE1RI8HCAEMjLBVAHzxZQBfoCFctqEssfyz8ibrOUWM8XAZEy4gHJAfsAkjsw4iDXCwHDAJMwMjfjDRBWBEUVA3AB8AMXAGSCENUydttKAwRtcXCAEMjLBVAHzxZQBfoCFctqEssfyz8ibrOUWM8XAZEy4gHJAfsABgCyU5XHBfLhkdMfAYIQc2VsbLqORVR3Y1R3ZXEs8ANwghAFE42RIcgpzxYkzxYnVTBxcIAQyMsFUAfPFlAF+gIVy2oSyx/LPyJus5RYzxcBkTLiAckB+wDyAN4BABoAumh0dHBzOi8vY2xvdWRmbGFyZS1pcGZzLmNvbS9pcGZzL2JhZnliZWloem13N3NvYm50cGF0cTd6eWFrcHB0eTJjeDU0azNzaTV5aTI3Nm5hcXhseTducmI3amplL2PV5Lk=","c7_register":{"type":"tuple","value":[{"type":"tuple","value":[{"type":"int","value":"124711402"},{"type":"int","value":"0"},{"type":"int","value":"0"},{"type":"int","value":"1647206289"},{"type":"int","value":"1647206289"},{"type":"int","value":"1647206289"},{"type":"int","value":"405448960944048076912533454474163447403335447360159618829009007772079104040"},{"type":"tuple","value":[{"type":"int","value":"1000"},{"type":"null"}]},{"type":"cell_slice","value":"te6cckEBAQEAJAAAQ4AeGKizerOKjVdMf0ygusGYf8afGUSTg8X6pfID2a0dhjBlV57O"},{"type":"cell","value":"te6cckEBAQEAAgAAAEysuc0="}]}]}})");

//  for (int i = 0; i < 1000; ++i) {
    char* res = vm_exec(strlen(config.c_str()), (char*)config.c_str());
    printf("%s\n", res);
//  }
}

int main(int argc, char *argv[]) {



  for (int i = 0; i < 1000; ++i) {
    std::thread thr(threadFunction);
    thr.join();
  }


//  td::log_interface = memLog;
//  SET_VERBOSITY_LEVEL(verbosity_DEBUG);
//  memLog->clear();
//
//  auto res = vm_exec_from_config(config, []() -> std::string { return memLog->get_string(); });
//  printf("%s", res.c_str());

  return 0;
}
//int main(int argc, char *argv[]) {
//    SET_VERBOSITY_LEVEL(verbosity_FATAL);
//
//    td::OptionParser p;
//
//    p.set_description("TVM JSON Executor");
//    p.add_option('h', "help", "prints_help", [&]() {
//        char b[10240];
//        td::StringBuilder sb(td::MutableSlice{b, 10000});
//        sb << p;
//        std::cout << sb.as_cslice().c_str();
//        std::exit(2);
//    });
//    p.add_option('c', "config", "path to config", [&](td::Slice fname) { execute(fname); });
//    p.run(argc, argv).ensure();
//
//    printf("%d\n", argc);
//    printf("hello\n");
//    return 0;
//}
