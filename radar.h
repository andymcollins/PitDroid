#pragma once
struct RadarTarget {
  bool valid;
  int  distanceMm;
  float angleDeg;
};
void radarBegin();
RadarTarget radarRead();
