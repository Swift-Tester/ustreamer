/*****************************************************************************
#                                                                            #
#    uStreamer - Lightweight and fast MJPEG-HTTP streamer.                   #
#                                                                            #
#    Copyright (C) 2018-2023  Maxim Devaev <mdevaev@gmail.com>               #
#                                                                            #
#    This program is free software: you can redistribute it and/or modify    #
#    it under the terms of the GNU General Public License as published by    #
#    the Free Software Foundation, either version 3 of the License, or       #
#    (at your option) any later version.                                     #
#                                                                            #
#    This program is distributed in the hope that it will be useful,         #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of          #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
#    GNU General Public License for more details.                            #
#                                                                            #
#    You should have received a copy of the GNU General Public License       #
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.  #
#                                                                            #
*****************************************************************************/


#include "ftext.h"

#include <string.h>

#include <sys/types.h>

#include <linux/videodev2.h>

#include "../libs/tools.h"
#include "../libs/frame.h"

#include "ftext_font.h"


static void _ftext_draw_line(
	us_ftext_s *ft, const char *line,
	uint scale_x, uint scale_y,
	uint start_x, uint start_y);


us_ftext_s *us_ftext_init(void) {
	us_ftext_s *ft;
	US_CALLOC(ft, 1);
	ft->frame = us_frame_init();
	return ft;
}

void us_ftext_destroy(us_ftext_s *ft) {
	us_frame_destroy(ft->frame);
	US_DELETE(ft->text, free);
	free(ft);
}

void us_ftext_draw(us_ftext_s *ft, const char *text, uint width, uint height) {
	us_frame_s *const frame = ft->frame;

	if (
		frame->width == width && frame->height == height
		&& ft->text != NULL && !strcmp(ft->text, text)
	) {
		return;
	}

	US_DELETE(ft->text, free);
	ft->text = us_strdup(text);
	strcpy(ft->text, text);
	frame->width = width;
	frame->height = height;
	frame->format = V4L2_PIX_FMT_RGB24;
	frame->stride = width * 3;
	frame->used = width * height * 3;
	us_frame_realloc_data(frame, frame->used);
	memset(frame->data, 0, frame->used);

	if (frame->width == 0 || frame->height == 0) {
		return;
	}

	char *str = us_strdup(text);
	char *line;
	char *rest;

	uint block_width = 0;
	uint block_height = 0;
	while ((line = strtok_r((block_height == 0 ? str : NULL), "\n", &rest)) != NULL) {
		block_width = US_MAX(strlen(line) * 8, block_width);
		block_height += 8;
	}
	if (block_width == 0 || block_height == 0) {
		goto empty;
	}
	uint scale_x = frame->width / block_width / 2;
	uint scale_y = frame->height / block_height / 3;
	if (scale_x < scale_y / 1.5) {
		scale_y = scale_x * 1.5;
	} else if (scale_y < scale_x * 1.5) {
		scale_x = scale_y / 1.5;
	}

	strcpy(str, text);

	const uint start_y = (frame->height >= (block_height * scale_y)
		? ((frame->height - (block_height * scale_y)) / 2)
		: 0);
	uint n_line = 0;
	while ((line = strtok_r((n_line == 0 ? str : NULL), "\n", &rest)) != NULL) {
		const uint line_width = strlen(line) * 8 * scale_x;
		const uint start_x = (frame->width >= line_width
			? ((frame->width - line_width) / 2)
			: 0);
		_ftext_draw_line(ft, line, scale_x, scale_y, start_x, start_y + n_line * 8 * scale_y);
		++n_line;
	}

empty:
	free(str);
}

void _ftext_draw_line(
	us_ftext_s *ft, const char *line,
	uint scale_x, uint scale_y,
	uint start_x, uint start_y) {

	us_frame_s *const frame = ft->frame;

	const size_t len = strlen(line);

	for (uint ch_y = 0; ch_y < 8 * scale_y; ++ch_y) {
		const uint canvas_y = start_y + ch_y;
		for (uint ch_x = 0; ch_x < 8 * len * scale_x; ++ch_x) {
			if ((start_x + ch_x) >= frame->width) {
				break;
			}
			const uint canvas_x = (start_x + ch_x) * 3;
			const uint offset = canvas_y * frame->stride + canvas_x;
			if (offset >= frame->used) {
				break;
			}

			const u8 ch = US_MIN((u8)line[ch_x / 8 / scale_x], sizeof(US_FTEXT_FONT) / 8 - 1);
			const uint ch_byte = (ch_y / scale_y) % 8;
			const uint ch_bit = (ch_x / scale_x) % 8;
			const bool pix_on = !!(US_FTEXT_FONT[ch][ch_byte] & (1 << ch_bit));

			u8 *const b = &frame->data[offset]; // XXX: Big endian for Raspberry
			u8 *const g = b + 1;
			u8 *const r = g + 1;

			*r = pix_on * 0x51;
			*g = pix_on * 0x65;
			*b = pix_on * 0x65;
		}
	}
}
