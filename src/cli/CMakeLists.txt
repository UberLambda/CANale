# CANale/src/cli/CMakeLists.txt
#
# Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(canale-cli
    main.cc
)
set_target_properties(canale-cli PROPERTIES
    OUTPUT_NAME "canale"
)
target_link_libraries(canale-cli PUBLIC
    canale
    Qt5::Core
)
