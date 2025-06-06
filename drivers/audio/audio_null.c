/****************************************************************************
 * drivers/audio/audio_null.c
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

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/mqueue.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/audio/audio.h>
#include <nuttx/audio/audio_null.h>
#include <nuttx/signal.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct null_dev_s
{
  struct audio_lowerhalf_s dev; /* Audio lower half (this device) */
  bool          playback;       /* True: playback, False: recording */
  uint32_t      scaler;         /* Data bytes to sec scaler (bytes per sec) */
  struct file   mq;             /* Message queue for receiving messages */
  char          mqname[16];     /* Our message queue name */
  pthread_t     threadid;       /* ID of our thread */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  volatile bool terminate;      /* True: request to terminate */
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int   null_getcaps(FAR struct audio_lowerhalf_s *dev, int type,
                          FAR struct audio_caps_s *caps);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int   null_configure(FAR struct audio_lowerhalf_s *dev,
                            FAR void *session,
                            FAR const struct audio_caps_s *caps);
#else
static int   null_configure(FAR struct audio_lowerhalf_s *dev,
                            FAR const struct audio_caps_s *caps);
#endif
static int   null_shutdown(FAR struct audio_lowerhalf_s *dev);
static void *null_workerthread(pthread_addr_t pvarg);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int   null_start(FAR struct audio_lowerhalf_s *dev,
                        FAR void *session);
#else
static int   null_start(FAR struct audio_lowerhalf_s *dev);
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int   null_stop(FAR struct audio_lowerhalf_s *dev,
                       FAR void *session);
#else
static int   null_stop(FAR struct audio_lowerhalf_s *dev);
#endif
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int   null_pause(FAR struct audio_lowerhalf_s *dev,
                        FAR void *session);
static int   null_resume(FAR struct audio_lowerhalf_s *dev,
                         FAR void *session);
#else
static int   null_pause(FAR struct audio_lowerhalf_s *dev);
static int   null_resume(FAR struct audio_lowerhalf_s *dev);
#endif
#endif
static int   null_enqueuebuffer(FAR struct audio_lowerhalf_s *dev,
                                FAR struct ap_buffer_s *apb);
static int   null_cancelbuffer(FAR struct audio_lowerhalf_s *dev,
                               FAR struct ap_buffer_s *apb);
static int   null_ioctl(FAR struct audio_lowerhalf_s *dev, int cmd,
                        unsigned long arg);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int   null_reserve(FAR struct audio_lowerhalf_s *dev,
                          FAR void **session);
#else
static int   null_reserve(FAR struct audio_lowerhalf_s *dev);
#endif
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int   null_release(FAR struct audio_lowerhalf_s *dev,
                          FAR void *session);
#else
static int   null_release(FAR struct audio_lowerhalf_s *dev);
#endif
static int   null_sleep(FAR struct audio_lowerhalf_s *dev,
                        FAR struct ap_buffer_s *apb);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct audio_ops_s g_audioops =
{
  null_getcaps,       /* getcaps        */
  null_configure,     /* configure      */
  null_shutdown,      /* shutdown       */
  null_start,         /* start          */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  null_stop,          /* stop           */
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  null_pause,         /* pause          */
  null_resume,        /* resume         */
#endif
  NULL,               /* allocbuffer    */
  NULL,               /* freebuffer     */
  null_enqueuebuffer, /* enqueue_buffer */
  null_cancelbuffer,  /* cancel_buffer  */
  null_ioctl,         /* ioctl          */
  NULL,               /* read           */
  NULL,               /* write          */
  null_reserve,       /* reserve        */
  null_release        /* release        */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: null_sleep
 *
 * Description: Consume audio buffer in queue
 *
 ****************************************************************************/

static int null_sleep(FAR struct audio_lowerhalf_s *dev,
                      FAR struct ap_buffer_s *apb)
{
  FAR struct null_dev_s *priv = (struct null_dev_s *)dev;
  uint64_t sleep_time;

  sleep_time = USEC_PER_SEC * (uint64_t)apb->nbytes / priv->scaler;
  nxsig_usleep(sleep_time);
#ifdef CONFIG_AUDIO_MULTI_SESSION
  priv->dev.upper(priv->dev.priv, AUDIO_CALLBACK_DEQUEUE,
                  apb, OK, NULL);
#else
  priv->dev.upper(priv->dev.priv, AUDIO_CALLBACK_DEQUEUE,
                  apb, OK);
#endif
  if ((apb->flags & AUDIO_APB_FINAL) != 0)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      priv->dev.upper(priv->dev.priv,
                      AUDIO_CALLBACK_COMPLETE,
                      NULL,
                      OK,
                      NULL);
#else
      priv->dev.upper(priv->dev.priv,
                      AUDIO_CALLBACK_COMPLETE,
                      NULL,
                      OK);
#endif
    }

  return OK;
}

