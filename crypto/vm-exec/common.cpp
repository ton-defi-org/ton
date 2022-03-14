//
// Created by Narek Abovyan on 23.02.2022.
//

#include "common.h"

td::Ref<vm::Tuple> prepare_vm_c7(ton::UnixTime now) {
  //    auto now = static_cast<unsigned int>(td::Time::now());
  td::BitArray<256> rand_seed;
  td::RefInt256 rand_seed_int{true};
  td::Random::secure_bytes(rand_seed.as_slice());
  if (!rand_seed_int.unique_write().import_bits(rand_seed.cbits(), 256, false)) {
    return {};
  }
  auto balance = block::CurrencyCollection{1000, {}}.as_vm_tuple();
  auto tuple = vm::make_tuple_ref(td::make_refint(0x076ef1ea),  // [ magic:0x076ef1ea
                                  td::make_refint(0),           //   actions:Integer
                                  td::make_refint(0),           //   msgs_sent:Integer
                                  td::make_refint(now),         //   unixtime:Integer
                                  td::make_refint(now),         //   block_lt:Integer
                                  td::make_refint(now),         //   trans_lt:Integer
                                  std::move(rand_seed_int),     //   rand_seed:Integer
                                  balance                       //   balance_remaining:[Integer (Maybe Cell)]
                                  // my_addr,                      //  myself:MsgAddressInt
                                  // vm::StackEntry()              //  global_config:(Maybe Cell) ] = SmartContractInfo;
  );
  LOG(DEBUG) << "SmartContractInfo initialized with " << vm::StackEntry(tuple).to_string();
  return vm::make_tuple_ref(std::move(tuple));
}

td::Result<vm::StackEntry> json_to_stack_entry(td::JsonObject &obj) {
  TRY_RESULT(type, get_json_object_string_field(obj, "type"));

  if (type == "int") {
    TRY_RESULT(data, get_json_object_string_field(obj, "value"));
    return vm::StackEntry(td::dec_string_to_int256(data));
  }
  if (type == "cell") {
    TRY_RESULT(data, get_json_object_string_field(obj, "value"));
    TRY_RESULT(data_bytes, td::base64_decode(data));
    TRY_RESULT(boc, vm::std_boc_deserialize(data_bytes));
    TRY_RESULT(data_cell, boc->load_cell());
    return vm::StackEntry(data_cell.data_cell);
  }
  if (type == "cell_slice") {
    TRY_RESULT(data, get_json_object_string_field(obj, "value"));
    TRY_RESULT(data_bytes, td::base64_decode(data));
    TRY_RESULT(boc, vm::std_boc_deserialize(data_bytes));
    TRY_RESULT(data_cell, boc->load_cell());
    return vm::StackEntry(td::make_ref<vm::CellSlice>(vm::CellSlice(data_cell)));
  }
  if (type == "null") {
    return vm::StackEntry();
  }
  if (type == "tuple") {
    TRY_RESULT(data, td::get_json_object_field(obj, "value", td::JsonValue::Type::Array, false));
    auto &data_arr = data.get_array();

    std::vector<vm::StackEntry> tuple_components;

    for (auto &x : data_arr) {
      TRY_RESULT(item, json_to_stack_entry(x.get_object()));
      tuple_components.push_back(item);
    }

    return vm::StackEntry(tuple_components);
  }

  return vm::StackEntry();
}

td::Result<td::Ref<vm::Stack>> json_to_stack(td::JsonArray &array) {
  auto stack = td::make_ref<vm::Stack>();

  for (auto &x : array) {
    auto &obj = x.get_object();
    TRY_RESULT(entry, json_to_stack_entry(obj));
    stack.write().push(entry);
  }

  return stack;
}

td::Result<std::string> stack_entry_to_json(vm::StackEntry se) {
  if (se.is_int()) {
    return R"({ "type": "int", "value": ")" + td::dec_string(se.as_int()) + R"("})";
  }
  if (se.is_cell()) {
    TRY_RESULT(boc, vm::std_boc_serialize(se.as_cell()));
    auto value = td::base64_encode(boc);
    return R"({ "type": "cell", "value": ")" + value + R"("})";
  }
  if (se.type() == vm::StackEntry::Type::t_slice) {
    vm::CellBuilder b;
    b.append_cellslice(se.as_slice());
    TRY_RESULT(boc, vm::std_boc_serialize(b.finalize()));
    auto value = td::base64_encode(boc);
    return R"({ "type": "cell_slice", "value": ")" + value + R"("})";
  }
  if (se.is_null()) {
    return R"({ "type": "null" })";
  }
  if (se.is_tuple()) {
    std::string res = R"({ "type": "tuple", "value": [)";

    auto tuple = se.as_tuple();

    for (auto &x : *tuple) {
      TRY_RESULT(item, stack_entry_to_json(x));
      res.append(item);
      res.append(",");
    }
    res = res.substr(0, res.length() - 1);
    res += "] }";

    return res;
  }

  //
  // Non supported by TVM
  //
  if (se.type() == vm::StackEntry::Type::t_builder) {
    return R"({ "type": "t_builder" })";
  }
  if (se.type() == vm::StackEntry::Type::t_vmcont) {
    return R"({ "type": "t_vmcont" })";
  }
  if (se.type() == vm::StackEntry::Type::t_string) {
    return R"({ "type": "string", value: ")" + se.as_string() + R"("})";
  }
  if (se.type() == vm::StackEntry::Type::t_bytes) {
    return R"({ "type": "bytes", value: ")" + td::base64_encode(se.as_bytes()) + R"("})";
  }
  if (se.type() == vm::StackEntry::Type::t_bitstring) {
    return R"({ "type": "bitstring", value: ")" + td::base64_encode(se.as_bytes()) + R"("})";
  }

  return R"({ "type": "unknown" })";
}

