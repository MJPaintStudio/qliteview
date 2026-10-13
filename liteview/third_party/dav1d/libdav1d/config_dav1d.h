//
// Copyright (c) 2025 Institute of Software, Chinese Academy of Sciences (ISCAS)
// LiteView is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
//          http://license.coscl.org.cn/MulanPSL2
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
//


#if defined(WIN32)

#if defined(_WIN64)
#include "../config/win/x64/config.h"
// The embedded Windows port builds the portable decoder without external NASM.
#undef HAVE_ASM
#define HAVE_ASM 0
#elif defined(_M_IX86) || defined(__i386__)
#include "../config/win/x86/config_dav1d_x86.h"
#endif

#else

#endif
