# ##############################################################################
# libs/libc/machine/arm/armv7-m/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

set(SRCS)

if(CONFIG_ARMV7M_MEMCHR)
  list(APPEND SRCS arch_memchr.S)
endif()

if(CONFIG_ARMV7M_MEMCPY)
  list(APPEND SRCS arch_memcpy.S)
endif()

if(CONFIG_ARMV7M_MEMSET)
  list(APPEND SRCS arch_memset.S)
endif()

if(CONFIG_ARMV7M_MEMMOVE)
  list(APPEND SRCS arch_memmove.S)
endif()

if(CONFIG_ARMV7M_STRCMP)
  list(APPEND SRCS arch_strcmp.S)
endif()

if(CONFIG_ARMV7M_STRCPY)
  list(APPEND SRCS arch_strcpy.S)
endif()

if(CONFIG_ARMV7M_STRLEN)
  list(APPEND SRCS arch_strlen.S)
endif()

if(CONFIG_LIBC_ARCH_ELF)
  list(APPEND SRCS arch_elf.c)
endif()

target_sources(c PRIVATE ${SRCS})
