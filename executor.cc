#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>

enum class Condition { n, z, p };

using Address = int16_t;
using Value = int16_t;
const Value kDefaultValue = 0x7777;

class Memory {
  std::unordered_map<Address, Value> memory_;

 public:
  void clear() { memory_.clear(); }
  Value& operator[](const Address& address) noexcept {
    if (!memory_.count(address)) memory_[address] = kDefaultValue;
    return memory_[address];
  }
};

using Instruction =
    std::function<bool(Address&, Condition&, Value[8], Memory&)>;

class Decoder {
  inline static void SetCC(const Value& value, Condition& cond) noexcept {
    if (value < 0)
      cond = Condition::n;
    else if (value == 0)
      cond = Condition::z;
    else
      cond = Condition::p;
  }

  template <int n>
  inline static Value ExtractBit(const Value& v) noexcept {
    const int mask = (1 << n) - 1;
    return v & mask;
  }

  template <int l, int r>  // requires(l <= r)  // c++20
  inline static Value ExtractBit(const Value& v) noexcept {
    static_assert(l <= r, "r can't be less than r");
    return ExtractBit<r - l + 1>(v >> l);
  }

  template <int n>
  inline static Value SextExtractBit(const Value& v) noexcept {
    const int mask = (1 << n) - 1;
    const int neg_mask =
        (1 << (std::numeric_limits<Value>::digits + 1)) - 1 - mask;
    auto ret = v & mask;
    if (ret & (1 << (n - 1))) ret |= neg_mask;
    return ret;
  }

  template <int l, int r>  // requires(l <= r)  // c++20
  inline static Value SextExtractBit(const Value& v) noexcept {
    static_assert(l <= r, "r can't be less than r");
    return SextExtractBit<r - l + 1>(v >> l);
  }

 public:
  inline static Instruction Decode(const Value& inst) {
    switch (ExtractBit<12, 15>(inst)) {
      case 0b0000:  // BR
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          if ((ExtractBit<9, 9>(inst) && cond == Condition::p) ||
              (ExtractBit<10, 10>(inst) && cond == Condition::z) ||
              (ExtractBit<11, 11>(inst) && cond == Condition::n)) {
            addr += SextExtractBit<0, 8>(inst);
          }
          return true;
        };
      case 0b0001:  // ADD
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          if (!ExtractBit<5, 5>(inst)) {
            reg[ExtractBit<9, 11>(inst)] =
                reg[ExtractBit<6, 8>(inst)] + reg[ExtractBit<0, 2>(inst)];
          } else {
            reg[ExtractBit<9, 11>(inst)] =
                reg[ExtractBit<6, 8>(inst)] + SextExtractBit<0, 4>(inst);
          }
          SetCC(reg[ExtractBit<9, 11>(inst)], cond);
          return true;
        };
      case 0b0010:  // LD
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          reg[ExtractBit<9, 11>(inst)] = mem[addr + SextExtractBit<0, 8>(inst)];
          SetCC(reg[ExtractBit<9, 11>(inst)], cond);
          return true;
        };
      case 0b0011:  // ST
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          mem[addr + SextExtractBit<0, 8>(inst)] = reg[ExtractBit<9, 11>(inst)];
          return true;
        };
      case 0b0100:  // JSR
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          const auto tmp = addr + 1;
          if (!ExtractBit<11, 11>(inst)) {
            addr = reg[ExtractBit<6, 8>(inst)];
          } else {
            addr = addr + 1 + SextExtractBit<0, 10>(inst);
          }
          reg[7] = tmp;
          return true;
        };
      case 0b0101:  // AND
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          if (!ExtractBit<5, 5>(inst)) {
            reg[ExtractBit<9, 11>(inst)] =
                reg[ExtractBit<6, 8>(inst)] & reg[ExtractBit<0, 2>(inst)];
          } else {
            reg[ExtractBit<9, 11>(inst)] =
                reg[ExtractBit<6, 8>(inst)] & SextExtractBit<0, 4>(inst);
          }
          SetCC(reg[ExtractBit<9, 11>(inst)], cond);
          return true;
        };
      case 0b0110:  // LDR
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          reg[ExtractBit<9, 11>(inst)] =
              mem[reg[ExtractBit<6, 8>(inst)] + SextExtractBit<0, 5>(inst)];
          SetCC(reg[ExtractBit<9, 11>(inst)], cond);
          return true;
        };
      case 0b0111:  // STR
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          mem[reg[ExtractBit<6, 8>(inst)] + SextExtractBit<0, 5>(inst)] =
              reg[ExtractBit<9, 11>(inst)];
          return true;
        };
      case 0b1001:  // NOT
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          reg[ExtractBit<9, 11>(inst)] = ~reg[ExtractBit<6, 8>(inst)];
          SetCC(reg[ExtractBit<9, 11>(inst)], cond);
          return true;
        };
      case 0b1010:  // LDI
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          reg[ExtractBit<9, 11>(inst)] =
              mem[mem[addr + SextExtractBit<0, 8>(inst)]];
          SetCC(reg[ExtractBit<9, 11>(inst)], cond);
          return true;
        };
      case 0b1011:  // STI
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          mem[mem[addr + SextExtractBit<0, 8>(inst)]] =
              reg[ExtractBit<9, 11>(inst)];
          return true;
        };
      case 0b1100:  // JMP
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr = reg[ExtractBit<6, 8>(inst)];
          return true;
        };
      case 0b1110:  // LEA
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool {
          addr++;
          reg[ExtractBit<9, 11>(inst)] = addr + SextExtractBit<0, 8>(inst);
          return true;
        };
      case 0b1111:  // TRAP(ONLY supports HALT)
        return [inst](Address& addr, Condition& cond, Value reg[8],
                      Memory& mem) -> bool { return false; };
      default:
        throw std::runtime_error("Invalid Instruction");
    }
  }
};

class Machine {
  Address start_pc_;
  Value reg_[8];
  Memory memory_;
  Decoder decoder_;
  Condition condition_ = Condition::z;

 public:
  void Run() {
    std::fill(reg_, reg_ + sizeof(reg_) / sizeof(reg_[0]), kDefaultValue);
    Address pc = start_pc_;
    while (decoder_.Decode(memory_[pc])(pc, condition_, reg_, memory_)) {
#ifndef ONLINE_JUDGE
      std::cerr << std::hex << pc << std::endl;
#endif
    }
  }
  friend std::istream& operator>>(std::istream& is, Machine& other) {
    other.memory_.clear();
    std::string s;
    is >> s;
    Address now = other.start_pc_ = std::bitset<16>(s).to_ulong();
    is >> s;
    while (is) {
      other.memory_[now++] = std::bitset<16>(s).to_ulong();
      is >> s;
    }
    return is;
  }
  friend std::ostream& operator<<(std::ostream& os, Machine& other) {
    for (int i = 0; i < 8; i++)
      os << 'R' << i << " = x" << std::hex << std::uppercase << std::setw(4)
         << std::setfill('0') << other.reg_[i] << std::endl;
    return os;
  }
};

int main() {
  Machine m;
  std::cin >> m;
  m.Run();
  std::cout << m;
  return 0;
}
