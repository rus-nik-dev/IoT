#pragma once

enum Mode { MONITORING, SILENT };

void buttonBegin();
bool isButtonPressedWithDebounce();