/****************************************************************************
 * Name: null_getcaps
 *
 * Description: Get the audio device capabilities
 *
 ****************************************************************************/

static int null_getcaps(FAR struct audio_lowerhalf_s *dev, int type,
                        FAR struct audio_caps_s *caps)
{
  FAR struct null_dev_s *priv = (struct null_dev_s *)dev;

  audinfo("type=%d\n", type);

  /* Validate the structure */

  DEBUGASSERT(caps->ac_len >= sizeof(struct audio_caps_s));

  /* Fill in the caller's structure based on requested info */

  caps->ac_format.hw  = 0;
  caps->ac_controls.w = 0;

  switch (caps->ac_type)
    {
      /* Caller is querying for the types of units we support */

      case AUDIO_TYPE_QUERY:

        /* Provide our overall capabilities.  The interfacing software
         * must then call us back for specific info for each capability.
         */

        caps->ac_channels = 2;       /* Stereo output */

        switch (caps->ac_subtype)
          {
            case AUDIO_TYPE_QUERY:
              /* We don't decode any formats!  Only something above us in
               * the audio stream can perform decoding on our behalf.
               */

              /* The types of audio units we implement */

              caps->ac_controls.b[0] = priv->playback ?
                                       AUDIO_TYPE_OUTPUT : AUDIO_TYPE_INPUT;
              caps->ac_format.hw = 1 << (AUDIO_FMT_PCM - 1);

              break;

            case AUDIO_FMT_MIDI:

              /* We only support Format 0 */

              caps->ac_controls.b[0] = AUDIO_SUBFMT_END;
              break;

            default:
              caps->ac_controls.b[0] = AUDIO_SUBFMT_END;
              break;
          }

        break;

      /* Provide capabilities of our OUTPUT unit */

      case AUDIO_TYPE_OUTPUT:
      case AUDIO_TYPE_INPUT:

        caps->ac_channels = 2;

        switch (caps->ac_subtype)
          {
            case AUDIO_TYPE_QUERY:

              /* Report the Sample rates we support */

              caps->ac_controls.hw[0] = AUDIO_SAMP_RATE_8K |
                                        AUDIO_SAMP_RATE_11K |
                                        AUDIO_SAMP_RATE_16K |
                                        AUDIO_SAMP_RATE_22K |
                                        AUDIO_SAMP_RATE_32K |
                                        AUDIO_SAMP_RATE_44K |
                                        AUDIO_SAMP_RATE_48K;
              break;

            case AUDIO_FMT_MP3:
            case AUDIO_FMT_WMA:
            case AUDIO_FMT_PCM:
              break;

            default:
              break;
          }

        break;

      /* Provide capabilities of our FEATURE units */

      case AUDIO_TYPE_FEATURE:

        /* If the sub-type is UNDEF,
         * then report the Feature Units we support
         */

        if (caps->ac_subtype == AUDIO_FU_UNDEF)
          {
            /* Fill in the ac_controls section with
             * the Feature Units we have
             */

            caps->ac_controls.b[0] = AUDIO_FU_VOLUME |
                                     AUDIO_FU_BASS |
                                     AUDIO_FU_TREBLE;
            caps->ac_controls.b[1] = AUDIO_FU_BALANCE >> 8;
          }
        else
          {
            /* TODO:  Do we need to provide specific info for the
             * Feature Units, such as volume setting ranges, etc.?
             */
          }

        break;

      /* Provide capabilities of our PROCESSING unit */

      case AUDIO_TYPE_PROCESSING:

        switch (caps->ac_subtype)
          {
            case AUDIO_PU_UNDEF:

              /* Provide the type of Processing Units we support */

              caps->ac_controls.b[0] = AUDIO_PU_STEREO_EXTENDER;
              break;

            case AUDIO_PU_STEREO_EXTENDER:

              /* Provide capabilities of our Stereo Extender */

              caps->ac_controls.b[0] = AUDIO_STEXT_ENABLE |
                                       AUDIO_STEXT_WIDTH;
              break;

            default:

              /* Other types of processing uint we don't support */

              break;
          }

        break;

      /* All others we don't support */

      default:

        /* Zero out the fields to indicate no support */

        caps->ac_subtype = 0;
        caps->ac_channels = 0;

        break;
    }

  /* Return the length of the audio_caps_s struct for validation of
   * proper Audio device type.
   */

  audinfo("Return %d\n", caps->ac_len);
  return caps->ac_len;
}

