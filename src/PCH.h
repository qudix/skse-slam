#pragma once

#include "SKSE/SKSE.h"
#include "RE/Skyrim.h"

#include <random>
#include <unordered_set>

#ifndef NDEBUG
#include <spdlog/sinks/msvc_sink.h>
#else
#include <spdlog/sinks/basic_file_sink.h>
#endif

using namespace std::literals;

namespace logger = SKSE::log;

#define DLLEXPORT __declspec(dllexport)