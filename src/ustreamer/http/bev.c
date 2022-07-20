/*****************************************************************************
#                                                                            #
#    uStreamer - Lightweight and fast MJPEG-HTTP streamer.                   #
#                                                                            #
#    Copyright (C) 2018-2022  Maxim Devaev <mdevaev@gmail.com>               #
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


#include "bev.h"


char *us_bufferevent_format_reason(short what) {
	char *reason;
	US_CALLOC(reason, 2048);

	// evutil_socket_error_to_string() is not thread-safe
	char perror_buf[1024] = {0};
	const char *perror_ptr = us_errno_to_string(EVUTIL_SOCKET_ERROR(), perror_buf, 1024);
	bool first = true;

	strcat(reason, perror_ptr);
	strcat(reason, " (");

#	define FILL_REASON(x_bev, x_name) { \
			if (what & x_bev) { \
				if (first) { \
					first = false; \
				} else { \
					strcat(reason, ","); \
				} \
				strcat(reason, x_name); \
			} \
		}

	FILL_REASON(BEV_EVENT_READING, "reading");
	FILL_REASON(BEV_EVENT_WRITING, "writing");
	FILL_REASON(BEV_EVENT_ERROR, "error");
	FILL_REASON(BEV_EVENT_TIMEOUT, "timeout");
	FILL_REASON(BEV_EVENT_EOF, "eof"); // cppcheck-suppress unreadVariable

#	undef FILL_REASON

	strcat(reason, ")");
	return reason;
}