/****************************************************************************
 * Name: null_configure
 *
 * Description:
 *   Configure the audio device for the specified  mode of operation.
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int null_configure(FAR struct audio_lowerhalf_s *dev,
                          FAR void *session,
                          FAR const struct audio_caps_s *caps)
#else
static int null_configure(FAR struct audio_lowerhalf_s *dev,
                          FAR const struct audio_caps_s *caps)
#endif
{
  FAR struct null_dev_s *priv = (FAR struct null_dev_s *)dev;
  audinfo("ac_type: %d\n", caps->ac_type);

  if (priv->mqname[0] == '\0')
    {
      struct mq_attr attr;
      int ret;

      /* Create a message queue for the worker thread */

      snprintf(priv->mqname, sizeof(priv->mqname), "/tmp/%" PRIXPTR,
               (uintptr_t)priv);

      attr.mq_maxmsg  = 16;
      attr.mq_msgsize = sizeof(struct audio_msg_s);
      attr.mq_curmsgs = 0;
      attr.mq_flags   = 0;

      ret = file_mq_open(&priv->mq, priv->mqname,
                         O_RDWR | O_CREAT, 0644, &attr);
      if (ret < 0)
        {
          /* Error creating message queue! */

          auderr("ERROR: Couldn't allocate message queue\n");
          return ret;
        }
    }

  /* Process the configure operation */

  switch (caps->ac_type)
    {
    case AUDIO_TYPE_FEATURE:
      audinfo("  AUDIO_TYPE_FEATURE\n");

      /* Process based on Feature Unit */

      switch (caps->ac_format.hw)
        {
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
        case AUDIO_FU_VOLUME:
          audinfo("    Volume: %d\n", caps->ac_controls.hw[0]);
          break;
#endif /* CONFIG_AUDIO_EXCLUDE_VOLUME */

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
        case AUDIO_FU_BASS:
          audinfo("    Bass: %d\n", caps->ac_controls.b[0]);
          break;

        case AUDIO_FU_TREBLE:
          audinfo("    Treble: %d\n", caps->ac_controls.b[0]);
          break;
#endif /* CONFIG_AUDIO_EXCLUDE_TONE */

        default:
          auderr("    ERROR: Unrecognized feature unit\n");
          break;
        }
      break;

    case AUDIO_TYPE_OUTPUT:
    case AUDIO_TYPE_INPUT:
      priv->scaler = caps->ac_channels
                   * caps->ac_controls.hw[0]
                   * caps->ac_controls.b[2] / 8;
      audinfo("    Number of channels: %u\n", caps->ac_channels);
      audinfo("    Sample rate:        %u\n", caps->ac_controls.hw[0]);
      audinfo("    Sample width:       %u\n", caps->ac_controls.b[2]);
      break;

    case AUDIO_TYPE_PROCESSING:
      audinfo("  AUDIO_TYPE_PROCESSING:\n");
      break;
    }

  audinfo("Return OK\n");
  return OK;
}

/****************************************************************************
 * Name: null_shutdown
 *
 * Description:
 *   Shutdown the driver and put it in the lowest power state possible.
 *
 ****************************************************************************/

static int null_shutdown(FAR struct audio_lowerhalf_s *dev)
{
  audinfo("Return OK\n");
  return OK;
}

/****************************************************************************
 * Name: null_workerthread
 *
 *  This is the thread that feeds data to the chip and keeps the audio
 *  stream going.
 *
 ****************************************************************************/

