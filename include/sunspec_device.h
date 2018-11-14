
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

#ifndef _SUNSPEC_DEVICE_H_
#define _SUNSPEC_DEVICE_H_

#include <stdint.h>

#include "sunspec_modbus.h"
#include "sunspec_value.h"

#define SUNS_MODEL_LEN_MAX              0x8000
#define SUNS_MODBUS_READ_MAX_LEN        (4 * 1024)
#define SUNS_BASE_ADDR_UNKNOWN          0xffff
#define SUNS_MODEL_ID_END               0xffff

#define SUNS_MODEL_PATH_LEN             256
#define SUNS_MODEL_BUF_SIZE             (4 * 1024)

#define SUNS_TAG_MODELS_ROOT            "sunSpecModels"
#define SUNS_TAG_MODEL                  "model"
#define SUNS_TAG_BLOCK                  "block"
#define SUNS_TAG_POINT                  "point"
#define SUNS_ATTR_ID                    "id"
#define SUNS_ATTR_LEN                   "len"
#define SUNS_ATTR_SIZE                  "size"
#define SUNS_ATTR_NAME                  "name"
#define SUNS_ATTR_OFFSET                "offset"
#define SUNS_ATTR_TYPE                  "type"
#define SUNS_ATTR_SF                    "sf"
#define SUNS_ATTR_UNITS                 "units"
#define SUNS_ATTR_VALUE_REPEAT          "repeating"

/* SunSpec data types */
#define SUNS_TYPE_INT16	                1
#define SUNS_TYPE_UINT16                2
#define SUNS_TYPE_ACC16                 3
#define SUNS_TYPE_INT32                 4
#define SUNS_TYPE_UINT32                5
#define SUNS_TYPE_ACC32                 6
#define SUNS_TYPE_INT64                 7
#define SUNS_TYPE_UINT64                8
#define SUNS_TYPE_ACC64                 9
#define SUNS_TYPE_FLOAT32               10
#define SUNS_TYPE_SUNSSF                11
#define SUNS_TYPE_ENUM16                12
#define SUNS_TYPE_ENUM32                13
#define SUNS_TYPE_BIT16                 14
#define SUNS_TYPE_BIT32                 15
#define SUNS_TYPE_PAD                   16
#define SUNS_TYPE_IPADDR                17
#define SUNS_TYPE_IPV6ADDR              18
#define SUNS_TYPE_STR                   19

typedef struct _suns_data_t {
    char *id;
    int16_t type;
    int16_t base_type;
    int16_t len;
    int16_t has_unimpl_value;
    suns_value_t unimpl_value;
    void (*to_float)(suns_value_t value, int16_t sf, float *f32);
    void (*from_float)(suns_value_t *value, int16_t sf, float f32);
    void (*to_str)(suns_value_t value, int16_t sf, char *str, uint16_t len);
    uint16_t (*is_implemented)(suns_value_t value);
    void (*modbus_to_value)(unsigned char *buf, suns_value_t *value, uint16_t len);
    void (*modbus_from_value)(unsigned char *buf, suns_value_t value, uint16_t len);
} suns_data_t;

typedef struct _suns_point_def_t {
    char *id;
    uint16_t offset;
    suns_data_t *type;
    uint16_t len;
    uint8_t required;
    uint8_t access;
    char *sf_name;
    uint16_t sf_offset;
    int16_t sf_value;
    char *units;
    struct _suns_point_def_t *next;
} suns_point_def_t;

typedef struct _suns_block_def_t {
    uint16_t len;
    uint8_t repeating;
    suns_point_def_t *points;
    struct _suns_block_def_t *next;
} suns_block_def_t;

#define SUNS_MODEL_ID_LEN               16
#define SUNS_MODEL_NAME_LEN             64
#define SUNS_BLOCK_FIXED                0
#define SUNS_BLOCK_REPEATING            1
#define SUNS_BLOCK_TYPE_COUNT           2

typedef struct _suns_model_def_t {
    uint16_t id;
    uint16_t len;
    char id_str[SUNS_MODEL_ID_LEN];
    char name[SUNS_MODEL_NAME_LEN];
    suns_block_def_t *blocks[SUNS_BLOCK_TYPE_COUNT];
    struct _suns_model_def_t *next;
} suns_model_def_t;

typedef struct _suns_point_t {
    struct _suns_block_t *block;
    suns_point_def_t *point_def;
    uint16_t addr;
    struct _suns_point_t *sf_point;
    uint8_t impl;
    suns_value_t value_base;
    uint16_t value_sf;
    char *value_ptr;
    uint8_t dirty;
    struct _suns_point_t *next;
} suns_point_t;

typedef struct _suns_block_t {
    struct _suns_model_t *model;
    suns_block_def_t *block_def;
    uint16_t addr;
    uint16_t type;
    uint16_t index;
    suns_point_t *points;
    suns_point_t *points_sf;
} suns_block_t;

typedef struct _suns_model_t {
    struct _suns_device_t *device;
    uint16_t id;
    uint16_t len;
    uint16_t total_len;
    uint16_t index;
    uint16_t addr;
    uint16_t block_count;
    suns_model_def_t *model_def;
    struct _suns_model_t *next;
    suns_block_t *blocks[1];             /* array is sized during model allocation */
} suns_model_t;

typedef struct _suns_device_t {
    uint16_t base_addr;
    suns_modbus_io_t modbus_io;
    suns_model_t *models;
} suns_device_t;

#ifdef __cplusplus
extern "C" {
#endif

suns_point_t * suns_block_point_find(suns_block_t *block, char *id);
suns_err_t suns_model_add(suns_device_t *device, uint16_t id, uint16_t len, uint16_t addr, suns_model_t **model_ptr);
void suns_model_dump(suns_model_t *model, char *str);
void suns_model_free(suns_model_t *model);
suns_model_def_t * suns_model_def_get(uint16_t id);
void suns_point_def_dump(suns_point_def_t *point, char *str);
void suns_block_def_dump(suns_block_def_t *block, char *str);
void suns_model_def_dump(suns_model_def_t *model, char *str);
uint16_t suns_point_value_equals(suns_point_t *p1, suns_point_t *p2);
void suns_device_free_models(suns_device_t *device);
suns_err_t suns_device_modbus_read(suns_device_t *device, uint16_t addr, uint16_t len,
                                   unsigned char *buf, uint32_t timeout);
suns_err_t suns_device_modbus_write(suns_device_t *device, uint16_t addr, uint16_t len,
                                    unsigned char *buf, uint32_t timeout);
suns_err_t suns_block_write(suns_block_t *block);
void suns_block_clear_write(suns_block_t *block);
suns_err_t suns_model_update(suns_model_t *model, unsigned char *buf);
void suns_device_dump(suns_device_t *device, char *str);
suns_data_t * suns_data_type_find(const char *type);

#ifdef __cplusplus
}
#endif

#endif /* _SUNSPEC_DEVICE_H_ */
