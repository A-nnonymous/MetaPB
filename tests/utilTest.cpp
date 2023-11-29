#include "utils/typeParse.hpp"
#include <iostream>

void test_typeParse(){
  std::string ret = utils::typeParse<uint32_t>();
  std::cout << ret <<std::endl;
}
int main(){
  test_typeParse();
  return 0;
}