
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
#include <stdint.h>
#include <string.h>

#include "ezxml.h"
#include "sunspec_config.h"
#include "sunspec_error.h"
#include "sunspec_device.h"
#include "sunspec_log.h"
#include "sunspec_modbus.h"
#include "sunspec_modbus_rtu.h"
#include "sunspec_modbus_sim.h"
#include "sunspec_value.h"

suns_model_def_t *suns_model_def_list = NULL;

suns_data_t suns_data_types[] = {
    {"int16", SUNS_TYPE_INT16, SUNS_TYPE_INT16, 1, 0, {.s16 = 0x8000},
              suns_value_int16_to_float, suns_value_float_to_int16, suns_value_int16_str, suns_value_int16_impl,
              suns_modbus_to_int16, suns_modbus_from_int16},
    {"uint16", SUNS_TYPE_UINT16, SUNS_TYPE_UINT16, 1, 0, {.u16 = 0xFFFF},
              suns_value_uint16_to_float, suns_value_float_to_uint16, suns_value_uint16_str, suns_value_uint16_impl,
              suns_modbus_to_uint16, suns_modbus_from_uint16},
    {"acc16", SUNS_TYPE_ACC16, SUNS_TYPE_UINT16, 1, 0, {.u16 = 0},
              suns_value_int16_to_float, suns_value_float_to_int16, suns_value_uint16_str, suns_value_impl,
              suns_modbus_to_uint16, suns_modbus_from_uint16},
    {"int32", SUNS_TYPE_INT32, SUNS_TYPE_INT32, 2, 0, {.s32 = 0x80000000},
              suns_value_int32_to_float, suns_value_float_to_int32, suns_value_int32_str, suns_value_int32_impl,
              suns_modbus_to_int32, suns_modbus_from_int32},
    {"uint32", SUNS_TYPE_UINT32, SUNS_TYPE_UINT32, 2, 0, {.u32 = 0xFFFFFFFF},
              suns_value_uint32_to_float, suns_value_float_to_uint32, suns_value_uint32_str, suns_value_uint32_impl,
              suns_modbus_to_uint32, suns_modbus_from_uint32},
    {"acc32", SUNS_TYPE_ACC32, SUNS_TYPE_UINT32, 2, 0, {.u32 = 0},
              suns_value_uint32_to_float, suns_value_float_to_uint32, suns_value_uint32_str, suns_value_impl,
              suns_modbus_to_uint32, suns_modbus_from_uint32},
    {"int64", SUNS_TYPE_INT64, SUNS_TYPE_INT64, 4, 0, {.s64 = 0x8000000000000000LL},
              suns_value_int64_to_float, suns_value_float_to_int64, suns_value_int64_str, suns_value_int64_impl,
              suns_modbus_to_int64, suns_modbus_from_int64},
    {"uint64", SUNS_TYPE_UINT64, SUNS_TYPE_UINT64, 4, 0, {.u64 = 0xFFFFFFFFFFFFFFFFLL},
              suns_value_uint64_to_float, suns_value_float_to_uint64, suns_value_uint64_str, suns_value_uint64_impl,
              suns_modbus_to_uint64, suns_modbus_from_uint64},
    {"acc64", SUNS_TYPE_ACC64, SUNS_TYPE_UINT64, 4, 0, {.u64 = 0},
              suns_value_uint64_to_float, suns_value_float_to_uint64, suns_value_uint64_str, suns_value_impl,
              suns_modbus_to_uint64, suns_modbus_from_uint64},
    {"float32", SUNS_TYPE_FLOAT32, SUNS_TYPE_FLOAT32, 2, 0, {.u32 = 0x7fc00000},
              suns_value_float_to_float, NULL, suns_value_float32_str, suns_value_float32_impl,
              suns_modbus_to_float, suns_modbus_from_float},
    {"string", SUNS_TYPE_STR, SUNS_TYPE_STR, 0, 0, {.u16 = 0},
              NULL, NULL, suns_value_str_str, suns_value_string_impl, suns_modbus_to_str, suns_modbus_from_str},
    {"sunssf", SUNS_TYPE_SUNSSF, SUNS_TYPE_INT16, 1, 0, {.s16 = 0x8000},
              suns_value_int16_to_float, suns_value_float_to_int16, suns_value_int16_str, suns_value_sunssf_impl,
              suns_modbus_to_int16, suns_modbus_from_int16},
    {"enum16", SUNS_TYPE_ENUM16, SUNS_TYPE_UINT16, 1, 0, {.u16 = 0xFFFF},
              suns_value_uint16_to_float, suns_value_float_to_uint16, suns_value_uint16_str, suns_value_enum16_impl,
              suns_modbus_to_uint16, suns_modbus_from_uint16},
    {"enum32", SUNS_TYPE_ENUM32, SUNS_TYPE_UINT32, 2, 0, {.u32 = 0xFFFFFFFF},
              suns_value_uint32_to_float, suns_value_float_to_uint32, suns_value_uint32_str, suns_value_enum32_impl,
              suns_modbus_to_uint32, suns_modbus_from_uint32},
    {"bitfield16", SUNS_TYPE_BIT16, SUNS_TYPE_UINT16, 1, 0, {.u16 = 0xFFFF},
              suns_value_uint16_to_float, suns_value_float_to_uint16, suns_value_uint16_str, suns_value_bitfield16_impl,
              suns_modbus_to_uint16, suns_modbus_from_uint16},
    {"bitfield32", SUNS_TYPE_BIT32, SUNS_TYPE_UINT32, 2, 0, {.u32 = 0xFFFFFFFF},
              suns_value_uint32_to_float, suns_value_float_to_uint32, suns_value_uint32_str, suns_value_bitfield32_impl,
              suns_modbus_to_uint32, suns_modbus_from_uint32},
    {"pad", SUNS_TYPE_PAD, SUNS_TYPE_INT16, 1, 1, {.s16 = 0x8000}, NULL, NULL, NULL, suns_value_impl, suns_modbus_to_int16, suns_modbus_from_int16},
    {"ipaddr", SUNS_TYPE_IPADDR, SUNS_TYPE_UINT32, 2, 0, {.u32 = 0}, suns_value_uint32_to_float, suns_value_float_to_uint32, suns_value_uint32_str, suns_value_ipaddr_impl,
              suns_modbus_to_uint32, suns_modbus_from_uint32},
    {"ipv6addr", SUNS_TYPE_IPV6ADDR, SUNS_TYPE_STR, 8, 0, {.u16 = 0}, NULL, NULL, suns_value_uint16_str, suns_value_ipv6addr_impl, suns_modbus_to_str, suns_modbus_from_str},
    {NULL},
};

