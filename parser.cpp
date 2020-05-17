#include "parser.h"
#include <stdio.h>
#include <assert.h>

Inst parse_inst(const char *line) {
  struct Inst ret;
  if (line[0] == 'A') {
    // ADD
    ret.type = InstType::Add;
    sscanf(line, "ADD,R%d,R%d,R%d", &ret.rd, &ret.rs1, &ret.rs2);
  } else if (line[0] == 'S') {
    // SUB
    ret.type = InstType::Sub;
    sscanf(line, "SUB,R%d,R%d,R%d", &ret.rd, &ret.rs1, &ret.rs2);
  } else if (line[0] == 'M') {
    // MUL
    ret.type = InstType::Mul;
    sscanf(line, "MUL,R%d,R%d,R%d", &ret.rd, &ret.rs1, &ret.rs2);
  } else if (line[0] == 'D') {
    // DIV
    ret.type = InstType::Div;
    sscanf(line, "DIV,R%d,R%d,R%d", &ret.rd, &ret.rs1, &ret.rs2);
  } else if (line[0] == 'L') {
    // LD
    ret.type = InstType::Load;
    sscanf(line, "LD,R%d,%x", &ret.rd, &ret.imm);
  } else if (line[0] == 'J') {
    // JUMP
    ret.type = InstType::Jump;
    sscanf(line, "JUMP,%x,R%d,%x", &ret.rd, &ret.imm, &ret.offset);
  } else {
    fprintf(stderr, "Invalid instruction: %s\n", line);
    assert(false);
  }
  return ret;
}