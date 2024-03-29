#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

using namespace std;

enum ResStationType { AddSub, MulDiv, LoadBuffer };

// reservation stations
struct ResStation {
  ResStationType type;
  int pc;
  bool busy;
  bool executing;
  InstType op;
  // writes to register destination
  bool wrd;
  // dest register
  int rd;
  // jump offset
  int32_t offset;

  // value of r1, vj
  int32_t value_1;
  // dependency of r1, qj
  int rs_index_1;
  // r1 is ready, qj == NULL
  bool ready_1;

  // value of r2, vk
  int32_t value_2;
  // dependency of r2, qk
  int rs_index_2;
  // r2 is ready, qk == NULL
  bool ready_2;
};

// execution units
struct ExecUnit {
  ResStationType type;
  int pc;
  InstType op;
  bool busy;
  // cycles left
  int cycles_left;
  // where the inst comes from
  int rs_index;
  // writes to register destination
  bool wrd;
  // register destination
  int rd;
  // jump offset
  int32_t offset;
  // value of r1
  int32_t value_1;
  // value of r2
  int32_t value_2;
  // result value
  int32_t res;
};

// info when instruction first executes
struct InstInfo {
  int issue;
  int exec_complete;
  int write_res;
};

vector<ResStation> res_stations;
vector<ExecUnit> exec_units;
vector<InstInfo> inst_infos;

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
  if (argc < 3 || argc > 5) {
    printf("Usage: %s input_nel output_log [output_trace] [output_reg]\n",
           argv[0]);
    return 1;
  }

  FILE *input_nel = fopen(argv[1], "r");
  if (input_nel == NULL) {
    printf("Unable to open file %s\n", argv[1]);
    return 1;
  }

  FILE *output_log = fopen(argv[2], "w");
  if (output_log == NULL) {
    printf("Unable to create file %s\n", argv[2]);
    return 1;
  }

  FILE *output_trace = NULL;
  if (argc >= 4) {
    output_trace = fopen(argv[3], "w");
    if (output_trace == NULL) {
      printf("Unable to create file %s\n", argv[3]);
      return 1;
    }
  }

  FILE *output_reg = NULL;
  if (argc >= 5) {
    output_reg = fopen(argv[4], "w");
    if (output_reg == NULL) {
      printf("Unable to create file %s\n", argv[4]);
      return 1;
    }
  }

  char buffer[1024];
  vector<Inst> instructions;

  while (!feof(input_nel)) {
    if (fgets(buffer, sizeof(buffer), input_nel) == NULL) {
      break;
    }
    instructions.push_back(parse_inst(buffer));
  }
  printf("Parsed %ld instructions\n", instructions.size());
  inst_infos.resize(instructions.size());

  // six addsub, three muldiv, three load buffer
  add_rs(6, ResStationType::AddSub);
  add_rs(3, ResStationType::MulDiv);
  add_rs(3, ResStationType::LoadBuffer);
  // three addsub, two muldiv, two load buffer
  add_exec_unit(3, ResStationType::AddSub);
  add_exec_unit(2, ResStationType::MulDiv);
  add_exec_unit(2, ResStationType::LoadBuffer);

  // reg file and reg status
  int32_t reg_file[32] = {0};
  int reg_status[32] = {0};
  bool reg_status_busy[32] = {false};
  int pc = 0;
  bool issue_stall = false;

  // main loop
  int cycle = 1;
  while (true) {
    // end condition
    if (pc >= instructions.size()) {
      bool quit = true;
      for (int i = 0; i < res_stations.size(); i++) {
        if (res_stations[i].busy) {
          quit = false;
          break;
        }
      }

      if (quit) {
        break;
      }
    }

    // writeback
    // for each exec unit that has cycles_left = 0
    for (int i = 0; i < exec_units.size(); i++) {
      if (exec_units[i].busy) {
        if (exec_units[i].cycles_left == 0) {
          exec_units[i].busy = false;
          res_stations[exec_units[i].rs_index].busy = false;

          if (exec_units[i].op == InstType::Jump) {
            issue_stall = false;
            pc += exec_units[i].res;
          }

          if (inst_infos[exec_units[i].pc].exec_complete == 0) {
            inst_infos[exec_units[i].pc].exec_complete = cycle - 1;
            inst_infos[exec_units[i].pc].write_res = cycle;
          }

          if (exec_units[i].wrd) {
            // write to reg file
            int rd = exec_units[i].rd;
            if (reg_status[rd] == exec_units[i].rs_index &&
                reg_status_busy[rd]) {
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
        } else {
          exec_units[i].cycles_left -= 1;
        }
      }
    }

    // issue
    if (pc < instructions.size() && !issue_stall) {
      struct Inst inst = instructions[pc];
      ResStationType type;
      switch (inst.type) {
      case InstType::Add:
      case InstType::Sub:
      case InstType::Jump:
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

            break;
          case InstType::Jump:
            // one source register, one imm
            if (reg_status_busy[inst.rs1]) {
              // not ready
              res_stations[i].ready_1 = false;
              res_stations[i].rs_index_1 = reg_status[inst.rs1];
            } else {
              // ready
              res_stations[i].ready_1 = true;
              res_stations[i].value_1 = reg_file[inst.rs1];
            }

            res_stations[i].value_2 = inst.imm;
            res_stations[i].ready_2 = true;

            // stall issue stage
            issue_stall = true;
            break;
          case InstType::Load:
            // only imm, no source register
            res_stations[i].value_1 = inst.imm;
            res_stations[i].ready_1 = true;
            res_stations[i].ready_2 = true;
            break;
          default:
            assert(false);
            break;
          }

          res_stations[i].wrd = (inst.type != InstType::Jump);
          res_stations[i].rd = inst.rd;
          res_stations[i].pc = pc;
          res_stations[i].offset = inst.offset;
          res_stations[i].busy = true;
          res_stations[i].executing = false;

          // update reg status
          if (res_stations[i].wrd) {
            reg_status[inst.rd] = i;
            reg_status_busy[inst.rd] = true;
          }

          if (inst_infos[pc].issue == 0) {
            inst_infos[pc].issue = cycle;
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
              !exec_units[i].busy && res_stations[j].busy &&
              !res_stations[j].executing && res_stations[j].ready_1 &&
              res_stations[j].ready_2) {
            res_stations[j].executing = true;
            exec_units[i].busy = true;

            exec_units[i].rs_index = j;
            exec_units[i].offset = res_stations[j].offset;
            exec_units[i].value_1 = res_stations[j].value_1;
            exec_units[i].value_2 = res_stations[j].value_2;
            exec_units[i].rd = res_stations[j].rd;
            exec_units[i].wrd = res_stations[j].wrd;
            exec_units[i].pc = res_stations[j].pc;
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
            case InstType::Jump:
              // res = pc offset
              if (exec_units[i].value_1 == exec_units[i].value_2) {
                // reg == imm, jump
                exec_units[i].res = exec_units[i].offset - 1;
              } else {
                // don't jump
                exec_units[i].res = 0;
              }
              exec_units[i].cycles_left = 1;
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

    // output trace for debugging
    if (output_trace != NULL) {
      fprintf(output_trace, "Cycle: %d\n", cycle);
      fprintf(output_trace, "Reservation Stations:\n");
      for (int i = 0; i < res_stations.size(); i++) {
        if (res_stations[i].busy) {
          switch (res_stations[i].type) {
          case ResStationType::AddSub:
            fprintf(output_trace, "Ars %d:", i);
            break;
          case ResStationType::MulDiv:
            fprintf(output_trace, "Mrs %d:", i);
            break;
          case ResStationType::LoadBuffer:
            fprintf(output_trace, "LB %d:", i);
            break;
          default:
            assert(false);
            break;
          }

          switch (res_stations[i].op) {
          case InstType::Add:
            fprintf(output_trace, " ADD");
            break;
          case InstType::Sub:
            fprintf(output_trace, " SUB");
            break;
          case InstType::Mul:
            fprintf(output_trace, " MUL");
            break;
          case InstType::Div:
            fprintf(output_trace, " DIV");
            break;
          case InstType::Load:
            fprintf(output_trace, " LOAD");
            break;
          case InstType::Jump:
            fprintf(output_trace, " JUMP");
            break;
          default:
            assert(false);
            break;
          }

          if (res_stations[i].ready_1) {
            fprintf(output_trace, " vj=%08x", res_stations[i].value_1);
          } else {
            fprintf(output_trace, " qj=%08x", res_stations[i].rs_index_1);
          }

          if (res_stations[i].type != ResStationType::LoadBuffer) {
            if (res_stations[i].ready_2) {
              fprintf(output_trace, " vk=%08x", res_stations[i].value_2);
            } else {
              fprintf(output_trace, " qk=%08x", res_stations[i].rs_index_2);
            }
          }

          fprintf(output_trace, "\n");
        }
      }

      fprintf(output_trace, "Execution Unit:\n");
      for (int i = 0; i < exec_units.size(); i++) {
        if (exec_units[i].busy) {
          switch (exec_units[i].type) {
          case ResStationType::AddSub:
            fprintf(output_trace, "Add %d:", i);
            break;
          case ResStationType::MulDiv:
            fprintf(output_trace, "Mult %d:", i);
            break;
          case ResStationType::LoadBuffer:
            fprintf(output_trace, "Load %d:", i);
            break;
          default:
            assert(false);
            break;
          }

          switch (exec_units[i].op) {
          case InstType::Add:
            fprintf(output_trace, " ADD");
            break;
          case InstType::Sub:
            fprintf(output_trace, " SUB");
            break;
          case InstType::Mul:
            fprintf(output_trace, " MUL");
            break;
          case InstType::Div:
            fprintf(output_trace, " DIV");
            break;
          case InstType::Load:
            fprintf(output_trace, " LOAD");
            break;
          case InstType::Jump:
            fprintf(output_trace, " JUMP");
            break;
          default:
            assert(false);
            break;
          }

          if (exec_units[i].wrd) {
            fprintf(output_trace, " rd=%02d", exec_units[i].rd);
          }
          fprintf(output_trace, " vj=%08x", exec_units[i].value_1);

          if (exec_units[i].type != ResStationType::LoadBuffer) {
            fprintf(output_trace, " vk=%08x", exec_units[i].value_1);
          }
          fprintf(output_trace, " cycles=%d", exec_units[i].cycles_left);

          fprintf(output_trace, "\n");
        }
      }

      fprintf(output_trace, "Register Status:\n");
      for (int i = 0; i < 32; i++) {
        if (reg_status_busy[i]) {
          fprintf(output_trace, "R[%02d]=%d ", i, reg_status[i]);
        }
      }
      fprintf(output_trace, "\n");

      fprintf(output_trace, "\n");
    }

    cycle++;
  }

  for (int i = 0; i < instructions.size(); i++) {
    fprintf(output_log, "%d,%d,%d\n", inst_infos[i].issue,
            inst_infos[i].exec_complete, inst_infos[i].write_res);
  }

  // print reg result
  if (output_reg != NULL) {
    for (int i = 0; i < 32;i++) {
      fprintf(output_reg, "R[%02d]=%08x\n", i, reg_file[i]);
    }
  } 

  return 0;
}