td::Result<std::string> stack2json(vm::Ref<vm::Stack> stack) {
  if (stack->is_empty()) {
    return "[]";
  }
  std::string out = "[";
  for (const auto &x : stack->as_span()) {
    TRY_RESULT(item, stack_entry_to_json(x));
    out.append(item);
    out.append(",");
  }
  out = out.substr(0, out.length() - 1);
  out.append("]");
  return out;
}

td::Result<std::string> run_vm(td::Ref<vm::Cell> code, td::Ref<vm::Cell> data, td::JsonArray &stack_array,
                               td::JsonObject &c7_register, int function_selector,
                               std::function<std::string()> getLogs) {
  vm::init_op_cp0(true);

  TRY_RESULT(stack, json_to_stack(stack_array));
  TRY_RESULT(c7_data, json_to_stack_entry(c7_register));

  td::int64 method_id = function_selector;
  stack.write().push_smallint(method_id);

  long long gas_limit = vm::GasLimits::infty;
  vm::GasLimits gas{gas_limit};
  LOG(DEBUG) << "creating VM";
  vm::VmState vm{std::move(code), std::move(stack), gas, 1, data, vm::VmLog()};

  vm.set_c7(c7_data.as_tuple());

  LOG(DEBUG) << "SmartContractInfo initialized with " << vm::StackEntry(vm.get_c7()).to_string();
  LOG(INFO) << "starting VM to run method `" << function_selector << "` of smart contract";

  int exit_code;
  try {
    exit_code = ~vm.run();
  } catch (vm::VmVirtError &err) {
    LOG(ERROR) << "virtualization error while running VM to locally compute runSmcMethod result: " << err.get_msg();
  } catch (vm::VmError &err) {
    LOG(ERROR) << "error while running VM to locally compute runSmcMethod result: " << err.get_msg();
  }

  if (exit_code != 0) {
    auto serialized_logs = td::base64_encode(getLogs());

    std::string result;
    result += "{";
    result += R"("ok": true,)";
    result += R"("exit_code":)" + std::to_string(exit_code) + ",";
    result += R"("logs": ")" + serialized_logs + R"(")";
    result += "}";

    return result;
  }

  stack = vm.get_stack_ref();

  std::string result;

  auto committed_state = vm.get_committed_state();

  TRY_RESULT(data_cell_boc, vm::std_boc_serialize(committed_state.c4));
  auto serialized_data_cell = td::base64_encode(data_cell_boc);
  TRY_RESULT(action_list_boc, vm::std_boc_serialize(committed_state.c5));
  auto serialized_action_list_cell = td::base64_encode(action_list_boc);
  auto serialized_logs = td::base64_encode(getLogs());
  TRY_RESULT(stack_str,  stack2json(stack));

  result += "{";
  result += R"("ok": true,)";
  result += R"("exit_code":)" + std::to_string(exit_code) + ",";
  result += R"("stack":)" + stack_str + ",";
  result += R"("data_cell": ")" + serialized_data_cell + R"(",)";
  result += R"("gas_consumed": )" + std::to_string(vm.gas_consumed()) + R"(,)";
  result += R"("action_list_cell": ")" + serialized_action_list_cell + R"(",)";
  result += R"("logs": ")" + serialized_logs + R"(")";
  result += "}";

  return result;
}

td::Result<std::string> vm_exec_from_config(std::string config, std::function<std::string()> getLogs) {
  TRY_RESULT(input_json, td::json_decode(config))
  auto &obj = input_json.get_object();

  TRY_RESULT(code, td::get_json_object_string_field(obj, "code", false));
  TRY_RESULT(data, td::get_json_object_string_field(obj, "data", false));
  TRY_RESULT(function_selector, td::get_json_object_int_field(obj, "function_selector", false));
  TRY_RESULT(initial_stack, td::get_json_object_field(obj, "init_stack", td::JsonValue::Type::Array, false));
  TRY_RESULT(c7_register, td::get_json_object_field(obj, "c7_register", td::JsonValue::Type::Object, false));

  auto &initial_stack_array = initial_stack.get_array();

  TRY_RESULT(data_bytes, td::base64_decode(data));
  TRY_RESULT(boc, vm::std_boc_deserialize(data_bytes));
  TRY_RESULT(data_cell, boc->load_cell());

  TRY_RESULT(decoded_code, td::base64_decode(code));
  TRY_RESULT(code_boc, vm::std_boc_deserialize(decoded_code));
  TRY_RESULT(code_cell, code_boc->load_cell());

  return run_vm(code_cell.data_cell, data_cell.data_cell, initial_stack_array, c7_register.get_object(),
                function_selector, getLogs);
}