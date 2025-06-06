/****************************************************************************
 * libs/libc/stream/lib_lzfcompress.c
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

#include <nuttx/config.h>

#include <unistd.h>
#include <nuttx/streams.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lzfoutstream_flush
 ****************************************************************************/

static int lzfoutstream_flush(FAR struct lib_outstream_s *self)
{
  FAR struct lib_lzfoutstream_s *stream =
                                 (FAR struct lib_lzfoutstream_s *)self;
  FAR struct lzf_header_s *header;
  size_t outlen;

  if (stream->offset > 0)
    {
      outlen = lzf_compress(stream->in, stream->offset,
                            &stream->out[LZF_MAX_HDR_SIZE],
                            stream->offset, stream->state, &header);
      if (outlen > 0)
        {
          lib_stream_puts(stream->backend, header, outlen);
        }

      stream->offset = 0;
    }

  return lib_stream_flush(stream->backend);
}

/****************************************************************************
 * Name: lzfoutstream_puts
 ****************************************************************************/

static ssize_t lzfoutstream_puts(FAR struct lib_outstream_s *self,
                                 FAR const void *buf, size_t len)
{
  FAR struct lib_lzfoutstream_s *stream =
                                 (FAR struct lib_lzfoutstream_s *)self;
  FAR struct lzf_header_s *header;
  FAR const char *ptr = buf;
  size_t total = len;
  size_t copying;
  size_t outlen;
  ssize_t ret;

  while (total > 0)
    {
      copying = stream->offset + total > LZF_STREAM_BLOCKSIZE ?
                LZF_STREAM_BLOCKSIZE - stream->offset : total;

      memcpy(stream->in + stream->offset, ptr, copying);

      ptr            += copying;
      stream->offset += copying;
      self->nput     += copying;
      total          -= copying;

      if (stream->offset == LZF_STREAM_BLOCKSIZE)
        {
          outlen = lzf_compress(stream->in, stream->offset,
                                &stream->out[LZF_MAX_HDR_SIZE],
                                stream->offset, stream->state,
                                &header);
          if (outlen > 0)
            {
              ret = lib_stream_puts(stream->backend, header, outlen);
              if (ret < 0)
                {
                  return ret;
                }
            }

          stream->offset = 0;
        }
    }

  return len;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lib_lzfoutstream
 *
 * Description:
 *  LZF compressed pipeline stream
 *
 * Input Parameters:
 *   stream  - User allocated, uninitialized instance of struct
 *                lib_lzfoutstream_s to be initialized.
 *   backend - Stream backend port.
 *
 * Returned Value:
 *   None (User allocated instance initialized).
 *
 ****************************************************************************/

void lib_lzfoutstream(FAR struct lib_lzfoutstream_s *stream,
                      FAR struct lib_outstream_s *backend)
{
  if (stream == NULL || backend == NULL)
    {
      return;
    }

  memset(stream, 0, sizeof(*stream));
  stream->common.puts  = lzfoutstream_puts;
  stream->common.flush = lzfoutstream_flush;
  stream->backend      = backend;
}
