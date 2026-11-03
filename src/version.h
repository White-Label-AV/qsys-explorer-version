// Single source of the release version; used by the .rc files and package.cmd.
#pragma once

#define QPH_VERSION_MAJOR 1
#define QPH_VERSION_MINOR 0
#define QPH_VERSION_PATCH 0

#define QPH_STR2(x) #x
#define QPH_STR(x) QPH_STR2(x)
#define QPH_VERSION_STR QPH_STR(QPH_VERSION_MAJOR) "." QPH_STR(QPH_VERSION_MINOR) "." QPH_STR(QPH_VERSION_PATCH)

#define QPH_PUBLISHER "White Label AV"
#define QPH_COPYRIGHT "Copyright (c) 2026 White Label AV. MIT License."
