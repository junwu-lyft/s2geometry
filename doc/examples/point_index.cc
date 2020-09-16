// Copyright 2017 Google Inc. All Rights Reserved.
// Author: ericv@google.com (Eric Veach)
//
// This example shows how to build and query an in-memory index of points
// using S2PointIndex.

#include "s2/s2cell_id.h"
#include "s2/s2latlng.h"


#include<iostream>

static S2CellId GetCellId(double lat_degrees, double lng_degrees) {
  S2CellId id(S2LatLng::FromDegrees(lat_degrees, lng_degrees));
  std::cout<< std::hex << id.id() <<std::endl;
  return id;
}

int main (int argc, char *argv[]) { 
  GetCellId(37.7539498,-122.4076829);
  return 0;
}