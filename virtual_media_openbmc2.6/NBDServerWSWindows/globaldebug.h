#ifndef GLOBALDEBUG_H
#define GLOBALDEBUG_H

using namespace std;
#define NBD_SERVER_ARGC 1
#define DONT_LAUNCH_NBDC 1
#define RMS_TLS 1
#if NBD_SERVER_ARGC
int nbd_server_start(int argc, char *argv[]);
#else
struct ws_client *nbd_server_start(
        string cert,
        string privatekey,
        string device_string,
        string IP,
        int nbd_port,
        int force_tls,
        string password);
#endif

#endif // GLOBALDEBUG_H
