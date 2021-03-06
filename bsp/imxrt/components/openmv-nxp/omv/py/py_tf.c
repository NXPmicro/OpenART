/* This file is part of the OpenMV project.
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io> & Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 */

#include <mp.h>
#include "py_helper.h"
#include "py_image.h"
#include "ff_wrapper.h"
#include "libtf.h"
#include "libtf_person_detect_model_data.h"

typedef struct HookOp_struct {
	uint32_t opCode;
	const char *pszName;
}HookOp_t;
#define CFG_TS_CNT	512
#define CFG_OPC_CNT	48
typedef struct tagOpProfile {
	uint32_t usTime;
	uint32_t x10000Rate;
	uint16_t ndx;
	uint16_t opCode;
	char szOpName[32];
}OpProfile_t;

OpProfile_t g_opPfs[CFG_TS_CNT], g_opcPfs[CFG_OPC_CNT];
volatile uint32_t g_tsNdx=0;
volatile uint32_t g_usTotal=0;
volatile uint32_t g_ts0=0;

static volatile int g_tf_profiling_en = 0;

static uint32_t rt_tick_get_us(){
	uint32_t tick = rt_tick_get();
	uint32_t us = (SysTick->LOAD - SysTick->VAL) / (SystemCoreClock/1000000);
	us += tick * 1000;
	return us;
}

uint32_t TFLm_HookBeforeInvoke(HookOp_t *pCtx) {
    if (!g_tf_profiling_en) return 0;

	g_ts0 = rt_tick_get_us();
	if (g_tsNdx < CFG_TS_CNT) {
		OpProfile_t *p = g_opPfs + g_tsNdx;
		p->ndx = (uint16_t) g_tsNdx;
		if (strlen(pCtx->pszName) < 32) {
			strcpy(p->szOpName, pCtx->pszName);
		} else {
			memcpy(p->szOpName, pCtx->pszName, 31);
			p->szOpName[31] = 0;
		}
		p->opCode = (uint16_t) pCtx->opCode;
	}		
	return 0;
}

uint32_t TFLm_HookAfterInvoke(HookOp_t *pCtx) {
    if (!g_tf_profiling_en) return 0;
	uint32_t dt = rt_tick_get_us() - g_ts0;
	if (g_tsNdx < CFG_TS_CNT) {
		OpProfile_t *p = g_opPfs + g_tsNdx;
		p->usTime = dt;
		g_tsNdx++;
	}
	g_usTotal += dt;	
	return 0;
}

static void SortDescending(OpProfile_t *p, uint32_t cnt) {
	int i, j;
	OpProfile_t tmp;
	for (i=0; i<cnt; i++) {
		for (j=i+1; j<cnt; j++) {
			if (p[i].usTime < p[j].usTime) {
				tmp = p[i];
				p[i] = p[j];
				p[j] = tmp;
			}
		}
	}
}

void ShowProfiling(void)
{
	OpProfile_t *p;
	mp_printf(&mp_plat_print, "\r\n--------------------------------------------------");
	mp_printf(&mp_plat_print, "\r\n-------------------Profiling----------------------");
	mp_printf(&mp_plat_print, "\r\n--------------------------------------------------\r\n");
	mp_printf(&mp_plat_print, "Total inference time: %d.%03dms\r\n", g_usTotal/1000, g_usTotal % 1000);
	
	for (int i=0; i<g_tsNdx; i++) {
		g_opPfs[i].x10000Rate = (uint32_t)((uint64_t)g_opPfs[i].usTime * 10000 / g_usTotal);
	}
	
	// SortDescending(g_opPfs, g_tsNdx);
	uint32_t usAcc = 0, pcntAcc = 0;
	mp_printf(&mp_plat_print, "odr, ndx, time ms,   unit%%, total%%\r\n");
	for (int i=0; i<g_tsNdx; i++) {
		// g_opPfs[i].x10000Rate = (uint32_t)((uint64_t)g_opPfs[i].usTime * 10000 / g_usTotal);
		usAcc += g_opPfs[i].usTime;
		pcntAcc = (uint64_t)usAcc * 10000 / g_usTotal;
		mp_printf(&mp_plat_print, "%03d, %03d, %03d.%03dms, %02d.%02d%%, %02d.%02d%%, %s\r\n", 
			i + 1, g_opPfs[i].ndx, 
			g_opPfs[i].usTime / 1000, g_opPfs[i].usTime % 1000, 
			g_opPfs[i].x10000Rate / 100, g_opPfs[i].x10000Rate % 100,
			pcntAcc / 100, pcntAcc % 100, g_opPfs[i].szOpName);
	}
	// calculate by operator type 
	mp_printf(&mp_plat_print, "\r\n--------------------------------------------------");
	mp_printf(&mp_plat_print, "\r\n                  per operator                    ");
	mp_printf(&mp_plat_print, "\r\n--------------------------------------------------\r\n");
	mp_printf(&mp_plat_print, "Total inference time: %d.%03dms\r\n", g_usTotal/1000, g_usTotal % 1000);
	{
		int opCodeNdx, i, opCodeTypeCnt = 0;
		const char *pszName;
		uint32_t pcnt;
		usAcc = 0;
		pcntAcc = 0;
		for (opCodeNdx=0; opCodeNdx < 256; opCodeNdx++) {
			uint32_t opcUs = 0;
			uint32_t opInstanceCnt = 0;
			for (i=0; i<g_tsNdx; i++) {
				if (g_opPfs[i].opCode != opCodeNdx) {
					continue;
				}
				pszName = g_opPfs[i].szOpName;
				opcUs += g_opPfs[i].usTime;
				opInstanceCnt++;
			}
			if (0 == opcUs) 
				continue;
			if (opCodeTypeCnt >= CFG_OPC_CNT) {
				continue;
			}
			g_opcPfs[opCodeTypeCnt].ndx = opInstanceCnt;
			pcnt = (uint64_t)opcUs * 10000 / g_usTotal;
			g_opcPfs[opCodeTypeCnt].x10000Rate = pcnt;
			g_opcPfs[opCodeTypeCnt].usTime = opcUs;
			g_opcPfs[opCodeTypeCnt].opCode = opCodeNdx;
			if (strlen(pszName) < 32) {
				strcpy(g_opcPfs[opCodeTypeCnt].szOpName, pszName);
			} else {
				memcpy(g_opcPfs[opCodeTypeCnt].szOpName, pszName, 31);
				g_opcPfs[opCodeTypeCnt].szOpName[31] = 0;
			}
			opCodeTypeCnt++;
		}
		SortDescending(g_opcPfs, opCodeTypeCnt);
		mp_printf(&mp_plat_print, "odr, opc, time ms,   unit%%, total%%,count,name\r\n");
		for (int i=0; i < opCodeTypeCnt; i++) {
			usAcc += g_opcPfs[i].usTime;
			pcntAcc = (uint64_t)usAcc * 10000 / g_usTotal;
			mp_printf(&mp_plat_print, "%02d, %03d, %03d.%03dms, %02d.%02d%%, %02d.%02d%%, %03d, %s\r\n",
				i + 1, g_opcPfs[i].opCode, 
				g_opcPfs[i].usTime/1000, g_opcPfs[i].usTime%1000, 
				g_opcPfs[i].x10000Rate/100, g_opcPfs[i].x10000Rate%100,
				pcntAcc / 100, pcntAcc % 100, g_opcPfs[i].ndx, g_opcPfs[i].szOpName);					
		}
	}
	
	g_tsNdx=0;
	g_usTotal=0;
	g_ts0=0;
}

