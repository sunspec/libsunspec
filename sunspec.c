
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
#include <string.h>

#include "sunspec_device.h"
#include "sunspec_error.h"
#include "sunspec_cea2045.h"
#include "sunspec_modbus_rtu.h"
#include "sunspec_modbus_sim.h"
#include "sunspec_log.h"

#define SUNS_BASE_ADDR_LIST_LEN         3
uint16_t suns_base_addr_list[SUNS_BASE_ADDR_LIST_LEN] = {40000, 0, 50000};

#define SUNS_DEVICE_MAGIC_LEN           4
unsigned char suns_device_magic[SUNS_DEVICE_MAGIC_LEN] = {'S','u','n','S'};

suns_device_t *
suns_device_alloc()
{
    suns_device_t *device = (suns_device_t *) calloc(1, sizeof(suns_device_t));

    if (device != NULL) {
        device->base_addr = SUNS_BASE_ADDR_UNKNOWN;
        device->modbus_io.magic = SUNS_MODBUS_IO_MAGIC;
    }

    return device;
}

void
suns_device_free(suns_device_t *device)
{
    suns_device_free_models(device);
    free(device);
}

suns_err_t
suns_device_rtu_cea2045(suns_device_t *device, uint16_t slave_id)
{
    if (device == NULL) {
        return SUNS_ERR_INIT;
    }

    return suns_modbus_rtu_cea2045_open(&device->modbus_io, slave_id);
}

suns_err_t
suns_device_rtu_serial(suns_device_t *device, char *ifc_name, uint16_t slave_id,
                       uint32_t baudrate, uint8_t parity)
{
    return SUNS_ERR_UNIMPL;
}

suns_err_t
suns_device_sim(suns_device_t *device, uint16_t base_addr,
                uint16_t *sim_map, uint16_t sim_map_len, uint16_t slave_id)
{
    if (device == NULL) {
        return SUNS_ERR_INIT;
    }

    return suns_modbus_sim_open(&device->modbus_io, base_addr, sim_map, sim_map_len, slave_id);
}

suns_err_t
suns_device_scan(suns_device_t *device)
{
    suns_err_t err;
    unsigned char buf[SUNS_MODBUS_READ_MAX_LEN];
    uint16_t found = 0;
    uint16_t i;
    uint16_t end = 0;
    uint16_t addr;
    uint16_t model_id;
    uint16_t model_len;

    for (i = 0; i < SUNS_BASE_ADDR_LIST_LEN; i++) {
        err = suns_device_modbus_read(device, suns_base_addr_list[i], SUNS_DEVICE_MAGIC_LEN/2, buf, 0);
        if (err == SUNS_ERR_OK) {
            found = 1;
            if (memcmp(buf, suns_device_magic, SUNS_DEVICE_MAGIC_LEN) == 0) {
                device->base_addr = suns_base_addr_list[i];
                break;
            }
        }
    }

    if ((addr = device->base_addr) == SUNS_BASE_ADDR_UNKNOWN) {
        if (found) {
            return SUNS_ERR_NOT_SUNSPEC;
        } else {
            return SUNS_ERR_NOT_FOUND;
        }
    }

    addr += 2;
    while (!end) {
        err = suns_device_modbus_read(device, addr, 2, buf, 0);
        if (err != SUNS_ERR_OK) {
            /* check if no length field on termination block */
            if (err == SUNS_ERR_RANGE) {
            }
            goto error_exit;
        }

        model_id = suns_modbus_to_16(buf);
        model_len = suns_modbus_to_16(&buf[2]);

        if (model_id == SUNS_MODEL_ID_END) {
            break;
        }

        if ((model_len < 1) || (model_len > SUNS_MODEL_LEN_MAX)) {
            err = SUNS_ERR_RANGE;
            goto error_exit;
        }

        err = suns_model_add(device, model_id, model_len, addr + 2, NULL);
        if (err == SUNS_ERR_OK) {
            suns_log(SUNS_LOG_INFO, "Model %d added\n", model_id);
        } else {
            /* skip unknown models */
            if (err != SUNS_ERR_MODEL_DEF_NOT_FOUND) {
                suns_log(SUNS_LOG_WARN, "Model definition not found for model id %d\n", model_id);
            } else {
                suns_log(SUNS_LOG_ERR, "Model add error: %d\n", err);
                goto error_exit;
            }
        }

        addr += model_len + 2;

    }

    return SUNS_ERR_OK;

error_exit:

    /* free models on error */
    suns_device_free_models(device);

    return err;
}

suns_model_t *
suns_device_get_model(suns_device_t *device, uint16_t id, char *name, uint16_t index)
{
    suns_model_t *model = device->models;

    while (model) {
        if (model->index == index) {
            if (name) {
                if (strcmp(name, model->model_def->name) == 0) {
                     return model;
                }
            } else if (model->id == id) {
                return model;
            }
        }
        model = model->next;
    }

    return NULL;
}

