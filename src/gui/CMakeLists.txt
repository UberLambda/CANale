# CANale/src/gui/CMakeLists.txt
#
# Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(canale-gui WIN32
    main.cc
)
set_target_properties(canale-gui PROPERTIES
    OUTPUT_NAME "canale-gui"
)
target_link_libraries(canale-gui PUBLIC
    canale
    Qt5::Core
    Qt5::Widgets
)