#ifdef IMLIB_ENABLE_TF

#define PY_TF_PUTCHAR_BUFFER_LEN 1023

char* py_tf_putchar_buffer = NULL;
size_t py_tf_putchar_buffer_len = 0;
int py_tf_putchar_buffer_index = 0;
STATIC void alloc_putchar_buffer()
{
    py_tf_putchar_buffer = (char *) fb_alloc0(PY_TF_PUTCHAR_BUFFER_LEN + 1, FB_ALLOC_NO_HINT);
    py_tf_putchar_buffer_len = PY_TF_PUTCHAR_BUFFER_LEN;
	py_tf_putchar_buffer_index = 0;
}

// TF Model Object
typedef struct py_tf_model_obj {
    mp_obj_base_t base;
    unsigned char *model_data;
    unsigned int model_data_len, height, width, channels;
    bool signed_or_unsigned;
    bool is_float;
} py_tf_model_obj_t;

STATIC void py_tf_model_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_tf_model_obj_t *self = self_in;
    mp_printf(print,
              "{\"len\":%d, \"height\":%d, \"width\":%d, \"channels\":%d, \"signed\":%d, \"is_float\":%d}",
              self->model_data_len,
              self->height,
              self->width,
              self->channels,
              self->signed_or_unsigned,
              self->is_float);
}

// TF Classification Object
#define py_tf_classification_obj_size 5
typedef struct py_tf_classification_obj {
    mp_obj_base_t base;
    mp_obj_t x, y, w, h, output;
} py_tf_classification_obj_t;

STATIC void py_tf_classification_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_tf_classification_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"output\":",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h));
    mp_obj_print_helper(print, self->output, kind);
    mp_printf(print, "}");
}

STATIC mp_obj_t py_tf_classification_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_tf_classification_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_tf_classification_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_tf_classification_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->w;
            case 3: return self->h;
            case 4: return self->output;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_tf_classification_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_tf_classification_obj_t *) self_in)->x,
                                              ((py_tf_classification_obj_t *) self_in)->y,
                                              ((py_tf_classification_obj_t *) self_in)->w,
                                              ((py_tf_classification_obj_t *) self_in)->h});
}

mp_obj_t py_tf_classification_x(mp_obj_t self_in) { return ((py_tf_classification_obj_t *) self_in)->x; }
mp_obj_t py_tf_classification_y(mp_obj_t self_in) { return ((py_tf_classification_obj_t *) self_in)->y; }
mp_obj_t py_tf_classification_w(mp_obj_t self_in) { return ((py_tf_classification_obj_t *) self_in)->w; }
mp_obj_t py_tf_classification_h(mp_obj_t self_in) { return ((py_tf_classification_obj_t *) self_in)->h; }
mp_obj_t py_tf_classification_output(mp_obj_t self_in) { return ((py_tf_classification_obj_t *) self_in)->output; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_classification_rect_obj, py_tf_classification_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_classification_x_obj, py_tf_classification_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_classification_y_obj, py_tf_classification_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_classification_w_obj, py_tf_classification_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_classification_h_obj, py_tf_classification_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_classification_output_obj, py_tf_classification_output);

STATIC const mp_rom_map_elem_t py_tf_classification_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&py_tf_classification_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_tf_classification_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_tf_classification_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w), MP_ROM_PTR(&py_tf_classification_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h), MP_ROM_PTR(&py_tf_classification_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_output), MP_ROM_PTR(&py_tf_classification_output_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_tf_classification_locals_dict, py_tf_classification_locals_dict_table);

static const mp_obj_type_t py_tf_classification_type = {
    { &mp_type_type },
    .name  = MP_QSTR_tf_classification,
    .print = py_tf_classification_print,
    .subscr = py_tf_classification_subscr,
    .locals_dict = (mp_obj_t) &py_tf_classification_locals_dict
};

static const mp_obj_type_t py_tf_model_type;

extern char Image$$WEIT_CACHE_AREA$$Base[];
#define weit_cache_area Image$$WEIT_CACHE_AREA$$Base


void* conv_helper_enter(uint32_t ndx, uint32_t maxSizeInBytes, uint32_t *pAllocedSizeInBytes) 
{
    pAllocedSizeInBytes[0] = WEIT_CACHE_SIZE;
    return (void*) (weit_cache_area);
}