suns_err_t
suns_model_read(suns_model_t *model)
{
    suns_err_t err;
    unsigned char buf[SUNS_MODEL_BUF_SIZE];

    if (model->device == NULL) {
        return SUNS_ERR_INIT;
    }

    if ((model->len * 2) > SUNS_MODEL_BUF_SIZE) {
        return SUNS_ERR_BUF_SIZE;
    }

    memset(buf, 0, SUNS_MODEL_BUF_SIZE);
    err = suns_device_modbus_read(model->device, model->addr, model->len, buf, 0);
    if (err != SUNS_ERR_OK) {
        return err;
    }

    err = suns_model_update(model, buf);

    return err;
}

suns_err_t
suns_model_write(suns_model_t *model)
{
    uint16_t i;
    suns_err_t err = SUNS_ERR_OK;

    if (model == NULL || model->device == NULL) {
        return SUNS_ERR_INIT;
    }

    /* if a write fails on a block, clear dirty/write indication for points in all blocks */
    for (i = 0; i < model->block_count; i++) {
        if (err == SUNS_ERR_OK) {
            err = suns_block_write(model->blocks[i]);
        } else {
            suns_block_clear_write(model->blocks[i]);
        }
    }

    return err;
}

suns_point_t *
suns_model_get_point(suns_model_t *model, char *id, uint16_t index)
{
    suns_point_t *point = NULL;

    if (index < model->block_count) {
        point = suns_block_point_find(model->blocks[index], id);
    }

    return point;
}

uint16_t
suns_point_is_implemented(suns_point_t *point)
{
    if (point) {
        return point->point_def->type->is_implemented(point->value_base);
    }
    return 0;
}

uint16_t
suns_model_point_is_implemented(suns_model_t *model, char *id, uint16_t index)
{
    return suns_point_is_implemented(suns_model_get_point(model, id, index));
}

suns_err_t
suns_point_get_str(suns_point_t *point, char **str)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->value_ptr == NULL) {
        return SUNS_ERR_TYPE;
    }
    *str = point->value_base.str;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_str(suns_point_t *point, char *str)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type != SUNS_TYPE_STR) {
        return SUNS_ERR_TYPE;
    }
    if ((strlen(str) + 1) > (point->point_def->len * 2)) {
        return SUNS_ERR_RANGE;
    }

    strncpy(point->value_base.str, str, point->point_def->len * 2);
    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_get_int16(suns_point_t *point, int16_t *value, int16_t *sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type != SUNS_TYPE_INT16) {
        return SUNS_ERR_TYPE;
    }
    *value = point->value_base.s16;
    if (sf) {
        if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
            *sf = point->sf_point->value_base.s16;
        } else {
            *sf = 0;
        }
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_int16(suns_point_t *point, int16_t value, int16_t sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }

    if (point->point_def->type->base_type != SUNS_TYPE_INT16) {
        return SUNS_ERR_TYPE;
    }

    if (point->sf_point && point->sf_point->value_base.s16 != sf) {
        return SUNS_ERR_SF;
    }

    point->value_base.s16 = value;
    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_get_uint16(suns_point_t *point, uint16_t *value, int16_t *sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type != SUNS_TYPE_UINT16) {
        return SUNS_ERR_TYPE;
    }
    *value = point->value_base.u16;
    if (sf) {
        if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
            *sf = point->sf_point->value_base.s16;
        } else {
            *sf = 0;
        }
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_uint16(suns_point_t *point, uint16_t value, int16_t sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }

    if (point->point_def->type->base_type != SUNS_TYPE_UINT16) {
        return SUNS_ERR_TYPE;
    }

    if (point->sf_point && point->sf_point->value_base.s16 != sf) {
        return SUNS_ERR_SF;
    }

    point->value_base.u16 = value;
    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_get_int32(suns_point_t *point, int32_t *value, int16_t *sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type != SUNS_TYPE_INT32) {
        return SUNS_ERR_TYPE;
    }
    *value = point->value_base.s32;
    if (sf) {
        if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
            *sf = point->sf_point->value_base.s16;
        } else {
            *sf = 0;
        }
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_int32(suns_point_t *point, int32_t value, int16_t sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }

    if (point->point_def->type->base_type != SUNS_TYPE_INT32) {
        return SUNS_ERR_TYPE;
    }

    if (point->sf_point && point->sf_point->value_base.s16 != sf) {
        return SUNS_ERR_SF;
    }

    point->value_base.s32 = value;
    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_get_uint32(suns_point_t *point, uint32_t *value, int16_t *sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type != SUNS_TYPE_UINT32) {
        return SUNS_ERR_TYPE;
    }
    *value = point->value_base.u32;
    if (sf) {
        if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
            *sf = point->sf_point->value_base.s16;
        } else {
            *sf = 0;
        }
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_uint32(suns_point_t *point, uint32_t value, int16_t sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }

    if (point->point_def->type->base_type != SUNS_TYPE_UINT32) {
        return SUNS_ERR_TYPE;
    }

    if (point->sf_point && point->sf_point->value_base.s16 != sf) {
        return SUNS_ERR_SF;
    }

    point->value_base.u32 = value;
    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_get_int64(suns_point_t *point, int64_t *value, int16_t *sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type != SUNS_TYPE_INT64) {
        return SUNS_ERR_TYPE;
    }
    *value = point->value_base.s64;
    if (sf) {
        if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
            *sf = point->sf_point->value_base.s16;
        } else {
            *sf = 0;
        }
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_int64(suns_point_t *point, int64_t value, int16_t sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }

    if (point->point_def->type->base_type != SUNS_TYPE_INT64) {
        return SUNS_ERR_TYPE;
    }

    if (point->sf_point && point->sf_point->value_base.s16 != sf) {
        return SUNS_ERR_SF;
    }

    point->value_base.s64 = value;
    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_get_uint64(suns_point_t *point, uint64_t *value, int16_t *sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type != SUNS_TYPE_UINT64) {
        return SUNS_ERR_TYPE;
    }
    *value = point->value_base.u64;
    if (sf) {
         if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
            *sf = point->sf_point->value_base.s16;
        } else {
            *sf = 0;
        }
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_uint64(suns_point_t *point, uint64_t value, int16_t sf)
{
    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }

    if (point->point_def->type->base_type != SUNS_TYPE_UINT32) {
        return SUNS_ERR_TYPE;
    }

    if (point->sf_point && point->sf_point->value_base.s16 != sf) {
        return SUNS_ERR_SF;
    }

    point->value_base.u64 = value;
    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_get_float32(suns_point_t *point, float *value)
{
    int16_t sf = 0;

    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }
    if (point->point_def->type->base_type == SUNS_TYPE_FLOAT32) {
        *value = point->value_base.f32;
    } else {
        if (point->point_def->type->to_float != NULL) {
            if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
                sf = point->sf_point->value_base.s16;
            }
            point->point_def->type->to_float(point->value_base, sf, value);
        } else {
            return SUNS_ERR_TYPE;
        }
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_point_set_float32(suns_point_t *point, float value)
{
    int16_t sf = 0;

    if (point == NULL) {
        return SUNS_ERR_NOT_FOUND;
    }

    if (point->point_def->type->base_type == SUNS_TYPE_FLOAT32) {
        point->value_base.f32 = value;
    } else {
        if (point->point_def->type->from_float != NULL) {
            if (point->sf_point && point->sf_point->point_def->type->is_implemented(point->sf_point->value_base)) {
                sf = point->sf_point->value_base.s16;
            }
            point->point_def->type->from_float(&point->value_base, sf, value);
        } else {
            return SUNS_ERR_TYPE;
        }
    }

    point->dirty = 1;

    return SUNS_ERR_OK;
}

