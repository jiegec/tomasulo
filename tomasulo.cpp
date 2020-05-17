#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

using namespace std;

enum ResStationType { AddSub, MulDiv, LoadBuffer };

struct ResStation {
  ResStationType type;
  bool busy;
  bool executing;
  InstType op;
  // dest register
  int rd;

  // value of r1, vj
  uint32_t value_1;
  // dependency of r1, qj
  int rs_index_1;
  // r1 is ready, qj == NULL
  bool ready_1;

  // value of r2, vk
  uint32_t value_2;
  // dependency of r2, qk
  int rs_index_2;
  // r2 is ready, qk == NULL
  bool ready_2;
};

struct ExecUnit {
  ResStationType type;
  InstType op;
  bool busy;
  // cycles left
  int cycles_left;
  // where the inst comes from
  int rs_index;
  // register destinaiton
  int rd;
  // value of r1
  uint32_t value_1;
  // value of r2
  uint32_t value_2;
  // result value
  uint32_t res;
};

vector<ResStation> res_stations;
vector<ExecUnit> exec_units;

void add_rs(int count, ResStationType type) {
  ResStation station;
  station.busy = false;
  station.type = type;
  for (int i = 0; i < count; i++) {
    res_stations.push_back(station);
  }
}

void add_exec_unit(int count, ResStationType type) {
  ExecUnit exec_unit;
  exec_unit.busy = false;
  exec_unit.type = type;
  for (int i = 0; i < count; i++) {
    exec_units.push_back(exec_unit);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s input_nel output_log\n", argv[0]);
    return 1;
  }

  FILE *input_file = fopen(argv[1], "r");
  if (input_file == NULL) {
    printf("Unable to open file %s\n", argv[1]);
    return 1;
  }

  FILE *output_file = fopen(argv[2], "w");
  if (output_file == NULL) {
    printf("Unable to create file %s\n", argv[2]);
    return 1;
  }

  char buffer[1024];
  vector<Inst> instructions;

  while (!feof(input_file)) {
    if (fgets(buffer, sizeof(buffer), input_file) == NULL) {
      break;
    }
    instructions.push_back(parse_inst(buffer));
  }
  printf("Parsed %ld instructions\n", instructions.size());

  // six addsub, three muldiv, three load buffer
  add_rs(6, ResStationType::AddSub);
  add_rs(3, ResStationType::MulDiv);
  add_rs(3, ResStationType::LoadBuffer);
  // three addsub, two muldiv, two load buffer
  add_exec_unit(3, ResStationType::AddSub);
  add_exec_unit(2, ResStationType::MulDiv);
  add_exec_unit(2, ResStationType::LoadBuffer);

  // reg file and reg status
  uint32_t reg_file[32] = {0};
  int reg_status[32] = {0};
  bool reg_status_busy[32] = {false};
  uint32_t pc = 0;

  // main loop
  while (true) {
    // issue
    if (pc < instructions.size()) {
      struct Inst inst = instructions[pc];
      ResStationType type;
      switch (inst.type) {
      case InstType::Add:
      case InstType::Sub:
        type = ResStationType::AddSub;
        break;
      case InstType::Mul:
      case InstType::Div:
        type = ResStationType::MulDiv;
        break;
      case InstType::Load:
        type = ResStationType::LoadBuffer;
        break;
      default:
        assert(false);
        break;
      }

      for (int i = 0; i < res_stations.size(); i++) {
        if (res_stations[i].type == type && !res_stations[i].busy) {
          // res station is not busy, dispatch to it and bump pc
          res_stations[i].busy = true;
          res_stations[i].op = inst.type;

          switch (inst.type) {
          case InstType::Add:
          case InstType::Sub:
          case InstType::Mul:
          case InstType::Div:
            if (reg_status_busy[inst.rs1]) {
              // not ready
              res_stations[i].ready_1 = false;
              res_stations[i].rs_index_1 = reg_status[inst.rs1];
            } else {
              // ready
              res_stations[i].ready_1 = true;
              res_stations[i].value_1 = reg_file[inst.rs1];
            }

            if (reg_status_busy[inst.rs2]) {
              // not ready
              res_stations[i].ready_2 = false;
              res_stations[i].rs_index_2 = reg_status[inst.rs2];
            } else {
              // ready
              res_stations[i].ready_2 = true;
              res_stations[i].value_2 = reg_file[inst.rs2];
            }

            res_stations[i].rd = inst.rd;
            res_stations[i].busy = true;
            res_stations[i].executing = false;

            // update reg status
            reg_status[inst.rd] = i;
            reg_status_busy[inst.rd] = true;
            break;
          case InstType::Load:
            // only imm, no source register
            res_stations[i].value_1 = inst.imm;
            res_stations[i].ready_1 = true;
            res_stations[i].ready_2 = true;
            res_stations[i].rd = inst.rd;
            res_stations[i].busy = true;
            res_stations[i].executing = false;

            // update reg status
            reg_status[inst.rd] = i;
            reg_status_busy[inst.rd] = true;
            break;
          default:
            assert(false);
            break;
          }

          pc += 1;
          break;
        }
      }
    }

    // execute
    // for each empty exec unit, find a ready instruction
    for (int i = 0; i < exec_units.size(); i++) {
      if (!exec_units[i].busy) {
        for (int j = 0; j < res_stations.size(); j++) {
          // non busy && non executing && all operands are ready
          if (exec_units[i].type == res_stations[j].type &&
              res_stations[j].busy && !res_stations[j].executing &&
              res_stations[j].ready_1 && res_stations[j].ready_2) {
            res_stations[j].executing = true;
            exec_units[i].busy = true;

            exec_units[i].rs_index = j;
            exec_units[i].value_1 = res_stations[j].value_1;
            exec_units[i].value_2 = res_stations[j].value_2;
            exec_units[i].rd = res_stations[j].rd;
            exec_units[i].op = res_stations[j].op;

            switch (exec_units[i].op) {
            case InstType::Add:
              exec_units[i].res = exec_units[i].value_1 + exec_units[i].value_2;
              exec_units[i].cycles_left = 3;
              break;
            case InstType::Sub:
              exec_units[i].res = exec_units[i].value_1 - exec_units[i].value_2;
              exec_units[i].cycles_left = 3;
              break;
            case InstType::Mul:
              exec_units[i].res = exec_units[i].value_1 * exec_units[i].value_2;
              exec_units[i].cycles_left = 4;
              break;
            case InstType::Div:
              if (exec_units[i].value_2 == 0) {
                // div 0
                exec_units[i].res = exec_units[i].value_1;
                exec_units[i].cycles_left = 1;
              } else {
                exec_units[i].res =
                    exec_units[i].value_1 / exec_units[i].value_2;
                exec_units[i].cycles_left = 4;
              }
              break;
            case InstType::Load:
              exec_units[i].res = exec_units[i].value_1;
              exec_units[i].cycles_left = 3;
              break;
            default:
              assert(false);
              break;
            }
          }
        }
      }
    }

    // writeback
    // for each exec unit that has cycles_left = 0
    for (int i = 0; i < exec_units.size(); i++) {
      if (exec_units[i].busy) {
        exec_units[i].cycles_left -= 1;
        if (exec_units[i].cycles_left == 0) {
          exec_units[i].busy = false;
          res_stations[exec_units[i].rs_index].busy = false;

          // write to reg file
          int rd = exec_units[i].rd;
          if (reg_status[rd] == exec_units[i].rs_index && reg_status_busy[rd]) {
            reg_status_busy[rd] = false;
            reg_status[rd] = 0;
            reg_file[rd] = exec_units[i].res;
          }

          // common data bus, write to res stations
          for (int j = 0; j < res_stations.size(); j++) {
            if (res_stations[j].busy && !res_stations[j].ready_1 &&
                res_stations[j].rs_index_1 == exec_units[i].rs_index) {
              res_stations[j].ready_1 = true;
              res_stations[j].value_1 = exec_units[i].res;
            }

            if (res_stations[j].busy && !res_stations[j].ready_2 &&
                res_stations[j].rs_index_2 == exec_units[i].rs_index) {
              res_stations[j].ready_2 = true;
              res_stations[j].value_2 = exec_units[i].res;
            }
          }
        }
      }
    }
  }
  return 0;
}