#pragma once

void sensorsBegin();
float readLux();
bool readDHT(float &temperature, float &humidity);