static void *null_workerthread(pthread_addr_t pvarg)
{
  FAR struct null_dev_s *priv = (FAR struct null_dev_s *) pvarg;
  struct audio_msg_s msg;
  int msglen;
  unsigned int prio;

  audinfo("Entry\n");

  /* Loop as long as we are supposed to be running */

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  while (!priv->terminate)
#else
  for (; ; )
#endif
    {
      /* Wait for messages from our message queue */

      msglen = file_mq_receive(&priv->mq, (FAR char *)&msg,
                               sizeof(msg), &prio);

      /* Handle the case when we return with no message */

      if (msglen < sizeof(struct audio_msg_s))
        {
          auderr("ERROR: Message too small: %d\n", msglen);
          continue;
        }

      /* Process the message */

      switch (msg.msg_id)
        {
          case AUDIO_MSG_DATA_REQUEST:
            break;

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
          case AUDIO_MSG_STOP:
            priv->terminate = true;
            break;
#endif

          case AUDIO_MSG_ENQUEUE:
            null_sleep(&priv->dev, (FAR struct ap_buffer_s *)msg.u.ptr);
            break;

          case AUDIO_MSG_COMPLETE:
            break;

          default:
            auderr("ERROR: Ignoring message ID %d\n", msg.msg_id);
            break;
        }
    }

  /* Close the message queue */

  file_mq_close(&priv->mq);
  file_mq_unlink(priv->mqname);
  priv->mqname[0] = '\0';
  priv->terminate = false;

  /* Send an AUDIO_MSG_COMPLETE message to the client */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  priv->dev.upper(priv->dev.priv, AUDIO_CALLBACK_COMPLETE, NULL, OK, NULL);
#else
  priv->dev.upper(priv->dev.priv, AUDIO_CALLBACK_COMPLETE, NULL, OK);
#endif

  audinfo("Exit\n");
  return NULL;
}

/****************************************************************************
 * Name: null_start
 *
 * Description:
 *   Start the configured operation (audio streaming, volume enabled, etc.).
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int null_start(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int null_start(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct null_dev_s *priv = (FAR struct null_dev_s *)dev;
  struct sched_param sparam;
  pthread_attr_t tattr;
  FAR void *value;
  int ret;

  audinfo("Entry\n");

  /* Join any old worker thread we had created to prevent a memory leak */

  if (priv->threadid != 0)
    {
      audinfo("Joining old thread\n");
      pthread_join(priv->threadid, &value);
    }

  /* Start our thread for sending data to the device */

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 3;
  pthread_attr_setschedparam(&tattr, &sparam);
  pthread_attr_setstacksize(&tattr, CONFIG_AUDIO_NULL_WORKER_STACKSIZE);

  audinfo("Starting worker thread\n");
  ret = pthread_create(&priv->threadid, &tattr, null_workerthread,
                       (pthread_addr_t)priv);
  if (ret != OK)
    {
      auderr("ERROR: pthread_create failed: %d\n", ret);
    }
  else
    {
      pthread_setname_np(priv->threadid, "null audio");
      audinfo("Created worker thread\n");
    }

  audinfo("Return %d\n", ret);
  return ret;
}

/****************************************************************************
 * Name: null_stop
 *
 * Description: Stop the configured operation (audio streaming, volume
 *              disabled, etc.).
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int null_stop(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int null_stop(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct null_dev_s *priv = (FAR struct null_dev_s *)dev;
  struct audio_msg_s term_msg;
  FAR void *value;

  /* Send a message to stop all audio streaming */

  /* REVISIT:
   * There should be a check to see if the worker thread is still  running.
   */

  term_msg.msg_id = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  file_mq_send(&priv->mq, (FAR const char *)&term_msg, sizeof(term_msg),
               CONFIG_AUDIO_NULL_MSG_PRIO);

  /* Join the worker thread */

  pthread_join(priv->threadid, &value);
  priv->threadid = 0;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  dev->upper(dev->priv, AUDIO_CALLBACK_COMPLETE, NULL, OK, NULL);
#else
  dev->upper(dev->priv, AUDIO_CALLBACK_COMPLETE, NULL, OK);
#endif

  audinfo("Return OK\n");
  return OK;
}
#endif

/****************************************************************************
 * Name: null_pause
 *
 * Description: Pauses the playback.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int null_pause(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int null_pause(FAR struct audio_lowerhalf_s *dev)
#endif
{
  audinfo("Return OK\n");
  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: null_resume
 *
 * Description: Resumes the playback.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int null_resume(FAR struct audio_lowerhalf_s *dev, FAR void *session)
#else
static int null_resume(FAR struct audio_lowerhalf_s *dev)
#endif
{
  audinfo("Return OK\n");
  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: null_enqueuebuffer
 *
 * Description: Enqueue an Audio Pipeline Buffer for playback/ processing.
 *
 ****************************************************************************/