STATIC mp_obj_t int_py_tf_load(mp_obj_t path_obj, bool alloc_mode, bool weit_cache, bool helper_mode)
{
    if (!helper_mode) {
        fb_alloc_mark();
    }

//	if(weit_cache){
//		init_weigth_cache(weit_cache_area, WEIT_CACHE_SIZE);
//	}else{
//		// Using the weight cache by assert ptr_head==NULL, so if not use weit_cache, 
//		// need to deinit, in case open then close, will mislead the code
//		deinit_weight_cache();
//	}
    const char *path = mp_obj_str_get_str(path_obj);
    py_tf_model_obj_t *tf_model = m_new_obj(py_tf_model_obj_t);
    tf_model->base.type = &py_tf_model_type;

    if (!strcmp(path, "person_detection")) {
        tf_model->model_data = (unsigned char *) g_person_detect_model_data;
        tf_model->model_data_len = g_person_detect_model_data_len;
    } else {
        FIL fp;
        file_read_open(&fp, path);
        tf_model->model_data_len = f_size(&fp);
        tf_model->model_data = alloc_mode
            ? fb_alloc(tf_model->model_data_len, FB_ALLOC_PREFER_SIZE)
            : xalloc(tf_model->model_data_len);
        read_data(&fp, tf_model->model_data, tf_model->model_data_len);
        file_close(&fp);
    }

    if (!helper_mode) {
        alloc_putchar_buffer();
    }

    uint32_t tensor_arena_size;
    uint8_t *tensor_arena = fb_alloc_all(&tensor_arena_size, FB_ALLOC_PREFER_SIZE);

    PY_ASSERT_FALSE_MSG(libtf_get_input_data_hwc(tf_model->model_data,
                                                 tensor_arena,
                                                 tensor_arena_size,
                                                 &tf_model->height,
                                                 &tf_model->width,
                                                 &tf_model->channels,
                                                 &tf_model->signed_or_unsigned,
                                                 &tf_model->is_float),
                        py_tf_putchar_buffer - (PY_TF_PUTCHAR_BUFFER_LEN - py_tf_putchar_buffer_len));

    fb_free(); // free fb_alloc_all()

    if (!helper_mode) {
        fb_free(); // free alloc_putchar_buffer()
    }

    // In this mode we leave the model allocated on the frame buffer.
    // py_tf_free_from_fb() must be called to free the model allocated on the frame buffer.
    // On error everything is cleaned because of fb_alloc_mark().

    if ((!helper_mode) && (!alloc_mode)) {
        fb_alloc_free_till_mark();
    } else if ((!helper_mode) && alloc_mode) {
        fb_alloc_mark_permanent(); // tf_model->model_data will not be popped on exception.
    }

    return tf_model;
}

STATIC mp_obj_t py_tf_load(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    return int_py_tf_load(args[0], py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_load_to_fb), false), py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_weit_cache), true), false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_tf_load_obj, 1, py_tf_load);

STATIC mp_obj_t py_tf_free_from_fb()
{
    fb_alloc_free_till_mark_past_mark_permanent();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_tf_free_from_fb_obj, py_tf_free_from_fb);

STATIC py_tf_model_obj_t *py_tf_load_alloc(mp_obj_t path_obj)
{
    if (MP_OBJ_IS_TYPE(path_obj, &py_tf_model_type)) {
        return (py_tf_model_obj_t *) path_obj;
    } else {
        return (py_tf_model_obj_t *) int_py_tf_load(path_obj, true, true, true);
    }
} 

typedef struct py_tf_input_data_callback_data {
    image_t *img;
    rectangle_t *roi;
	int offset, scale;
	uint8_t bgr_mode;//1:bgr,0:rgb
} py_tf_input_data_callback_data_t;



