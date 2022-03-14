#include <terminal.h>
#include <tonlib/keys/bip39.h>
#include "tonlib/keys/Mnemonic.h"

using namespace tonlib;

int main(int argc, char *argv[]) {
  auto out = td::TerminalIO::out();

  auto bip_words = Mnemonic::normalize_and_split(td::SecureString(bip39_english()));

  for (int i = 0; i <= 1000; i++ ){
    std::vector<td::SecureString> words;
    for (int i = 0; i < 24; i++) {
      words.push_back(bip_words[std::rand() % bip_words.size()].copy());
    }

    td::SecureString pass;
    auto m = Mnemonic::create(std::move(words), pass.copy()).move_as_ok();

//    Mnemonic::Options options;
//    auto c = Mnemonic::create_new(std::move(options)).move_as_ok();
    td::Timer timer;
    auto private_key = m.to_private_key().as_octet_string();
    LOG(INFO) << "Mnemonic generation debug stats: " << timer;
    LOG(INFO) << private_key;
//    out << "\n";
  }

}