void
suns_dump_buffer(uint16_t addr, uint16_t len, unsigned char *buf)
{
    uint16_t i;

    for (i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("\n%d: ", addr + i);
        }
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

suns_data_t *
suns_data_type_find(const char *type)
{
    uint16_t i;
    const char *id;

    for (i = 0; (id = suns_data_types[i].id) != NULL; i++) {
        if (strcmp(id, type) == 0) {
            return &suns_data_types[i];
        }
    }

    return NULL;
}

suns_model_def_t *
suns_model_def_find(suns_model_def_t *list, uint16_t id)
{
    suns_model_def_t *model = list;

    while (model) {
        if (model->id == id) {
            return model;
        }
        model = model->next;
    }

    return NULL;
}

suns_point_def_t *
suns_point_def_find(suns_point_def_t *list, const char *id)
{
    suns_point_def_t *point = list;

    while (point) {
        if (strcmp(point->id, id) == 0) {
            return point;
        }
        point = point->next;
    }

    return NULL;
}

void
suns_point_def_free(suns_point_def_t *point)
{
    if (point) {
        if (point->id) {
            free(point->id);
        }
        if (point->sf_name) {
            free(point->sf_name);
        }
        if (point->units) {
            free(point->units);
        }
        free(point);
    }
}

void
suns_block_def_free(suns_block_def_t *block)
{
    suns_point_def_t *point;
    suns_point_def_t *next;

    if (block) {
        point = block->points;
        while (point) {
            next = point->next;
            suns_point_def_free(point);
            point = next;
        }
        free(block);
    }
}

void
suns_model_def_free(suns_model_def_t *model)
{
    uint16_t i;

    if (model) {
        for (i = 0; i < SUNS_BLOCK_TYPE_COUNT; i++) {
            if (model->blocks[i]) {
                suns_block_def_free(model->blocks[i]);
            }
        }
        free(model);
    }
}

uint16_t
suns_point_def_add(suns_block_def_t *block, ezxml_t xml)
{
    uint16_t ret = 1;
    const char *id;
    uint16_t offset;
    uint16_t size = 0;
    uint16_t len = 0;
    const char *units = NULL;
    const char *sf;
    const char *sf_name = NULL;
    short sf_value = 0;
    const char *str;
    suns_data_t *data_type;
    suns_point_def_t *point = NULL;
    suns_point_def_t *ptr;
    // short access;
    // short required;

    id = ezxml_attr(xml, SUNS_ATTR_ID);
    if (id == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing point: id attribute missing\n");
        goto suns_point_def_add_exit;
    }

    str = ezxml_attr(xml, SUNS_ATTR_OFFSET);
    if (str == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing point %s: offset attribute missing\n", id);
        goto suns_point_def_add_exit;
    }

    if (sscanf(str, "%hu", &offset) != 1) {
        suns_log(SUNS_LOG_ERR, "Error parsing point %s: illegal offset value: %s\n", id, str);
        goto suns_point_def_add_exit;
    }

    str = ezxml_attr(xml, SUNS_ATTR_SIZE);
    if (str) {
        if (sscanf(str, "%hu", &size) != 1) {
            suns_log(SUNS_LOG_ERR, "Error parsing point %s: illegal size value: %s\n", id, str);
            goto suns_point_def_add_exit;
        }
    }

    str = ezxml_attr(xml, SUNS_ATTR_LEN);
    if (str) {
        if (sscanf(str, "%hu", &size) != 1) {
            suns_log(SUNS_LOG_ERR, "Error parsing point %s: illegal len value: %s\n", id, str);
            goto suns_point_def_add_exit;
        }
    }

    str = ezxml_attr(xml, SUNS_ATTR_TYPE);
    if (str == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing point %s: type attribute missing\n", id);
        goto suns_point_def_add_exit;
    }

    if ((data_type = suns_data_type_find(str)) == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing point %s: unknown data type: %s\n", id, str);
        goto suns_point_def_add_exit;
    }
    len = data_type->len;

    if (data_type->type == SUNS_TYPE_STR) {
        if (size == 0) {
            suns_log(SUNS_LOG_ERR, "Error parsing point %s: size or len required for string type\n", id);
            goto suns_point_def_add_exit;
        }
        len = size;
    }

    sf = ezxml_attr(xml, SUNS_ATTR_SF);
    if (sf != NULL) {
        if (sscanf(sf, "%hu", &sf_value) != 1) {
            sf_name = sf;
        }
    }

    units = ezxml_attr(xml, SUNS_ATTR_UNITS);

    if ((point = (suns_point_def_t *) calloc(1, sizeof(suns_point_def_t)))) {
        /* id */
        if ((point->id = calloc(1, strlen(id) + 1))) {
            strncpy(point->id, id, strlen(id) + 1);
        } else {
            suns_log(SUNS_LOG_ERR, "suns_point_def_add: memory allocation error\n");
            goto suns_point_def_add_exit;
        }
        /* sf_name */
        if (sf_name) {
            if ((point->sf_name = calloc(1, strlen(sf_name) + 1))) {
                strncpy(point->sf_name, sf_name, strlen(sf_name) + 1);
            } else {
                suns_log(SUNS_LOG_ERR, "suns_point_def_add: memory allocation error\n");
                goto suns_point_def_add_exit;
            }
        }
        /* units */
        if (units) {
            if ((point->units = calloc(1, strlen(units) + 1))) {
                strncpy(point->units, units, strlen(units) + 1);
            } else {
                suns_log(SUNS_LOG_ERR, "suns_point_def_add: memory allocation error\n");
                goto suns_point_def_add_exit;
            }
        }
    } else {
        suns_log(SUNS_LOG_ERR, "suns_point_def_add: memory allocation error\n");
        goto suns_point_def_add_exit;
    }

    point->offset = offset;
    point->type = data_type;
    point->len = len;
    point->required = 0;
    point->access = 0;
    point->sf_value = sf_value;

    /* put at end of point list */
    if ((ptr = block->points)) {
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        ptr->next = point;
    } else {
        block->points = point;
    }

    point->next = NULL;
    ret = 0;

suns_point_def_add_exit:

    if (ret != 0) {
        if (point) {
            suns_point_def_free(point);
        }
    }

    return ret;
}

uint16_t
suns_block_def_add(suns_model_def_t *model, ezxml_t xml)
{
    uint16_t ret = 1;
    ezxml_t e;
    // const char *error;
    const char *tag;
    const char *str;
    uint16_t len;
    uint8_t repeating = 0;
    uint16_t block_index = SUNS_BLOCK_FIXED;
    suns_block_def_t *block = NULL;
    suns_block_def_t *model_block;
    suns_point_def_t *point;
    suns_point_def_t *sf_point;

    str = ezxml_attr(xml, SUNS_ATTR_LEN);
    if (str == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing block: len attribute missing\n");
        goto suns_block_def_add_exit;
    }

    if (sscanf(str, "%hu", &len) != 1) {
        suns_log(SUNS_LOG_ERR, "Error parsing block: illegal len value: %s\n", str);
        goto suns_block_def_add_exit;
    }

    str = ezxml_attr(xml, SUNS_ATTR_TYPE);
    if (str != NULL) {
        if (strcmp(str, SUNS_ATTR_VALUE_REPEAT) == 0) {
            repeating = 1;
            block_index = SUNS_BLOCK_REPEATING;
        }
    }

    if ((block = (suns_block_def_t *) calloc(1, sizeof(suns_block_def_t))) == NULL) {
        suns_log(SUNS_LOG_ERR, "suns_block_def_add: memory allocation error\n");
        goto suns_block_def_add_exit;
    }

    block->len = len;
    block->repeating = repeating;

    /* add each point */
    e = ezxml_child(xml, SUNS_TAG_POINT);
    while (e) {
        tag = ezxml_name(e);
        if (strncmp(tag, SUNS_TAG_POINT, sizeof(SUNS_TAG_POINT)) == 0) {

            /* abort processing on error */
            if (suns_point_def_add(block, e)) {
                goto suns_block_def_add_exit;
            }
        } else {
            suns_log(SUNS_LOG_ERR, "Error parsing model: unexpected tag %s at point level\n", tag);
            goto suns_block_def_add_exit;
        }
        e = ezxml_next(e);
    }

    /* resolve sf names */
    point = block->points;
    while (point) {
        if (point->sf_name) {
            /* check in same block */
            if ((sf_point = suns_point_def_find(block->points, point->sf_name))) {
                point->sf_offset = sf_point->offset;
            } else {
                /* should be in fixed block of model */
                if ((model_block = model->blocks[SUNS_BLOCK_FIXED]) &&
                    (sf_point = suns_point_def_find(model_block->points, point->sf_name))) {
                    point->sf_offset = sf_point->offset;
                } else {
                    suns_log(SUNS_LOG_ERR, "Error parsing model %d: could not resolve sf name %s - %s\n",
                         model->id, point->id, point->sf_name);
                    goto suns_block_def_add_exit;
                }
            }
        }
        point = point->next;
    }

    model->blocks[block_index] = block;

    ret = 0;

suns_block_def_add_exit:

    if (ret != 0) {
        if (block) {
            suns_block_def_free(block);
        }
    }

    return ret;
}

uint16_t
suns_model_def_add(suns_model_def_t **list, ezxml_t xml)
{
    uint16_t ret = 1;
    ezxml_t e;
    const char *tag;
    const char *attr;
    const char *name;
    uint16_t id;
    uint16_t len;
    suns_model_def_t *model = NULL;
    suns_model_def_t *next;

    attr = ezxml_attr(xml, SUNS_ATTR_ID);
    if (attr == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing model: id attribute missing\n");
        ret = 1;
        goto suns_model_def_add_exit;
    }

    if (sscanf(attr, "%hu", &id) != 1) {
        suns_log(SUNS_LOG_ERR, "Error parsing block: illegal id value: %s\n", attr);
        goto suns_model_def_add_exit;
    }

    /* error if model already on the list */
    if (suns_model_def_find(*list, id) != NULL) {
        suns_log(SUNS_LOG_ERR, "Error laoding model: duplicate model %hu\n", id);
        goto suns_model_def_add_exit;
    }

    attr = ezxml_attr(xml, SUNS_ATTR_LEN);
    if (attr == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing block: len attribute missing\n");
        goto suns_model_def_add_exit;
    }

    if (sscanf(attr, "%hu", &len) != 1) {
        suns_log(SUNS_LOG_ERR, "Error parsing block: illegal len value: %s\n", attr);
        goto suns_model_def_add_exit;
    }

    name = ezxml_attr(xml, SUNS_ATTR_NAME);

    if ((model = (suns_model_def_t *) calloc(1, sizeof(suns_model_def_t))) == NULL) {
        suns_log(SUNS_LOG_ERR, "suns_model_def_add: memory allocation error\n");
        goto suns_model_def_add_exit;
    }

    model->id = id;
    model->len = len;
    snprintf(model->id_str, SUNS_MODEL_ID_LEN, "%hu", id);
    if (name != NULL) {
        strncpy(model->name, name, SUNS_MODEL_NAME_LEN);
    }

    /* add each block */
    e = ezxml_child(xml, SUNS_TAG_BLOCK);
    while (e) {
        tag = ezxml_name(e);
        if (strncmp(tag, SUNS_TAG_BLOCK, sizeof(SUNS_TAG_BLOCK)) == 0) {
            /* abort processing on error */
            if (suns_block_def_add(model, e)) {
                ret = 1;
                goto suns_model_def_add_exit;
            }
        } else {
            suns_log(SUNS_LOG_ERR, "Error parsing model: unexpected tag %s at block level\n", tag);
            goto suns_model_def_add_exit;
        }
        e = ezxml_next(e);
    }

    /* add model */
    next = *list;
    *list = model;
    model->next = next;

    ret = 0;

suns_model_def_add_exit:

    if (ret != 0) {
        if (model) {
            suns_model_def_free(model);
        }
    }

    return ret;
}

void
suns_model_load(const char *file_path, suns_model_def_t **list)
{
    ezxml_t xml;
    ezxml_t e;
    const char *error;
    const char *tag;

    suns_log(SUNS_LOG_INFO, "Loading SunSpec model definition file: %s", file_path);

    if ((xml = ezxml_parse_file(file_path)) == NULL) {
        suns_log(SUNS_LOG_ERR, "Error parsing %s\n", file_path);
        goto suns_models_load_exit;
    }

    if (strlen((error = ezxml_error(xml))) > 0) {
        suns_log(SUNS_LOG_ERR, "Error parsing model file %s: %s\n", file_path, error);
        goto suns_models_load_exit;
    }

    tag = ezxml_name(xml);
    if (strncmp(tag, SUNS_TAG_MODELS_ROOT, sizeof(SUNS_TAG_MODELS_ROOT)) == 0) {
        e = ezxml_child(xml, SUNS_TAG_MODEL);
        while (e) {
            tag = ezxml_name(e);
            if (strncmp(tag, SUNS_TAG_MODEL, sizeof(SUNS_TAG_MODEL)) == 0) {
                /* abort processing on error */
                if (suns_model_def_add(list, e)) {
                    break;
                }
            } else {
                suns_log(SUNS_LOG_ERR, "Error parsing model file %s: unexpected tag %s at model level\n", file_path, tag);
                break;
            }
            e = ezxml_next(e);
        }
    } else if (strncmp(tag, SUNS_TAG_MODEL, sizeof(SUNS_TAG_MODEL)) == 0) {
        suns_model_def_add(list, xml);
    } else {
        suns_log(SUNS_LOG_ERR, "Error parsing %s: unknown tag %s\n", file_path, tag);
    }

suns_models_load_exit:

    if (xml) {
        free(xml);
    }
}

suns_model_def_t *
suns_model_def_get(uint16_t id)
{
    suns_model_def_t *list = suns_model_def_list;
    suns_model_def_t *model_def = suns_model_def_find(list, id);
    char file_path[SUNS_MODEL_PATH_LEN];

    if (model_def == NULL) {
        if (strlen(SUNS_SMDX_PATH) > 0) {
            snprintf(file_path, SUNS_MODEL_PATH_LEN, "%ssmdx_%05d.xml", SUNS_SMDX_PATH, id);
            suns_model_load(file_path, &list);
            model_def = suns_model_def_find(list, id);
        }
    }

    return model_def;
}

void
suns_point_def_dump(suns_point_def_t *point, char *str)
{
    printf("%s", str);
    printf("%s  %d  %d  %s  %s  %d  %d- next: %p\n",
           point->id, point->offset, point->len, point->type->id, point->sf_name,
           point->sf_value, point->sf_offset, point->next);
}

void
suns_block_def_dump(suns_block_def_t *block, char *str)
{
    suns_point_def_t *point;

    printf("%s", str);
    if (block) {
        printf("block %d  %d:\n", block->len, block->repeating);
        point = block->points;
        while (point) {
            suns_point_def_dump(point, "");
            point = point->next;
        }
    }
}

void
suns_model_def_dump(suns_model_def_t *model, char *str)
{
    uint16_t i;

    printf("%s", str);
    if (model) {
        printf("model; %d (%d):\n", model->id, model->len);
        for (i = 0; i < SUNS_BLOCK_TYPE_COUNT; i++) {
            suns_block_def_dump(model->blocks[i], "");
        }
    }
}

void
suns_point_free(suns_point_t *point)
{
    if (point->value_ptr) {
        free(point->value_ptr);
    }
    free(point);
}

void
suns_block_free(suns_block_t *block)
{
    suns_point_t *point;
    suns_point_t *next;

    if (block) {
        point = block->points;
        while (point) {
            next = point->next;
            suns_point_free(point);
            point = next;
        }
        free(block);
    }
}

void
suns_model_free(suns_model_t *model)
{
    uint16_t i;

    if (model) {
        for (i = 0; i < model->block_count; i++) {
            if (model->blocks[i]) {
                suns_block_free(model->blocks[i]);
            }
        }
        free(model);
    }
}

suns_point_t *
suns_block_point_find(suns_block_t *block, char *id)
{
    suns_point_t *point;

    if (block) {
        point = block->points;
        while (point) {
            if (strcmp(point->point_def->id, id) == 0) {
                return point;
            }
            point = point->next;
        }
    }

    return NULL;
}

suns_err_t
suns_point_add(suns_block_t *block, suns_point_def_t *point_def, uint16_t block_addr)
{
    suns_err_t err;
    suns_point_t *point;
    suns_point_t *last;

    if ((point = (suns_point_t *) calloc(1, sizeof(suns_point_t))) == NULL) {
        return SUNS_ERR_ALLOC;
    }

    if ((point_def->type->type == SUNS_TYPE_STR) || (point_def->type->type == SUNS_TYPE_IPV6ADDR)) {
        if ((point->value_ptr = calloc(1, point_def->len * 2)) == NULL) {
            err = SUNS_ERR_ALLOC;
            goto error_exit;
        }
        point->value_base.str = point->value_ptr;
    }

    point->block = block;
    point->point_def = point_def;
    point->addr = block_addr + point_def->offset;
    point->dirty = 1;
    if(point->point_def->type->has_unimpl_value) {
        point->value_base = point->point_def->type->unimpl_value;
    }

    if (block->points == NULL) {
        block->points = point;
    } else {
        last = block->points;
        while (last->next) {
            last = last->next;
        }
        last->next = point;
    }

    return SUNS_ERR_OK;

error_exit:

    if (point) {
        suns_point_free(point);
    }

    return err;
}

uint16_t
suns_point_value_equals(suns_point_t *p1, suns_point_t *p2)
{
    if ((p1->point_def->type->type != p2->point_def->type->type) ||
        (strcmp(p1->point_def->id, p2->point_def->id) != 0) ||
        ((p1->sf_point == NULL) && p2->sf_point) ||
        (p1->sf_point && (p2->sf_point == NULL)) ||
        (p1->sf_point && !suns_point_value_equals(p1->sf_point, p2->sf_point))) {
        return 0;
    }

    switch (p1->point_def->type->type) {
        case SUNS_TYPE_INT16:
        case SUNS_TYPE_UINT16:
        case SUNS_TYPE_ACC16:
        case SUNS_TYPE_ENUM16:
        case SUNS_TYPE_SUNSSF:
        case SUNS_TYPE_BIT16:
        case SUNS_TYPE_PAD:
            return (p1->value_base.u16 == p2->value_base.u16);
        case SUNS_TYPE_INT32:
        case SUNS_TYPE_UINT32:
        case SUNS_TYPE_ACC32:
        case SUNS_TYPE_FLOAT32:
        case SUNS_TYPE_ENUM32:
        case SUNS_TYPE_BIT32:
        case SUNS_TYPE_IPADDR:
            return (p1->value_base.u32 == p2->value_base.u32);
        case SUNS_TYPE_INT64:
        case SUNS_TYPE_UINT64:
        case SUNS_TYPE_ACC64:
            return (p1->value_base.u64 == p2->value_base.u64);
        case SUNS_TYPE_STR:
        case SUNS_TYPE_IPV6ADDR:
            if (p1->point_def->len != p2->point_def->len) {
                return 0;
            }
            return (memcmp(p1->value_base.str, p2->value_base.str, p1->point_def->len * 2) == 0);
    }

    return 0;
}

uint16_t
suns_block_value_equals(suns_block_t *b1, suns_block_t *b2)
{
    suns_point_t *p1 = b1->points;
    suns_point_t *p2 = b2->points;

    while (p1) {
        if ((p2 == NULL) || !suns_point_value_equals(p1, p2)) {
            return 0;
        }
        p1 = p1->next;
        p2 = p2->next;
    }

    if (p2 != NULL) {
        return 0;
    }

    return 1;
}

uint16_t
suns_model_value_equals(suns_model_t *m1, suns_model_t *m2)
{
    uint16_t i;

    if (m1->block_count != m2->block_count) {
        return 0;
    }

    for (i = 0; i < m1->block_count; i++) {
        if (!suns_block_value_equals(m1->blocks[i], m2->blocks[i])) {
            return 0;
        }
    }

    return 1;
}

uint16_t
suns_device_value_equals(suns_device_t *d1, suns_device_t *d2)
{
    suns_model_t *m1 = d1->models;
    suns_model_t *m2 = d2->models;

    while (m1) {
        if ((m2 == NULL) || !suns_model_value_equals(m1, m2)) {
            return 0;
        }
        m1 = m1->next;
        m2 = m2->next;
    }

    if (m2 != NULL) {
        return 0;
    }

    return 1;
}

suns_err_t
suns_block_add(suns_model_t *model, uint16_t index, suns_block_def_t *block_def, uint16_t addr)
{
    suns_err_t err;
    suns_block_t *block;
    suns_point_def_t *point_def;
    suns_point_t *point;

    if ((block = (suns_block_t *) calloc(1, sizeof(suns_block_t))) == NULL) {
        return SUNS_ERR_ALLOC;
    }
    block->model = model;
    block->block_def = block_def;
    block->addr = addr;
    block->index = index;

    point_def = block_def->points;

    while (point_def) {
        err = suns_point_add(block, point_def, addr);
        if (err != SUNS_ERR_OK) {
            goto error_exit;
        }
        point_def = point_def->next;
    }

    /* resolve point scale factors */
    point = block->points;
    while (point) {
        if (point->point_def->sf_name) {
            point->sf_point = suns_block_point_find(block, point->point_def->sf_name);
            /* if scale factor not found in repeating block, check fixed block */
            if ((point->sf_point == NULL) && (block->index > 0)) {
                point->sf_point = suns_block_point_find(model->blocks[0], point->point_def->sf_name);
            }
            if (point->sf_point == NULL) {
                err = SUNS_ERR_SF_RESOLVE;
                goto error_exit;
            }
        }
        point = point->next;
    }

    model->blocks[index] = block;

    return SUNS_ERR_OK;

error_exit:

    if (block) {
        suns_block_free(block);
    }

    return err;
}

suns_err_t
suns_model_add(suns_device_t *device, uint16_t id, uint16_t len, uint16_t addr, suns_model_t **model_ptr)
{
    suns_err_t err;
    suns_model_def_t *model_def;
    suns_block_def_t *fixed;
    suns_block_def_t *repeating;
    suns_model_t *model;
    suns_model_t **model_list = &device->models;
    uint16_t fixed_len = 0;
    uint16_t repeating_len = 0;
    uint16_t total_repeating_len = 0;
    uint16_t repeating_count = 0;
    uint16_t i;

    if (model_ptr) {
        *model_ptr = NULL;
    }

    if ((model_def = suns_model_def_get(id)) == NULL) {
        return SUNS_ERR_MODEL_DEF_NOT_FOUND;
    }

    if ((fixed = model_def->blocks[SUNS_BLOCK_FIXED]) != NULL) {
        fixed_len = fixed->len;
    }
    if ((repeating = model_def->blocks[SUNS_BLOCK_REPEATING]) != NULL) {
        repeating_len = repeating->len;
    }

    if ((fixed_len > 0) && (len < fixed_len)) {
        /* allow for legacy common model with no filler */
        if ((id != 1) || (len != fixed_len - 1)) {
            return SUNS_ERR_MODEL_LEN;
        } else {
            fixed_len -= 1;
        }
    }

    total_repeating_len = len - fixed_len;
    if (repeating_len) {
        if ((total_repeating_len % repeating_len) != 0) {
            return SUNS_ERR_MODEL_LEN;
        }
        repeating_count = total_repeating_len/repeating_len;
    }

    model = (suns_model_t *) calloc(1, (sizeof(suns_model_t) + (sizeof(suns_block_t *) * repeating_count)));

    model->device = device;
    model->id = id;
    model->len = len;
    model->total_len = fixed_len + repeating_count * repeating_len;
    model->index = 1;
    model->addr = addr;
    model->block_count = repeating_count + 1;
    model->model_def = model_def;

    if (fixed) {
        if ((err= suns_block_add(model, 0, fixed, addr)) != SUNS_ERR_OK) {
            goto error_exit;
        }
    }

    addr += fixed_len;
    for (i = 1; i <= repeating_count; i++) {
        if ((err= suns_block_add(model, i, repeating, addr)) != SUNS_ERR_OK) {
            goto error_exit;
        }
        addr += repeating_len;
    }

    while (*model_list != NULL) {
        /* adjust index if multiple models with model id */
        if ((*model_list)->id == model->id) {
            model->index++;
        }
        model_list = &((*model_list)->next);
    }
    *model_list = model;
    model->next = NULL;
    if (model_ptr) {
        *model_ptr = model;
    }

    return SUNS_ERR_OK;

error_exit:

    if (model) {
        suns_model_free(model);
    }

    return err;
}

suns_err_t
suns_block_update(suns_block_t *block, unsigned char *buf)
{
    suns_point_t *point = block->points;

    /* printf("block update: model %d  block %d  buf %p\n", block->model->id, block->index, buf); */

    while (point) {
        if (point->point_def->type->modbus_to_value) {
            point->point_def->type->modbus_to_value(buf + (point->point_def->offset * 2), &point->value_base, point->point_def->len);
        }
        point->dirty = 0;
        point = point->next;
    }

    return SUNS_ERR_OK;
}

suns_err_t
suns_model_update(suns_model_t *model, unsigned char *buf)
{
    suns_err_t err;
    uint16_t i;
    uint16_t offset = 0;

    for (i = 0; i < model->block_count; i++) {
        if ((err = suns_block_update(model->blocks[i], buf + offset)) != SUNS_ERR_OK) {
            return err;
        }
        offset += (model->blocks[i]->block_def->len * 2);
    }

    return SUNS_ERR_OK;
}

suns_point_t *
suns_model_point_find(suns_model_t *model, char *id, uint16_t index)
{
    suns_point_t *point = NULL;

    if (index < model->block_count) {
        point = suns_block_point_find(model->blocks[index], id);
    }

    return point;
}

suns_err_t
suns_block_write(suns_block_t *block)
{
    suns_err_t err = SUNS_ERR_OK;
    unsigned char buf[SUNS_MODEL_BUF_SIZE];
    uint16_t index = 0;
    uint16_t addr = 0;
    suns_point_t *point = block->points;

    while (point) {
        if (point->dirty) {
            point->dirty = 0;
            if (err == SUNS_ERR_OK) {
                if (index == 0) {
                    addr = point->addr;
                }
                point->point_def->type->modbus_from_value(&buf[index], point->value_base, point->point_def->len);
                index += point->point_def->len * 2;
            }
        } else {
            if (index > 0) {
                err = suns_device_modbus_write(block->model->device, addr, index/2, buf, 0);
                index = 0;
            }
        }
        point = point->next;
    }

    if (index > 0) {
        err = suns_device_modbus_write(block->model->device, addr, index/2, buf, 0);
    }

    return err;
}

void
suns_block_clear_write(suns_block_t *block)
{
    suns_point_t *point = block->points;

    while (point) {
        point->dirty = 0;
        point = point->next;
    }
}

void
suns_point_dump(suns_point_t *point, char *str)
{
    char *sf_id = "";
    char value_str[80];
    int16_t sf = 0;

    printf("%s", str);

    if (point->sf_point) {
        sf_id = point->sf_point->point_def->id;
        sf = point->sf_point->value_base.s16;
    }
    printf("\t\t%s  %d  %s", point->point_def->id, point->addr, sf_id);
    // printf("\nis_implemented = %d\n", point->point_def->type->is_implemented(point->value_base));
    // if (point->point_def->type->is_implemented(point->value_base) && point->point_def->type->to_str) {
    if (point->point_def->type->is_implemented(point->value_base) && point->point_def->type->to_str) {
        point->point_def->type->to_str(point->value_base, sf, value_str, 80);
        printf(": %s\n", value_str);
    } else {
        printf("\n");
    }
}

void
suns_block_dump(suns_block_t *block, char *str)
{
    suns_point_t *point;

    printf("%s", str);

    if (block) {
        printf("\tblock %d %d %d:\n", block->index, block->addr, block->block_def->len);
    }

    point = block->points;
    while (point) {
        suns_point_dump(point, "");
        point = point->next;
    }
}

void
suns_model_dump(suns_model_t *model, char *str)
{
    uint16_t i;

    printf("%s", str);

    if (model) {
        printf("model: %d (%d):\n", model->id, model->len);
        for (i = 0; i < model->block_count; i++) {
            suns_block_dump(model->blocks[i], "");
        }
    }
}

void
suns_device_free_models(suns_device_t *device)
{
    suns_model_t *model;
    suns_model_t *next;

    model = device->models;
    while (model) {
        next = model->next;
        suns_model_free(model);
        model = next;
    }
}

void
suns_device_dump(suns_device_t *device, char *str)
{
    suns_model_t *model;

    printf("%s", str);

    model = device->models;
    while (model) {
        suns_model_dump(model, "");
        model = model->next;
    }
}

suns_err_t
suns_device_modbus_read(suns_device_t *device, uint16_t addr, uint16_t len, unsigned char *buf, uint32_t timeout)
{
    suns_err_t err = SUNS_ERR_INIT;

    if (device && device->modbus_io.read && device->modbus_io.prot) {
        err = (device->modbus_io.read)(device->modbus_io.prot, addr, len, buf, timeout);
    }

    return err;
}

suns_err_t
suns_device_modbus_write(suns_device_t *device, uint16_t addr, uint16_t len, unsigned char *buf, uint32_t timeout)
{
    suns_err_t err = SUNS_ERR_INIT;

    /* printf("suns_device_modbus_write: %p %d %d %p %d\n", device, addr, len, buf, timeout); */
    if (device && device->modbus_io.write && device->modbus_io.prot) {
        err = (device->modbus_io.write)(device->modbus_io.prot, addr, len, buf, timeout);
    }

    return err;
}



