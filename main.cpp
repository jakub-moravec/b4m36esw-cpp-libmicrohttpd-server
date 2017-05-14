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

#define PORT            8888
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
    struct MHD_PostProcessor *postprocessor;
    char * zipcontent;
    int zipindex;
    const char *answerstring;
    int answercode;
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
//    char* unzipped = (char *) calloc(POSTBUFFERSIZE, sizeof(char));
//    ulong destLen = strlen(con_info->zipcontent);
//    int des = uncompress((Bytef *) unzipped, &destLen, (const Bytef *) con_info->zipcontent, destLen);

    //tokenize
//    char *p;
//    p = strtok(con_info->zipcontent, "\\s+");
//
//    char_separator<char> sep("\\s+");
//    tokenizer< char_separator<char> > tokens(con_info->zipcontent, sep);
//    BOOST_FOREACH (const string& t, tokens) {
//        cout << t << "." << endl;
//    }

    char *p = strtok(con_info->zipcontent, "\\s+");
    while (p) {
        words.insert(p);
        p = strtok(NULL, " ");
    }
}

static void
request_completed(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct connection_info_struct *con_info = (connection_info_struct *) *con_cls;

    if (NULL == con_info) {
        return;
    }

    if (con_info->connectiontype == POST) {
        if (NULL != con_info->postprocessor) {
            MHD_destroy_post_processor(con_info->postprocessor);
        }

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



//            con_info->postprocessor = MHD_create_post_processor(connection, POSTBUFFERSIZE, &iterate_post, (void *) con_info);
//
//            if (con_info->postprocessor == NULL) {
//                free(con_info);
//                return MHD_NO;
//            }

            con_info->connectiontype = POST;
            con_info->answercode = MHD_HTTP_ACCEPTED;
        } else {
            con_info->connectiontype = GET;
        }

        *con_cls = (void *) con_info;

        return MHD_YES;
    }

    if (0 == strcasecmp(method, MHD_HTTP_METHOD_GET)) {
        char buffer[1024];

        // todo lock
        long word_size = words.size();
        // todo unlock

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