static int null_enqueuebuffer(FAR struct audio_lowerhalf_s *dev,
                              FAR struct ap_buffer_s *apb)
{
  FAR struct null_dev_s *priv = (FAR struct null_dev_s *)dev;
  struct audio_msg_s msg;
  int ret;

  DEBUGASSERT(priv && apb && priv->dev.upper);

  audinfo("apb=%p curbyte=%d nbytes=%d\n", apb, apb->curbyte, apb->nbytes);

  msg.msg_id = AUDIO_MSG_ENQUEUE;
  msg.u.ptr = apb;

  ret = file_mq_send(&priv->mq, (FAR const char *)&msg,
                     sizeof(msg), CONFIG_AUDIO_NULL_MSG_PRIO);
  if (ret < 0)
    {
      auderr("ERROR: file_mq_send failed: %d\n", ret);
    }

  audinfo("Return OK\n");
  return ret;
}

/****************************************************************************
 * Name: null_cancelbuffer
 *
 * Description: Called when an enqueued buffer is being cancelled.
 *
 ****************************************************************************/

static int null_cancelbuffer(FAR struct audio_lowerhalf_s *dev,
                             FAR struct ap_buffer_s *apb)
{
  audinfo("apb=%p curbyte=%d nbytes=%d, return OK\n",
          apb, apb->curbyte, apb->nbytes);

  return OK;
}

/****************************************************************************
 * Name: null_ioctl
 *
 * Description: Perform a device ioctl
 *
 ****************************************************************************/

static int null_ioctl(FAR struct audio_lowerhalf_s *dev, int cmd,
                      unsigned long arg)
{
  int ret = OK;
#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  FAR struct ap_buffer_info_s *bufinfo;
#endif

  audinfo("cmd=%d arg=%ld\n", cmd, arg);

  /* Deal with ioctls passed from the upper-half driver */

  switch (cmd)
    {
      /* Check for AUDIOIOC_HWRESET ioctl.  This ioctl is passed straight
       * through from the upper-half audio driver.
       */

      case AUDIOIOC_HWRESET:
        {
          audinfo("AUDIOIOC_HWRESET:\n");
        }
        break;

       /* Report our preferred buffer size and quantity */

#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
      case AUDIOIOC_GETBUFFERINFO:
        {
          audinfo("AUDIOIOC_GETBUFFERINFO:\n");
          bufinfo              = (FAR struct ap_buffer_info_s *) arg;
          bufinfo->buffer_size = CONFIG_AUDIO_NULL_BUFFER_SIZE;
          bufinfo->nbuffers    = CONFIG_AUDIO_NULL_NUM_BUFFERS;
        }
        break;
#endif

      default:
        ret = -ENOTTY;
        break;
    }

  audinfo("Return OK\n");
  return ret;
}

/****************************************************************************
 * Name: null_reserve
 *
 * Description: Reserves a session (the only one we have).
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int null_reserve(FAR struct audio_lowerhalf_s *dev,
                        FAR void **session)
#else
static int null_reserve(FAR struct audio_lowerhalf_s *dev)
#endif
{
  audinfo("Return OK\n");
  return OK;
}

/****************************************************************************
 * Name: null_release
 *
 * Description: Releases the session (the only one we have).
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int null_release(FAR struct audio_lowerhalf_s *dev,
                        FAR void *session)
#else
static int null_release(FAR struct audio_lowerhalf_s *dev)
#endif
{
  FAR struct null_dev_s *priv = (FAR struct null_dev_s *)dev;
  void  *value;

  /* Join any old worker thread we had created to prevent a memory leak */

  if (priv->threadid != 0)
    {
      pthread_join(priv->threadid, &value);
      priv->threadid = 0;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: audio_null_initialize
 *
 * Description:
 *   Initialize the null audio device.
 *
 * Input Parameters:
 *   playback - True: initialize for playback only
 *              False: initialize for recording only
 *
 * Returned Value:
 *   A new lower half audio interface for the NULL audio device is returned
 *   on success; NULL is returned on failure.
 *
 ****************************************************************************/

FAR struct audio_lowerhalf_s *audio_null_initialize(bool playback)
{
  FAR struct null_dev_s *priv;

  /* Allocate the null audio device structure */

  priv = kmm_zalloc(sizeof(struct null_dev_s));
  if (priv)
    {
      /* Initialize the null audio device structure.
       * Since we used kmm_zalloc, only the non-zero elements
       * of the structure need to be initialized.
       */

      priv->dev.ops = &g_audioops;
      return &priv->dev;
    }

  priv->playback = playback;

  auderr("ERROR: Failed to allocate null audio device\n");
  return NULL;
}
