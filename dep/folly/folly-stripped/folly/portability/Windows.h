/*
 * Copyright 2017 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

 // Only do anything if we are on windows.
#ifdef _WIN32

#if defined(min) || defined(max)
#error Windows.h needs to be included by this header, or else NOMINMAX needs \
 to be defined before including it yourself.
#endif

 // This is needed because, for some absurd reason, one of the windows headers
 // tries to define "min" and "max" as macros, which messes up most uses of
 // std::numeric_limits.
#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#include <WinSock2.h> // @manual
#include <direct.h> // @manual nolint
#include <io.h> // @manual nolint

#endif
