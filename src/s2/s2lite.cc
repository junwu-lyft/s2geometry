#include "s2/s2cell_id.h"
#include "s2/s2latlng.h"

uint64_t GetCellId(double lat_degrees, double lng_degrees) {
  S2CellId id(S2LatLng::FromDegrees(lat_degrees, lng_degrees));
  return id.id();
}

std::vector<uint64_t> GetEdgeNeighbors(uint64_t id_) {
  S2CellId id(id_);
  S2CellId neighbors[4];
  id.GetEdgeNeighbors(neighbors);
  std::vector<uint64_t> ret;
  for (int i =0; i < 4;++i)  {
    ret.push_back(neighbors[i].id());
  }
  return ret;
}