STATIC void py_tf_input_data_callback(void *callback_data,
                                      void *model_input,
                                      const unsigned int input_height,
                                      const unsigned int input_width,
                                      const unsigned int input_channels,
                                      const bool signed_or_unsigned,
                                      const bool is_float)
{

    py_tf_input_data_callback_data_t *arg = (py_tf_input_data_callback_data_t *) callback_data;
    int shift = signed_or_unsigned ? 128 : 0;
    float fscale = 1.0f / (arg->scale);
	float offset = arg->offset * fscale;

    float xscale = input_width / ((float) arg->roi->w);
    float yscale = input_height / ((float) arg->roi->h);
    // MAX == KeepAspectRationByExpanding - MIN == KeepAspectRatio
    float scale = IM_MAX(xscale, yscale), scale_inv = 1 / scale;
    float x_offset = ((arg->roi->w * scale) - input_width) / 2;
    float y_offset = ((arg->roi->h * scale) - input_height) / 2;

    switch (arg->img->bpp) {
        case IMAGE_BPP_BINARY: {
            for (int y = 0, yy = input_height; y < yy; y++) {
                uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(arg->img, fast_floorf((y + y_offset) * scale_inv) + arg->roi->y);
                int row = input_width * y;
                for (int x = 0, xx = input_width; x < xx; x++) {
                    int pixel = IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, fast_floorf((x + x_offset) * scale_inv) + arg->roi->x);
                    int index = row + x;
                    switch (input_channels) {
                        case 1: {
                            if (!is_float) {
                                ((uint8_t *) model_input)[index] = COLOR_BINARY_TO_GRAYSCALE(pixel) ^ shift;
                            } else {
                                ((float *) model_input)[index] = COLOR_BINARY_TO_GRAYSCALE(pixel) * fscale - offset;
                            }
                            break;
                        }
                        case 3: {
                            int index_3 = index * 3;
                            pixel = COLOR_BINARY_TO_RGB565(pixel);
                            if (!is_float) {
								if(arg->bgr_mode)
								{
									((uint8_t *) model_input)[index_3 + 0] = COLOR_RGB565_TO_B8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 2] = COLOR_RGB565_TO_R8(pixel) ^ shift;
								}
								else
								{
									((uint8_t *) model_input)[index_3 + 0] = COLOR_RGB565_TO_R8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 2] = COLOR_RGB565_TO_B8(pixel) ^ shift;
								}
                            } else {
								if(arg->bgr_mode)
								{
									((float *) model_input)[index_3 + 0] = COLOR_RGB565_TO_B8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 2] = COLOR_RGB565_TO_R8(pixel) * fscale - offset;
								}
								else
								{
									((float *) model_input)[index_3 + 0] = COLOR_RGB565_TO_R8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 2] = COLOR_RGB565_TO_B8(pixel) * fscale - offset;
								}
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            for (int y = 0, yy = input_height; y < yy; y++) {
                uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(arg->img, fast_floorf((y + y_offset) * scale_inv) + arg->roi->y);
                int row = input_width * y;
                for (int x = 0, xx = input_width; x < xx; x++) {
                    int pixel = IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, fast_floorf((x + x_offset) * scale_inv) + arg->roi->x);
                    int index = row + x;
                    switch (input_channels) {
                        case 1: {
                            if (!is_float) {
                                ((uint8_t *) model_input)[index] = pixel ^ shift;
                            } else {
                                ((float *) model_input)[index] = pixel * fscale - offset;
                            }
                            break;
                        }
                        case 3: {
                            int index_3 = index * 3;
                            pixel = COLOR_GRAYSCALE_TO_RGB565(pixel);
                            if (!is_float) {
								if(arg->bgr_mode)
								{
									((uint8_t *) model_input)[index_3 + 0] = COLOR_RGB565_TO_B8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 2] = COLOR_RGB565_TO_R8(pixel) ^ shift;
								}
								else
								{
									((uint8_t *) model_input)[index_3 + 0] = COLOR_RGB565_TO_R8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 2] = COLOR_RGB565_TO_B8(pixel) ^ shift;
								}
                            } else {
								if(arg->bgr_mode)
								{
									((float *) model_input)[index_3 + 0] = COLOR_RGB565_TO_B8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 2] = COLOR_RGB565_TO_R8(pixel) * fscale - offset;
								}
								else
								{
									((float *) model_input)[index_3 + 0] = COLOR_RGB565_TO_R8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 2] = COLOR_RGB565_TO_B8(pixel) * fscale - offset;
								}
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
            break;
        }
        case IMAGE_BPP_RGB565: {
            for (int y = 0, yy = input_height; y < yy; y++) {
                uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(arg->img, fast_floorf((y + y_offset) * scale_inv) + arg->roi->y);
                int row = input_width * y;
                for (int x = 0, xx = input_width; x < xx; x++) {
                    int pixel = IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, fast_floorf((x + x_offset) * scale_inv) + arg->roi->x);
                    int index = row + x;
                    switch (input_channels) {
                        case 1: {
                            if (!is_float) {
                                ((uint8_t *) model_input)[index] = COLOR_RGB565_TO_GRAYSCALE(pixel) ^ shift;
                            } else {
                                ((float *) model_input)[index] = COLOR_RGB565_TO_GRAYSCALE(pixel) * fscale - offset;
                            }
                            break;
                        }
                        case 3: {
                            int index_3 = index * 3;
                            if (!is_float) {
								if(arg->bgr_mode)
								{
									((uint8_t *) model_input)[index_3 + 0] = COLOR_RGB565_TO_B8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 2] = COLOR_RGB565_TO_R8(pixel) ^ shift;
								}
								else
								{
									((uint8_t *) model_input)[index_3 + 0] = COLOR_RGB565_TO_R8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) ^ shift;
									((uint8_t *) model_input)[index_3 + 2] = COLOR_RGB565_TO_B8(pixel) ^ shift;
								}
                            } else {
								if(arg->bgr_mode)
								{
									((float *) model_input)[index_3 + 0] = COLOR_RGB565_TO_B8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 2] = COLOR_RGB565_TO_R8(pixel) * fscale - offset;

								}
								else
								{
									((float *) model_input)[index_3 + 0] = COLOR_RGB565_TO_R8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 1] = COLOR_RGB565_TO_G8(pixel) * fscale - offset;
									((float *) model_input)[index_3 + 2] = COLOR_RGB565_TO_B8(pixel) * fscale - offset;
								}
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
            break;
        }
        default: {
            break;
        }
    }

}

typedef struct py_tf_classify_output_data_callback_data {
    mp_obj_t out;
} py_tf_classify_output_data_callback_data_t;

STATIC void py_tf_classify_output_data_callback(void *callback_data,
                                                void *model_output,
                                                const unsigned int output_height,
                                                const unsigned int output_width,
                                                const unsigned int output_channels,
                                                const bool signed_or_unsigned,
                                                const bool is_float)
{
    py_tf_classify_output_data_callback_data_t *arg = (py_tf_classify_output_data_callback_data_t *) callback_data;
    int shift = signed_or_unsigned ? 128 : 0;

    PY_ASSERT_TRUE_MSG(output_height == 1, "Expected model output height to be 1!");
    PY_ASSERT_TRUE_MSG(output_width == 1, "Expected model output width to be 1!");

    arg->out = mp_obj_new_list(output_channels, NULL);
    for (unsigned int i = 0; i < output_channels; i++) {
        if (!is_float) {
            ((mp_obj_list_t *) arg->out)->items[i] = mp_obj_new_float((((uint8_t *) model_output)[i] ^ shift) / 255.0f);
        } else {
            ((mp_obj_list_t *) arg->out)->items[i] = mp_obj_new_float(((float *) model_output)[i]);
        }
    }
}


