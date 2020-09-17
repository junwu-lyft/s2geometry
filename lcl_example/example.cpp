#include "s2lite.h"
#include <iostream>

int main (int argc, char *argv[]) { 
  uint64_t id = GetCellId(37.7539498,-122.4076829);
  std::cout<<"id = "<<id<<std::endl;
  std::vector<uint64_t> neighbors = GetEdgeNeighbors(id);

  std::cout<<"neighbors : "<<std::endl;
  for (int i =0; i < 4;++i)  {
  	std::cout<<neighbors[i]<<std::endl;
  }
  return 0;
}