suns_err_t
suns_model_point_get_str(suns_model_t *model, char *id, uint16_t index, char **str)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_str(point, str);
}

suns_err_t
suns_model_point_set_str(suns_model_t *model, char *id, uint16_t index, char *str)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_str(point, str);
}

suns_err_t
suns_model_point_get_int16(suns_model_t *model, char *id, uint16_t index, int16_t *value, int16_t *sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_int16(point, value, sf);
}

suns_err_t
suns_model_point_set_int16(suns_model_t *model, char *id, uint16_t index, int16_t value, int16_t sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_int16(point, value, sf);
}

suns_err_t
suns_model_point_get_uint16(suns_model_t *model, char *id, uint16_t index, uint16_t *value, int16_t *sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_uint16(point, value, sf);
}

suns_err_t
suns_model_point_set_uint16(suns_model_t *model, char *id, uint16_t index, uint16_t value, int16_t sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_uint16(point, value, sf);
}

suns_err_t
suns_model_point_get_int32(suns_model_t *model, char *id, uint16_t index, int32_t *value, int16_t *sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_int32(point, value, sf);
}

suns_err_t
suns_model_point_set_int32(suns_model_t *model, char *id, uint16_t index, int32_t value, int16_t sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_int32(point, value, sf);
}

suns_err_t
suns_model_point_get_uint32(suns_model_t *model, char *id, uint16_t index, uint32_t *value, int16_t *sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_uint32(point, value, sf);
}

suns_err_t
suns_model_point_set_uint32(suns_model_t *model, char *id, uint16_t index, uint32_t value, int16_t sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_uint32(point, value, sf);
}

suns_err_t
suns_model_point_get_int64(suns_model_t *model, char *id, uint16_t index, int64_t *value, int16_t *sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_int64(point, value, sf);
}

suns_err_t
suns_model_point_set_int64(suns_model_t *model, char *id, uint16_t index, int64_t value, int16_t sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_int64(point, value, sf);
}

suns_err_t
suns_model_point_get_uint64(suns_model_t *model, char *id, uint16_t index, uint64_t *value, int16_t *sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_uint64(point, value, sf);
}

suns_err_t
suns_model_point_set_uint64(suns_model_t *model, char *id, uint16_t index, uint64_t value, int16_t sf)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_uint64(point, value, sf);
}

suns_err_t
suns_model_point_get_float32(suns_model_t *model, char *id, uint16_t index, float *value)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_get_float32(point, value);
}

suns_err_t
suns_model_point_set_float32(suns_model_t *model, char *id, uint16_t index, float value)
{
    suns_point_t *point = suns_model_get_point(model, id, index);
    return suns_point_set_float32(point, value);
}