STATIC mp_obj_t py_tf_classify(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	int tick;
    fb_alloc_mark();
    alloc_putchar_buffer();

    py_tf_model_obj_t *arg_model = py_tf_load_alloc(args[0]);
    image_t *arg_img = py_helper_arg_to_image_mutable(args[1]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 2, kw_args, &roi);

    float arg_min_scale = py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_min_scale), 1.0f);
    PY_ASSERT_TRUE_MSG((0.0f < arg_min_scale) && (arg_min_scale <= 1.0f), "0 < min_scale <= 1");

    float arg_scale_mul = py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale_mul), 0.5f);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_scale_mul) && (arg_scale_mul < 1.0f), "0 <= scale_mul < 1");

    float arg_x_overlap = py_helper_keyword_float(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_overlap), 0.0f);
    PY_ASSERT_TRUE_MSG(((0.0f <= arg_x_overlap) && (arg_x_overlap < 1.0f)) || (arg_x_overlap == -1.0f), "0 <= x_overlap < 1");

    float arg_y_overlap = py_helper_keyword_float(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_overlap), 0.0f);
    PY_ASSERT_TRUE_MSG(((0.0f <= arg_y_overlap) && (arg_y_overlap < 1.0f)) || (arg_y_overlap == -1.0f), "0 <= y_overlap < 1");
	
	int offset = py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 128);	
	int fscale = py_helper_keyword_int(n_args, args, 8, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale), 128);
	int bgr_mode = py_helper_keyword_int(n_args, args, 9, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bgr), 0);
	
    uint32_t tensor_arena_size;
    uint8_t *tensor_arena = fb_alloc_all(&tensor_arena_size, FB_ALLOC_PREFER_SIZE);

    mp_obj_t objects_list = mp_obj_new_list(0, NULL);

    for (float scale = 1.0f; scale >= arg_min_scale; scale *= arg_scale_mul) {
        // Either provide a subtle offset to center multiple detection windows or center the only detection window.
        for (int y = roi.y + ((arg_y_overlap != -1.0f) ? (fmodf(roi.h, (roi.h * scale)) / 2.0f) : ((roi.h - (roi.h * scale)) / 2.0f));
            // Finish when the detection window is outside of the ROI.
            (y + (roi.h * scale)) <= (roi.y + roi.h);
            // Step by an overlap amount accounting for scale or just terminate after one iteration.
            y += ((arg_y_overlap != -1.0f) ? (roi.h * scale * (1.0f - arg_y_overlap)) : roi.h)) {
            // Either provide a subtle offset to center multiple detection windows or center the only detection window.
            for (int x = roi.x + ((arg_x_overlap != -1.0f) ? (fmodf(roi.w, (roi.w * scale)) / 2.0f) : ((roi.w - (roi.w * scale)) / 2.0f));
                // Finish when the detection window is outside of the ROI.
                (x + (roi.w * scale)) <= (roi.x + roi.w);
                // Step by an overlap amount accounting for scale or just terminate after one iteration.
                x += ((arg_x_overlap != -1.0f) ? (roi.w * scale * (1.0f - arg_x_overlap)) : roi.w)) {

                rectangle_t new_roi;
                rectangle_init(&new_roi, x, y, roi.w * scale, roi.h * scale);

                if (rectangle_overlap(&roi, &new_roi)) { // Check if new_roi is null...

                    py_tf_input_data_callback_data_t py_tf_input_data_callback_data;
                    py_tf_input_data_callback_data.img = arg_img;
                    py_tf_input_data_callback_data.roi = &new_roi;
					py_tf_input_data_callback_data.offset = offset;
					py_tf_input_data_callback_data.scale = fscale;
					py_tf_input_data_callback_data.bgr_mode = bgr_mode;
					tick = rt_tick_get();
                    py_tf_classify_output_data_callback_data_t py_tf_classify_output_data_callback_data;
                    PY_ASSERT_FALSE_MSG(libtf_invoke(arg_model->model_data,
                                                     tensor_arena,
                                                     tensor_arena_size,
                                                     py_tf_input_data_callback,
                                                     &py_tf_input_data_callback_data,
                                                     py_tf_classify_output_data_callback,
                                                     &py_tf_classify_output_data_callback_data),
                                        py_tf_putchar_buffer - (PY_TF_PUTCHAR_BUFFER_LEN - py_tf_putchar_buffer_len));

					//mp_printf(&mp_plat_print, "TFLite Inference during %dms\r\n",rt_tick_get()-tick);
                    py_tf_classification_obj_t *o = m_new_obj(py_tf_classification_obj_t);
                    o->base.type = &py_tf_classification_type;
                    o->x = mp_obj_new_int(new_roi.x);
                    o->y = mp_obj_new_int(new_roi.y);
                    o->w = mp_obj_new_int(new_roi.w);
                    o->h = mp_obj_new_int(new_roi.h);
                    o->output = py_tf_classify_output_data_callback_data.out;
                    mp_obj_list_append(objects_list, o);
                }
            }
        }
    }

    fb_alloc_free_till_mark();

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_tf_classify_obj, 2, py_tf_classify);

typedef struct py_tf_logistic_input_data_callback_data
{
    mp_obj_t *input;
    int scale;
    int offset;
    int data_size;
}py_tf_logistic_input_data_callback_data_t;

typedef struct py_tf_logistic_output_data_callback_data
{
    mp_obj_t out;
}py_tf_logistic_output_data_callback_data_t;
STATIC void py_tf_logistic_input_data_callback(void *callback_data,
                                      void *model_input,
                                      const unsigned int input_height,
                                      const unsigned int input_width,
                                      const unsigned int input_channels,
                                      const bool signed_or_unsigned,
                                      const bool is_float)
{
    py_tf_logistic_input_data_callback_data_t *arg = (py_tf_logistic_input_data_callback_data_t*)callback_data;
    int shift = signed_or_unsigned ? 128 : 0;
    float fscale = 1.0f / (arg->scale);
	float offset = arg->offset * fscale;
    int len = input_height * input_width * input_channels;

    PY_ASSERT_TRUE_MSG((len == arg->data_size), "input param size not match with model input");
    for(int i=0;i<len;i++)
    {
        if(is_float)
            ((float *)model_input)[i] = mp_obj_get_int(arg->input[i])*fscale - offset;
        else
            ((uint8_t *)model_input)[i] =  mp_obj_get_int(arg->input[i])^shift;
        
        mp_printf(&mp_plat_print, "%d:%f ",mp_obj_get_int(arg->input[i]),((float *)model_input)[i]);
    }
}

STATIC void py_tf_logistic_output_data_callback(void *callback_data,
                                                void *model_output,
                                                const unsigned int output_height,
                                                const unsigned int output_width,
                                                const unsigned int output_channels,
                                                const bool signed_or_unsigned,
                                                const bool is_float)
{
    py_tf_logistic_output_data_callback_data_t *arg = (py_tf_logistic_output_data_callback_data_t*)callback_data;
	int shift = signed_or_unsigned ? 128 : 0;
	
    arg->out = mp_obj_new_list(output_channels, NULL);
    for (unsigned int i = 0; i < output_channels; i++) {
        if (!is_float) {
            ((mp_obj_list_t *) arg->out)->items[i] = mp_obj_new_float((((uint8_t *) model_output)[i] ^ shift) / 255.0f);
        } else {
            ((mp_obj_list_t *) arg->out)->items[i] = mp_obj_new_float(((float *) model_output)[i]);
        }
    }
}

