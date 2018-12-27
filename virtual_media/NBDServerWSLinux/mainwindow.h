#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
//Nuvoton CS20 --Start
#include "QWsSocket.h"
#include <arpa/inet.h>
#include <fstream>

#include <QMessageBox>
#include <QtCore/QObject>
#include <QtNetwork/QSslError>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QWidget>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ui_mainwindow.h"
using namespace std;
//Nuvoton CS20 --End
namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
//Nuvoton CS20--STart
    QtWebsocket::QWsSocket* wsSocket_vmws;
    int destroy_flag;
    int connection_flag;
    int nego_state;
    int nego_done;
    bool writeable;
    int port;
    char dev_name[128];
    char ip_addr[16];
    uint64_t fsize;
    int fh;

    int allowWrite;
    int lock_status;
    int glob_flags;
    uint32_t nbdclient_namesize;
    char nbd_client_name[128];
    unsigned long write_len;
    uint64_t write_from;
    char write_handle[8];

    int login_get_sid;
    int connected_to_wss;
    QString SID;
    QSslSocket* sslSocket;
    void onSslMessageReceived(void);
    void onSslDisconnected();
    void onSslErrors(const QList<QSslError> &errors);
    void onSslConnected();

    void http_post(char * hostname,char * post_data, char * path);
    void http_get(char * hostname, char * path);
    void Connectedto(QUrl &url);
    void ConnectedtoWS();
    void onTextMessageReceived(QString message);

    int nbd_server_start(int argc, char *argv[]);
    int opendevice(void);
    int negotiate(void);

    int WRITE_TEXT(char *wherefrom, int howmuch);
    int WRITE_BINARY(char *wherefrom, int howmuch);

    int handle_request(struct nbd_request req);
    void handle_write(char *qbuffer, unsigned long receive);
    void handle_read(struct nbd_request *req);

    void send_reply( uint32_t opt, uint32_t reply_type, ssize_t datasize, const void* data);

//Nuvoton CS20--ENd

protected slots:
//Nuvoton CS20--STart
    void onBinaryMessageReceived(QByteArray message);
    void onConnected();
    void onDisconnected();
    void check_usb_device(QString device_list);
//Nuvoton CS20--ENd

private slots:
    void on_StartB_clicked();

    void on_StopB_clicked();

    void on_MountB_clicked();

    void on_UnmountB_clicked();

//private:
//    Ui::MainWindow *ui;
    void on_BrowseB_clicked();
    void on_SearchB_clicked();
    void on_ExitB_clicked();
};




#endif // MAINWINDOW_H
