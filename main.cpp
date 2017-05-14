#include <sys/types.h>

#ifndef _WIN32

#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include<iostream>
#include<boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <string.h>
#include <microhttpd.h>
#include <string>
#include <zlib.h>
#include <vector>

#ifdef _MSC_VER
#ifndef strcasecmp
#define strcasecmp(a,b) _stricmp((a),(b))
#endif /* !strcasecmp */
#endif /* _MSC_VER */

#if defined(_MSC_VER) && _MSC_VER + 0 <= 1800
/* Substitution is OK while return value is not used */
#define snprintf _snprintf
#endif

#define PORT            7777
#define POSTBUFFERSIZE  1000000

using namespace std;
using namespace boost;

char * response = (char *) "OK";

enum ConnectionType {
    GET = 0,
    POST = 1
};

struct connection_info_struct {
    enum ConnectionType connectiontype;
    char * zipcontent;
    int zipindex;
};

set<string> words;

static int
send_page(struct MHD_Connection *connection, const char *page, int status_code) {
    int ret;
    struct MHD_Response *response;

    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_MUST_COPY);
    if (!response) {
        return MHD_NO;
    }
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
    ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static void
add_words(void *coninfo_cls) {
    struct connection_info_struct *con_info = (connection_info_struct *) coninfo_cls;

    // unzip
    char * uncompressed = (char*)calloc(POSTBUFFERSIZE, sizeof(char));
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt) (size_t) con_info->zipindex; // size of input
    infstream.next_in = (Bytef *) con_info->zipcontent; // input char array
    infstream.avail_out = (uInt) POSTBUFFERSIZE * sizeof(char); // size of output
    infstream.next_out = (Bytef *)uncompressed; // output char array
    inflateInit2(&infstream,16+MAX_WBITS);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    //tokenize
    string separator = "\\s+|\\t+|\\n+|\\r+| ";
    char *p = strtok(uncompressed, separator);
    while (p) {
        string word = p;
        trim(word);
        if(!word.empty()) {
            words.insert(p);
        }
        p = strtok(NULL, separator);
    }
}

static void
request_completed(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct connection_info_struct *con_info = (connection_info_struct *) *con_cls;

    if (NULL == con_info) {
        return;
    }

    if (con_info->connectiontype == POST) {
        if (con_info->zipcontent) {
            free(con_info->zipcontent);
            con_info->zipcontent;
        }
    }

    free(con_info);
    *con_cls = NULL;
}


static int
answer_to_connection(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version,
                     const char *upload_data, size_t *upload_data_size, void **con_cls) {
    if (NULL == *con_cls) {
        struct connection_info_struct *con_info;

        con_info = (connection_info_struct *) malloc(sizeof(struct connection_info_struct));
        if (con_info == NULL) {
            return MHD_NO;
        }

        if (0 == strcasecmp(method, MHD_HTTP_METHOD_POST)) {
            con_info->zipcontent = (char *) calloc(POSTBUFFERSIZE, sizeof(char));
            con_info->connectiontype = POST;
        } else {
            con_info->connectiontype = GET;
        }

        *con_cls = (void *) con_info;

        return MHD_YES;
    }

    if (0 == strcasecmp(method, MHD_HTTP_METHOD_GET)) {
        char buffer[1024];
        long word_size = words.size();
        snprintf(buffer, sizeof(buffer), "%ld", word_size);
        return send_page(connection, buffer, MHD_HTTP_OK);
    }

    if (0 == strcasecmp(method, MHD_HTTP_METHOD_POST)) {
        struct connection_info_struct *con_info = (connection_info_struct *) *con_cls;

        if (0 != *upload_data_size) {
            for (size_t i = 0; i < *upload_data_size; ++i) {
                con_info->zipcontent[con_info->zipindex] = upload_data[i];
                con_info->zipindex ++;
            }
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            // zip content is complete
            // count words
            add_words(con_info);

            // send response
            return send_page(connection, response, MHD_HTTP_ACCEPTED);
        }
    }

    return send_page(connection, response, MHD_HTTP_BAD_REQUEST);
}

int
main() {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &answer_to_connection, NULL,
                              MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        return 1;
    }
    (void) getchar();
    MHD_stop_daemon(daemon);
    return 0;
}