STATIC mp_obj_t py_tf_logistic(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    py_tf_model_obj_t *arg_model = py_tf_load_alloc(args[0]);
    py_tf_logistic_input_data_callback_data_t py_tf_logistic_input_data_callback_data;
    py_tf_logistic_output_data_callback_data_t py_tf_logistic_output_data_callback_data;

    int data_size = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_size), 1);	
    int fscale = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale), 128);
    int offset = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    py_tf_logistic_input_data_callback_data.input = mp_obj_new_list(data_size, NULL);
    mp_obj_get_array_fixed_n(args[1], data_size, &py_tf_logistic_input_data_callback_data.input);
    py_tf_logistic_input_data_callback_data.data_size = data_size;

    py_tf_logistic_input_data_callback_data.scale = fscale;
    py_tf_logistic_input_data_callback_data.offset = offset;
    fb_alloc_mark();

    uint32_t tensor_arena_size;
    uint8_t *tensor_arena = fb_alloc_all(&tensor_arena_size, FB_ALLOC_PREFER_SIZE);
    mp_obj_t objects_list = mp_obj_new_list(0, NULL);

    PY_ASSERT_FALSE_MSG(libtf_invoke(arg_model->model_data,
                                                     tensor_arena,
                                                     tensor_arena_size,
                                                     py_tf_logistic_input_data_callback,
                                                     &py_tf_logistic_input_data_callback_data,
                                                     py_tf_logistic_output_data_callback,
                                                     &py_tf_logistic_output_data_callback_data),
                                        py_tf_putchar_buffer - (PY_TF_PUTCHAR_BUFFER_LEN - py_tf_putchar_buffer_len));

    mp_obj_list_append(objects_list, py_tf_logistic_output_data_callback_data.out);                                    
    fb_alloc_free_till_mark();

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_tf_logistic_obj,2,py_tf_logistic);


STATIC void py_tf_profile_output_data_callback(void *callback_data,
                                                void *model_output,
                                                const unsigned int output_height,
                                                const unsigned int output_width,
                                                const unsigned int output_channels,
                                                const bool signed_or_unsigned,
                                                const bool is_float)
{
 
}

STATIC mp_obj_t py_tf_classify_profile(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	int tick;
    fb_alloc_mark();
    alloc_putchar_buffer();

    py_tf_model_obj_t *arg_model = py_tf_load_alloc(args[0]);
    image_t *arg_img = py_helper_arg_to_image_mutable(args[1]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 2, kw_args, &roi);

    float arg_min_scale = py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_min_scale), 1.0f);
    PY_ASSERT_TRUE_MSG((0.0f < arg_min_scale) && (arg_min_scale <= 1.0f), "0 < min_scale <= 1");

    float arg_scale_mul = py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale_mul), 0.5f);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_scale_mul) && (arg_scale_mul < 1.0f), "0 <= scale_mul < 1");

    float arg_x_overlap = py_helper_keyword_float(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_overlap), 0.0f);
    PY_ASSERT_TRUE_MSG(((0.0f <= arg_x_overlap) && (arg_x_overlap < 1.0f)) || (arg_x_overlap == -1.0f), "0 <= x_overlap < 1");

    float arg_y_overlap = py_helper_keyword_float(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_overlap), 0.0f);
    PY_ASSERT_TRUE_MSG(((0.0f <= arg_y_overlap) && (arg_y_overlap < 1.0f)) || (arg_y_overlap == -1.0f), "0 <= y_overlap < 1");
	
	int offset = py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 128);	
	int fscale = py_helper_keyword_int(n_args, args, 8, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale), 128);

    uint32_t tensor_arena_size;
    uint8_t *tensor_arena = fb_alloc_all(&tensor_arena_size, FB_ALLOC_PREFER_SIZE);

    mp_obj_t objects_list = mp_obj_new_list(0, NULL);

    g_tf_profiling_en = 1;

    for (float scale = 1.0f; scale >= arg_min_scale; scale *= arg_scale_mul) {
        // Either provide a subtle offset to center multiple detection windows or center the only detection window.
        for (int y = roi.y + ((arg_y_overlap != -1.0f) ? (fmodf(roi.h, (roi.h * scale)) / 2.0f) : ((roi.h - (roi.h * scale)) / 2.0f));
            // Finish when the detection window is outside of the ROI.
            (y + (roi.h * scale)) <= (roi.y + roi.h);
            // Step by an overlap amount accounting for scale or just terminate after one iteration.
            y += ((arg_y_overlap != -1.0f) ? (roi.h * scale * (1.0f - arg_y_overlap)) : roi.h)) {
            // Either provide a subtle offset to center multiple detection windows or center the only detection window.
            for (int x = roi.x + ((arg_x_overlap != -1.0f) ? (fmodf(roi.w, (roi.w * scale)) / 2.0f) : ((roi.w - (roi.w * scale)) / 2.0f));
                // Finish when the detection window is outside of the ROI.
                (x + (roi.w * scale)) <= (roi.x + roi.w);
                // Step by an overlap amount accounting for scale or just terminate after one iteration.
                x += ((arg_x_overlap != -1.0f) ? (roi.w * scale * (1.0f - arg_x_overlap)) : roi.w)) {

                rectangle_t new_roi;
                rectangle_init(&new_roi, x, y, roi.w * scale, roi.h * scale);

                if (rectangle_overlap(&roi, &new_roi)) { // Check if new_roi is null...

                    py_tf_input_data_callback_data_t py_tf_input_data_callback_data;
                    py_tf_input_data_callback_data.img = arg_img;
                    py_tf_input_data_callback_data.roi = &new_roi;
					py_tf_input_data_callback_data.offset = offset;
					py_tf_input_data_callback_data.scale = fscale;
                    py_tf_classify_output_data_callback_data_t py_tf_classify_output_data_callback_data;
                    PY_ASSERT_FALSE_MSG(libtf_invoke(arg_model->model_data,
                                                     tensor_arena,
                                                     tensor_arena_size,
                                                     py_tf_input_data_callback,
                                                     &py_tf_input_data_callback_data,
                                                     py_tf_profile_output_data_callback,
                                                     &py_tf_classify_output_data_callback_data),
                                        py_tf_putchar_buffer - (PY_TF_PUTCHAR_BUFFER_LEN - py_tf_putchar_buffer_len));

		
                    py_tf_classification_obj_t *o = m_new_obj(py_tf_classification_obj_t);
                    o->base.type = &py_tf_classification_type;
                    o->x = mp_obj_new_int(new_roi.x);
                    o->y = mp_obj_new_int(new_roi.y);
                    o->w = mp_obj_new_int(new_roi.w);
                    o->h = mp_obj_new_int(new_roi.h);
                    o->output = py_tf_classify_output_data_callback_data.out;
                    mp_obj_list_append(objects_list, o);
                }
            }
        }
    }
    g_tf_profiling_en = 0;
    ShowProfiling();
    fb_alloc_free_till_mark();

    return objects_list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_tf_classify_profile_obj, 2, py_tf_classify_profile);

