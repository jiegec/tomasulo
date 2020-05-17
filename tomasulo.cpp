#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

using namespace std;

enum ResStationType { AddSub, MulDiv, LoadBuffer };

struct ResStation {
  ResStationType type;
  bool busy;
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

vector<ResStation> res_stations;

void add_rs(int count, ResStationType type) {
  ResStation station;
  station.busy = false;
  station.type = type;
  for (int i = 0; i < count; i++) {
    res_stations.push_back(station);
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

  // reg file and reg status
  uint32_t reg_file[32] = {0};
  int reg_status[32] = {0};
  bool reg_status_busy[32] = {false};
  uint32_t pc = 0;

  // main loop
  while (true) {
    // dispatch
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
  }
  return 0;
}