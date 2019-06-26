# CANale/src/3rdparty/CMakeLists.txt
#
# Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# CANnuccia (include only) - Upstream: https://github.com/UberLambda/CANnuccia - License: MPL2
add_library(CANnuccia INTERFACE)
target_include_directories(CANnuccia INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/CANnuccia/src/"
)

# ELFIO - Upstream: https://github.com/serge1/ELFIO - License: MIT
add_library(ELFIO INTERFACE)
target_include_directories(ELFIO INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/ELFIO/"
)
