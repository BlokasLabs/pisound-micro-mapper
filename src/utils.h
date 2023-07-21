#ifndef PISOUND_MICRO_UTILS_H
#define PISOUND_MICRO_UTILS_H

#include <string>

#include "control-manager.h"

std::string to_std_string(IControl::value_t value, IControl::Type t);

#endif // PISOUND_MICRO_UTILS_H
