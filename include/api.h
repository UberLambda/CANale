// CANale/include/api.h - DLL export marker for the CANale public API
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef API_H
#define API_H

#if defined(WIN32) || defined(_WIN32)
#   if defined(CA_EXPORTS)
#       define CA_API __declspec(dllexport)
#   else
#       define CA_API __declspec(dllimport)
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define CA_API __attribute__((visibility("default")))
#else
#   define CA_API
#endif

#endif // API_H
