#ifndef __PARSER_H__
#define __PARSER_H__

#include <vector>
#include <stdint.h>

enum InstType { Add, Mul, Sub, Div, Load, Jump };

struct Inst {
  InstType type;
  // target
  int rd;
  // source
  int rs1;
  int rs2;
  int32_t imm;
  // jump offset
  int32_t offset;
};

Inst parse_inst(const char *line);

#endif