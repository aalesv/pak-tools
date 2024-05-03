#include "crypt_utils.h"
typedef struct
{
    int thread_count;
    int start_k0;
    int end_k0;
    int debug_level;
    int optimization_level;
    QByteArray encr_buf;
    QByteArray clean_buf;
    QByteArray encr_test_buf;
    QByteArray clean_test_buf;
 } ThreadInit;

void start_bf_thread_pool(ThreadInit init);
int get_current_key();
bool thread_pool_finished();
