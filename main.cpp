#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <string.h>
#include <microhttpd.h>
#include <zlib.h>
#include <unordered_set>

#define PORT            7777
#define POSTBUFFERSIZE  1000000

char * response = (char *) "OK\r\n";
char * bad_response = (char *) "NOK\r\n";

enum ConnectionType {
    GET = 0,
    POST = 1
};

struct connection_info_struct {
    enum ConnectionType connectiontype;
    char * zipcontent;
    int zipindex;
};

/**
 * Holder for all the unique worlds accepted by post requests.
 */
std::unordered_set<std::string> * words;
pthread_rwlock_t *lock;

static int respond(struct MHD_Connection *connection, const char *message, int status_code) {
    int ret;
    struct MHD_Response *response;

    response = MHD_create_response_from_buffer(strlen(message), (void *) message, MHD_RESPMEM_MUST_COPY);
    if (!response) {
        return MHD_NO;
    }
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
    ret = MHD_queue_response(connection, (unsigned int) status_code, response);
    MHD_destroy_response(response);
    return ret;
}

/**
 * Uncompresses the post content and adds new unique words to set.
 * @param coninfo_cls
 */
static void add_words(void *coninfo_cls) {
    struct connection_info_struct *con_info = (connection_info_struct *) coninfo_cls;

    // unzip
    uint newlen = (uint) (4 * con_info->zipindex);
    char * uncompressed = (char*) calloc(newlen, sizeof(char));
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt) con_info->zipindex; // size of input
    infstream.next_in = (Bytef *) con_info->zipcontent; // input char array
    infstream.avail_out = newlen * sizeof(char); // size of output
    infstream.next_out = (Bytef *) uncompressed; // output char array
    inflateInit2(&infstream,32+MAX_WBITS);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    //tokenize
    char * saveptr;
    char * p = strtok_r(uncompressed, " \t\r\n", &saveptr);
    while (p) {
        std::string word(p);
        if(words->find(word) == words->end()) { // there is no need to create a reed lock - because of the set implementation
            pthread_rwlock_wrlock(lock);
            // add word
            words->insert(word);
            pthread_rwlock_unlock(lock);
        }
        p = strtok_r(NULL, " \t\r\n", &saveptr);
    }
}

/**
 * Called after the whole request is handeld. Used just to clean after self.
 * @param cls
 * @param connection
 * @param con_cls
 * @param toe
 */
static void request_completed(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct connection_info_struct *con_info = (connection_info_struct *) *con_cls;

    if (NULL == con_info) {
        return;
    }

    if (con_info->connectiontype == POST) {
        if (con_info->zipcontent != NULL) {
            free(con_info->zipcontent);
            con_info->zipcontent = NULL;
        }
    }
    free(con_info);
    *con_cls = NULL;
}

/**
 * Handle method for
 * @param cls
 * @param connection
 * @param url
 * @param method
 * @param version
 * @param upload_data
 * @param upload_data_size
 * @param con_cls
 * @return
 */
static int answer_to_connection(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version,
                     const char *upload_data, size_t *upload_data_size, void **con_cls) {

    /**
     * First request fragment, read header, leave the rest for next time.
     */
    if (NULL == *con_cls) {
        struct connection_info_struct *con_info;

        con_info = (connection_info_struct *) malloc(sizeof(struct connection_info_struct));
        if (con_info == NULL) {
            return MHD_NO;
        }

        if (0 == strcasecmp(method, MHD_HTTP_METHOD_POST)) {
            con_info->zipcontent = (char *) calloc(POSTBUFFERSIZE, sizeof(char));
            con_info->zipindex = 0;
            con_info->connectiontype = POST;
        } else {
            con_info->connectiontype = GET;
        }

        *con_cls = (void *) con_info;

        return MHD_YES;
    }

    /**
     * Handle get.
     */
    if (0 == strcasecmp(method, MHD_HTTP_METHOD_GET)) {
        long word_size = words->size();
        words->clear();

        int len = snprintf(NULL, 0, "%ld\r\n", word_size);
        char *response = (char *) malloc((len + 1) * sizeof(char));
        snprintf(response, (size_t) (len + 1), "%ld\r\n", word_size);
        return respond(connection, response, 200);
    }

    /**
     * Handle post.
     */
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
            /**
             * Handle complete request.
             */
            // count words
            add_words(con_info);

            // send response
            return respond(connection, response, 200);
        }
    }
    // situation, that should not occur
    return respond(connection, bad_response, 400);
}

/**
 * Main class just creates the service - deamon and starts it.
 * Deamon creates thread for every connection - which is much slower than thread pool, but also much easier to set up :)
 * @return
 */
int main() {
    struct MHD_Daemon *daemon;

    words = new std::unordered_set<std::string> ();
    lock = new pthread_rwlock_t();

    daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, PORT, NULL, NULL, &answer_to_connection, NULL,
                              MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        return 1;
    }
    (void) getchar();
    MHD_stop_daemon(daemon);
    return 0;
}