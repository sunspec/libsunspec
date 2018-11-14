
/*
 * Copyright (C) 2014 SunSpec Alliance
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sunspec_error.h"
#include "sunspec_io.h"
#include "sunspec_log.h"

extern uint16_t cea2045_flush(bool txBufClear, bool rxBufClear, bool force);
extern int16_t CEA2045ss_read(unsigned char *buf, uint16_t len, uint32_t timeout);
extern int16_t CEA2045ss_write(unsigned char *buf, uint16_t len, uint32_t timeout);

typedef struct _suns_cea2045_t {
    uint16_t something;
} suns_cea2045_t;

suns_err_t
suns_cea2045_close(void *io)
{
    if (io == NULL) {
        return SUNS_ERR_INIT;
    }

    if (((suns_io_t *) io)->prot != NULL) {
        free(((suns_io_t *) io)->prot);
    }

    ((suns_io_t *) io)->prot = NULL;
    ((suns_io_t *) io)->connect = NULL;
    ((suns_io_t *) io)->disconnect = NULL;
    ((suns_io_t *) io)->read = NULL;
    ((suns_io_t *) io)->write = NULL;
    ((suns_io_t *) io)->close = NULL;
    ((suns_io_t *) io)->flush = NULL;

    return SUNS_ERR_OK;
}

suns_err_t
suns_cea2045_connect(void *prot, uint32_t timeout)
{
    return SUNS_ERR_OK;
}

suns_err_t
suns_cea2045_disconnect(void *prot)
{
    return SUNS_ERR_OK;
}

suns_err_t
suns_cea2045_read(void *prot, unsigned char *buf, uint16_t *len, uint32_t timeout)
{
    uint16_t ret;

    if ((ret = CEA2045ss_read(buf, *len, timeout)) > 0) {
        *len = ret;
        return SUNS_ERR_OK;
    }
    printf("suns_cea2045_read timeout\n");

    return SUNS_ERR_TIMEOUT;
}

suns_err_t
suns_cea2045_write(void *prot, unsigned char *buf, uint16_t len, uint32_t timeout)
{
    int16_t ret;
    suns_err_t err = SUNS_ERR_OK;

    if ((ret = CEA2045ss_write(buf, len, timeout)) != len) {
        if (ret == -1) {
            err = SUNS_ERR_BUF_SIZE;
        } else {
            err = SUNS_ERR_TIMEOUT;
        }
    }
    return err;
}

suns_err_t
suns_cea2045_flush(void *prot, bool flush_tx, bool flush_rx)
{
    uint16_t ret;
    suns_err_t err = SUNS_ERR_OK;

    if ((ret = cea2045_flush(flush_tx, flush_rx, false)) != 0) {
        err = SUNS_ERR_BUSY;
    }
    return err;
}

suns_err_t
suns_cea2045_open(suns_io_t *io)
{
    if (io == NULL) {
        return SUNS_ERR_INIT;
    }

    if ((io->prot = malloc(sizeof(suns_cea2045_t))) == NULL) {
        return SUNS_ERR_ALLOC;
    }
    ((suns_cea2045_t *) io->prot)->something = 0;

    io->connect = suns_cea2045_connect;
    io->disconnect = suns_cea2045_disconnect;
    io->read = suns_cea2045_read;
    io->write = suns_cea2045_write;
    io->close = suns_cea2045_close;
    io->flush = suns_cea2045_flush;
    return SUNS_ERR_OK;
}
