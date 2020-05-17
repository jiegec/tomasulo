#include "parser.h"
#include <stdio.h>
#include <vector>

using namespace std;

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

  // simulation without tomasulo
  size_t pc = 0;
  uint32_t reg[32];
  int cycle = 0;
  while (pc < instructions.size()) {
    cycle += 1;
    struct Inst cur = instructions[pc];
    if (cur.type == InstType::Jump) {
      if (reg[cur.rs1] == cur.imm) {
        // jump
        pc += cur.offset;
      } else {
        pc += 1;
      }
      continue;
    } else if (cur.type == InstType::Load) {
      reg[cur.rd] = cur.imm;
    } else if (cur.type == InstType::Mul) {
      reg[cur.rd] = reg[cur.rs1] * reg[cur.rs2];
    } else if (cur.type == InstType::Div) {
      if (reg[cur.rs2] == 0) {
        reg[cur.rd] = reg[cur.rs1];
      } else {
        reg[cur.rd] = reg[cur.rs1] / reg[cur.rs2];
      }
    } else if (cur.type == InstType::Add) {
      reg[cur.rd] = reg[cur.rs1] + reg[cur.rs2];
    } else if (cur.type == InstType::Sub) {
      reg[cur.rd] = reg[cur.rs1] - reg[cur.rs2];
    }
    pc += 1;
    fprintf(output_file, "%02d: R[%02d] = %08x\n", cycle, cur.rd, reg[cur.rd]);
  }
  return 0;
}