typedef struct py_tf_detect_output_data_callback_data {
    mp_obj_t out;
} py_tf_detect_output_data_callback_data_t;

STATIC void py_tf_detect_output_data_callback(void *callback_data,
                                                void *model_output,
                                                const unsigned int output_height,
                                                const unsigned int output_width,
                                                const unsigned int output_channels,
                                                const bool signed_or_unsigned,
                                                const bool is_float)
{
    py_tf_classify_output_data_callback_data_t *arg = (py_tf_classify_output_data_callback_data_t *) callback_data;
    int shift = signed_or_unsigned ? 128 : 0;

    PY_ASSERT_TRUE_MSG(output_height == 1, "Expected model output height to be 1!");
    PY_ASSERT_TRUE_MSG(output_width == 1, "Expected model output width to be 1!");
	
	// here is a OD outputs, with 4 output tensor
	float** outputs = (float**)model_output;
	
	float* boxes = *outputs++;
	float* labels = *outputs++;
	float* scores = *outputs++;
	float* nums = *outputs;
	
    arg->out = mp_obj_new_list((uint32_t)(*nums), NULL);
	
	// all in float
    for (unsigned int i = 0; i < (*nums); i++) {
		float* box = boxes + 4 * i;
		mp_obj_t tmp_list = mp_obj_new_list(6, NULL);
		((mp_obj_list_t *)tmp_list)->items[0] = mp_obj_new_float(box[0]);
		((mp_obj_list_t *)tmp_list)->items[1] = mp_obj_new_float(box[1]);
		((mp_obj_list_t *)tmp_list)->items[2] = mp_obj_new_float(box[2]);
		((mp_obj_list_t *)tmp_list)->items[3] = mp_obj_new_float(box[3]);
		((mp_obj_list_t *)tmp_list)->items[4] = mp_obj_new_float(labels[i]);
		((mp_obj_list_t *)tmp_list)->items[5] = mp_obj_new_float(scores[i]);
        ((mp_obj_list_t *) arg->out)->items[i] = tmp_list;
    }
}


STATIC mp_obj_t py_tf_detect(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    fb_alloc_mark();
    alloc_putchar_buffer();

    py_tf_model_obj_t *arg_model = py_tf_load_alloc(args[0]);
    image_t *arg_img = py_helper_arg_to_image_mutable(args[1]);
	
	rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 2, kw_args, &roi);
	
	int offset = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 128);	
	int fscale = py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale), 128);
	int bgr_mode = py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bgr), 0);
	
    uint32_t tensor_arena_size;
    uint8_t *tensor_arena = fb_alloc_all(&tensor_arena_size, FB_ALLOC_PREFER_SIZE);

	rectangle_t new_roi;
	rectangle_init(&new_roi, 0, 0, roi.w, roi.h);

	py_tf_input_data_callback_data_t py_tf_input_data_callback_data;
	py_tf_input_data_callback_data.img = arg_img;
	py_tf_input_data_callback_data.roi = &new_roi;
	py_tf_input_data_callback_data.offset = offset;
	py_tf_input_data_callback_data.scale = fscale;
	py_tf_input_data_callback_data.bgr_mode = bgr_mode;
	
	py_tf_detect_output_data_callback_data_t py_tf_detect_output_data_callback_data;
	PY_ASSERT_FALSE_MSG(libtf_invoke(arg_model->model_data,
									 tensor_arena,
									 tensor_arena_size,
									 py_tf_input_data_callback,
									 &py_tf_input_data_callback_data,
									 py_tf_detect_output_data_callback,
									 &py_tf_detect_output_data_callback_data),
						py_tf_putchar_buffer - (PY_TF_PUTCHAR_BUFFER_LEN - py_tf_putchar_buffer_len));

	mp_obj_t objects_list = py_tf_detect_output_data_callback_data.out;
		

    fb_alloc_free_till_mark();

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_tf_detect_obj, 2, py_tf_detect);

typedef struct py_tf_segment_output_data_callback_data {
    mp_obj_t out;
} py_tf_segment_output_data_callback_data_t;

STATIC void py_tf_segment_output_data_callback(void *callback_data,
                                               void *model_output,
                                               const unsigned int output_height,
                                               const unsigned int output_width,
                                               const unsigned int output_channels,
                                               const bool signed_or_unsigned,
                                               const bool is_float)
{
    py_tf_segment_output_data_callback_data_t *arg = (py_tf_segment_output_data_callback_data_t *) callback_data;
    int shift = signed_or_unsigned ? 128 : 0;

    arg->out = mp_obj_new_list(output_channels, NULL);
    for (unsigned int i = 0; i < output_channels; i++) {
        image_t img = {
            .w = output_width,
            .h = output_height,
            .bpp = IMAGE_BPP_GRAYSCALE,
            .pixels = xalloc(output_width * output_height * sizeof(uint8_t))
        };
        ((mp_obj_list_t *) arg->out)->items[i] = py_image_from_struct(&img);
        for (unsigned int y = 0; y < output_height; y++) {
            unsigned int row = y * output_width * output_channels;
            uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&img, y);
            for (unsigned int x = 0; x < output_width; x++) {
                unsigned int col = x * output_channels;
                if (!is_float) {
                    IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr, x, ((uint8_t *) model_output)[row + col + i] ^ shift);
                } else {
                    IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr, x, ((float *) model_output)[i] * 255);
                }
            }
        }
    }
}

