# CANale/src/CMakeLists.txt
#
# Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Core Widgets SerialBus REQUIRED)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)

add_subdirectory(3rdparty/)

add_library(canale SHARED
    canale.cc
    comms.cc
    comm_op.cc
    types.cc
)
set_target_properties(canale PROPERTIES
    DEFINE_SYMBOL "CA_EXPORTS"
)
target_link_libraries(canale PUBLIC
    Qt5::Core
    Qt5::SerialBus
    ELFIO
    CANnuccia
)

add_subdirectory(cli/)
add_subdirectory(gui/)
