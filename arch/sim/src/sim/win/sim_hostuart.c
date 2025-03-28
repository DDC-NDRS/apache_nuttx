/****************************************************************************
 * arch/sim/src/sim/win/sim_hostuart.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <windows.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NUM_INPUT 16

/****************************************************************************
 * Private Data
 ****************************************************************************/

static HANDLE g_stdin_handle;
static HANDLE g_stdout_handle;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: host_uart_start
 ****************************************************************************/

void host_uart_start(void)
{
  DWORD mode;

  g_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
  if (GetConsoleMode(g_stdin_handle, &mode))
    {
      SetConsoleMode(g_stdin_handle, mode & ~(ENABLE_MOUSE_INPUT |
                                              ENABLE_WINDOW_INPUT |
                                              ENABLE_ECHO_INPUT |
                                              ENABLE_LINE_INPUT));
      FlushConsoleInputBuffer(g_stdin_handle);
    }

  g_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
}

/****************************************************************************
 * Name: host_uart_open
 ****************************************************************************/

int host_uart_open(const char *pathname)
{
  return -ENOSYS;
}

/****************************************************************************
 * Name: host_uart_close
 ****************************************************************************/

void host_uart_close(int fd)
{
}

/****************************************************************************
 * Name: host_uart_putc
 ****************************************************************************/

int host_uart_puts(int fd, const char *buf, size_t size)
{
  DWORD nwritten;
  int ret;

  ret = WriteConsole(g_stdout_handle, buf, size, &nwritten, NULL);

  return ret == 0 ? -EIO : nwritten;
}

/****************************************************************************
 * Name: host_uart_checkin
 ****************************************************************************/

bool host_uart_checkin(int fd)
{
  DWORD size;

  if (GetNumberOfConsoleInputEvents(g_stdin_handle, &size) && size > 1)
    {
      return true;
    }

  return false;
}

/****************************************************************************
 * Name: host_uart_checkout
 ****************************************************************************/

bool host_uart_checkout(int fd)
{
  return true;
}

/****************************************************************************
 * Name: host_uart_getc
 ****************************************************************************/

int host_uart_gets(int fd, char *buf, size_t size)
{
  INPUT_RECORD input[NUM_INPUT];
  char *pos = buf;
  DWORD ninput;
  int i;

  while (size > 0 && host_uart_checkin(fd))
    {
      ninput = size > NUM_INPUT ? NUM_INPUT : size;
      if (ReadConsoleInput(g_stdin_handle,
                           (void *)&input, ninput, &ninput) <= 0 ||
          ninput == 0)
        {
          break;
        }

      for (i = 0; i < ninput; i++)
        {
          if (input[i].EventType == KEY_EVENT &&
              input[i].Event.KeyEvent.bKeyDown &&
              input[i].Event.KeyEvent.uChar.AsciiChar != 0)
            {
              *pos++ = input[i].Event.KeyEvent.uChar.AsciiChar;
              size--;
            }
        }
    }

  return pos == buf ? -EIO : pos - buf;
}

/****************************************************************************
 * Name: host_uart_getcflag
 ****************************************************************************/

int host_uart_getcflag(int fd, unsigned int *cflag)
{
  return -ENOSYS;
}

/****************************************************************************
 * Name: host_uart_setcflag
 ****************************************************************************/

int host_uart_setcflag(int fd, unsigned int cflag)
{
  return -ENOSYS;
}