STATIC mp_obj_t py_tf_segment(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    fb_alloc_mark();
    alloc_putchar_buffer();

    py_tf_model_obj_t *arg_model = py_tf_load_alloc(args[0]);
    image_t *arg_img = py_helper_arg_to_image_mutable(args[1]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 2, kw_args, &roi);

    uint32_t tensor_arena_size;
    uint8_t *tensor_arena = fb_alloc_all(&tensor_arena_size, FB_ALLOC_PREFER_SIZE);

    py_tf_input_data_callback_data_t py_tf_input_data_callback_data;
    py_tf_input_data_callback_data.img = arg_img;
    py_tf_input_data_callback_data.roi = &roi;

    py_tf_segment_output_data_callback_data_t py_tf_segment_output_data_callback_data;

    PY_ASSERT_FALSE_MSG(libtf_invoke(arg_model->model_data,
                                     tensor_arena,
                                     tensor_arena_size,
                                     py_tf_input_data_callback,
                                     &py_tf_input_data_callback_data,
                                     py_tf_segment_output_data_callback,
                                     &py_tf_segment_output_data_callback_data),
                        py_tf_putchar_buffer - (PY_TF_PUTCHAR_BUFFER_LEN - py_tf_putchar_buffer_len));

    fb_alloc_free_till_mark();

    return py_tf_segment_output_data_callback_data.out;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_tf_segment_obj, 2, py_tf_segment);

mp_obj_t py_tf_len(mp_obj_t self_in) { return mp_obj_new_int(((py_tf_model_obj_t *) self_in)->model_data_len); }
mp_obj_t py_tf_height(mp_obj_t self_in) { return mp_obj_new_int(((py_tf_model_obj_t *) self_in)->height); }
mp_obj_t py_tf_width(mp_obj_t self_in) { return mp_obj_new_int(((py_tf_model_obj_t *) self_in)->width); }
mp_obj_t py_tf_channels(mp_obj_t self_in) { return mp_obj_new_int(((py_tf_model_obj_t *) self_in)->channels); }
mp_obj_t py_tf_signed(mp_obj_t self_in) { return mp_obj_new_int(((py_tf_model_obj_t *) self_in)->signed_or_unsigned); }
mp_obj_t py_tf_is_float(mp_obj_t self_in) { return mp_obj_new_int(((py_tf_model_obj_t *) self_in)->is_float); }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_len_obj, py_tf_len);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_height_obj, py_tf_height);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_width_obj, py_tf_width);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_channels_obj, py_tf_channels);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_signed_obj, py_tf_signed);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_tf_is_float_obj, py_tf_is_float);

STATIC const mp_rom_map_elem_t locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_len), MP_ROM_PTR(&py_tf_len_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&py_tf_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&py_tf_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_channels), MP_ROM_PTR(&py_tf_channels_obj) },
    { MP_ROM_QSTR(MP_QSTR_signed), MP_ROM_PTR(&py_tf_signed_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_float), MP_ROM_PTR(&py_tf_is_float_obj) },
    { MP_ROM_QSTR(MP_QSTR_classify), MP_ROM_PTR(&py_tf_classify_obj) },
    { MP_ROM_QSTR(MP_QSTR_logistic),        MP_ROM_PTR(&py_tf_logistic_obj) },
	{ MP_ROM_QSTR(MP_QSTR_regress),        MP_ROM_PTR(&py_tf_logistic_obj) },
    { MP_ROM_QSTR(MP_QSTR_profile), MP_ROM_PTR(&py_tf_classify_profile_obj)},
    { MP_ROM_QSTR(MP_QSTR_detect),          MP_ROM_PTR(&py_tf_detect_obj) },
    { MP_ROM_QSTR(MP_QSTR_segment), MP_ROM_PTR(&py_tf_segment_obj) }
};

STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

STATIC const mp_obj_type_t py_tf_model_type = {
    { &mp_type_type },
    .name  = MP_QSTR_tf_model,
    .print = py_tf_model_print,
    .locals_dict = (mp_obj_t) &locals_dict
};

#endif // IMLIB_ENABLE_TF

STATIC const mp_rom_map_elem_t globals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_tf) },
#ifdef IMLIB_ENABLE_TF
    { MP_ROM_QSTR(MP_QSTR_load),            MP_ROM_PTR(&py_tf_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_from_fb),    MP_ROM_PTR(&py_tf_free_from_fb_obj) },
    { MP_ROM_QSTR(MP_QSTR_classify),        MP_ROM_PTR(&py_tf_classify_obj) },
    { MP_ROM_QSTR(MP_QSTR_logistic),        MP_ROM_PTR(&py_tf_logistic_obj) },
    { MP_ROM_QSTR(MP_QSTR_regress),        MP_ROM_PTR(&py_tf_logistic_obj) },
    { MP_ROM_QSTR(MP_QSTR_profile),         MP_ROM_PTR(&py_tf_classify_profile_obj)},
    { MP_ROM_QSTR(MP_QSTR_detect),          MP_ROM_PTR(&py_tf_detect_obj) },
    { MP_ROM_QSTR(MP_QSTR_segment),         MP_ROM_PTR(&py_tf_segment_obj) },
#else
    { MP_ROM_QSTR(MP_QSTR_load),            MP_ROM_PTR(&py_func_unavailable_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_from_fb),    MP_ROM_PTR(&py_func_unavailable_obj) },
    { MP_ROM_QSTR(MP_QSTR_classify),        MP_ROM_PTR(&py_func_unavailable_obj) },
    { MP_ROM_QSTR(MP_QSTR_segment),         MP_ROM_PTR(&py_func_unavailable_obj) }
#endif // IMLIB_ENABLE_TF
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t tf_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t) &globals_dict
};