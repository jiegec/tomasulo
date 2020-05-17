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
  return 0;
}