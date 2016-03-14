/*
 * Copyright(c) 2016 Free Software Foundation, Inc.
 *
 * This file is part of libwget.
 *
 * Libwget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libwget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libwget.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Demo how to create a diagram from a text using www.websequencediagrams.com
 * Using low-level API.
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <libwget.h>

int main(int argc G_GNUC_WGET_UNUSED, const char *const *argv G_GNUC_WGET_UNUSED)
{
	wget_http_connection_t *conn = NULL;
	wget_http_request_t *req;
	wget_http_response_t *resp;
	wget_iri_t *iri;

	// set up libwget global configuration
	wget_global_init(
		// WGET_DEBUG_STREAM, stderr,
		WGET_ERROR_STREAM, stderr,
		WGET_INFO_STREAM, stdout,
		NULL);

	// This is the text that we want to convert into a GFX
	const char *text = "alice->bob: authentication request\nbob-->alice: response";
	const char *style = "qsd";
	wget_buffer_t *url = wget_buffer_alloc(128);
	wget_buffer_t *body = wget_buffer_alloc(128);


	wget_buffer_strcpy(body, "message=");
	wget_iri_escape_query(text, body);
	wget_buffer_printf_append(body, "&style=%s&apiVersion=1", style);

	iri = wget_iri_parse("http://www.websequencediagrams.com", NULL);
	req = wget_http_create_request(iri, "POST");
	wget_http_add_header(req, "Content-Type", "application/x-www-form-urlencoded");
	wget_http_add_header_printf(req, "Content-Length", "%lu", body->length);
	wget_http_add_header(req, "Connection", "keepalive");

	wget_http_open(&conn, iri);

	if (conn) {
		if (wget_http_send_request_with_body(conn, req, body->data, body->length))
			goto out;

		resp = wget_http_get_response(conn, NULL, req, 0);

		if (!resp)
			goto out;

		// wget_info_printf("answer=%s\n", resp->body->data);

		// extract image URL using a hack. Using a JSON parser would be correct.
		const char *p, *e;
		if (!(p = strstr(resp->body->data, "\"img\": \"")))
				goto out;
		if (!(e = strchr(p + 8, '\"')))
			goto out;

		p += 8;
		wget_buffer_printf(url, "http://www.websequencediagrams.com/%.*s", (int) (e - p), p);

		wget_http_free_response(&resp);
		wget_http_free_request(&req);
		wget_iri_free(&iri);

		iri = wget_iri_parse(url->data, NULL);
		req = wget_http_create_request(iri, "GET");
		wget_http_add_header(req, "Accept-Encoding", "gzip");

		if (wget_http_send_request(conn, req))
			goto out;

		resp = wget_http_get_response(conn, NULL, req, 0);

		if (!resp)
			goto out;

		FILE *fp;
		if ((fp = fopen("out.png", "w"))) {
			fwrite(resp->body->data, 1, resp->body->length, fp);
			fclose(fp);
			wget_info_printf("Saved out.png\n");
		}
	}

out:
	// The following cleanup code is just for demonstration and not needed before return/exit from an application
	wget_http_free_response(&resp);
	wget_http_free_request(&req);
	wget_http_close(&conn);
	wget_iri_free(&iri);
	wget_buffer_free(&body);
	wget_buffer_free(&url);

	// free resources - needed for valgrind testing
	wget_global_deinit();

	return 0;
}
