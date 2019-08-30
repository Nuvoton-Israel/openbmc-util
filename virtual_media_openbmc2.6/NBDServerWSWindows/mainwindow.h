/**********************************************************************
 *  This program is free software; you can redistribute it and/or     *
 *  modify it under the terms of the GNU General Public License       *
 *  as published by the Free Software Foundation; either version 2    *
 *  of the License, or (at your option) any later version.            *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 *  GNU General Public License for more details.                      *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, see http://gnu.org/licenses/     *
 *  ---                                                               *
 *  Copyright (C) 2009, Justin Davis <tuxdavis@gmail.com>             *
 *  Copyright (C) 2009-2014 ImageWriter developers                    *
 *                 https://sourceforge.net/projects/win32diskimager/  *
 **********************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifndef WINVER
#define WINVER 0x0601
#endif
#include <QtCore/QObject>
//#include <QtWebSockets/QWebSocket>
#include <QtNetwork/QSslError>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QUrl>
QT_FORWARD_DECLARE_CLASS(QWebSocket)
//#include <QMessageBox>
#include <QtWidgets>
#include <windows.h>
#include "ui_mainwindow.h"
#include "QWsSocket.h"

using namespace std;

int handle_lockio(HANDLE fh);
void handle_unlockio(HANDLE fh);
void handle_disc(HANDLE fh);

class QClipboard;
class ElapsedTimer;


#define EXAMPLE_RX_BUFFER_BYTES (1024 * 1024)

struct ws_client {
    LARGE_INTEGER fsize;
    int nego_state;
#ifdef WIN32
    HANDLE fh;
    int allowWrite;
    int lock_status;
#else
    int fd;
#endif
    char ip_addr[16];
    int port;
    char read_buffer[EXAMPLE_RX_BUFFER_BYTES];
    int data_receive;
    int data_taken;
    char dev_name[128];
    int dev_size;

    int destroy_flag;
    int connection_flag;
    int writeable;
    int nego_done;
    int close_flag;
};

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
    public:
       // QWebSocket m_webSocket;
        QtWebsocket::QWsSocket* wsSocket_vmws;
        //QtWebsocket::QWsSocket* wsSocket_enablemstg;
        //QtWebsocket::QWsSocket* wsSocket_disablemstg;

        int destroy_flag;
        int connection_flag;
        int nego_state;
        int nego_done;
        bool writeable;
        int port;
        char dev_name[128];
        char ip_addr[16];
        LARGE_INTEGER fsize;
        HANDLE fh;
        int allowWrite;
        int lock_status;
        int glob_flags;
        uint32_t nbdclient_namesize;
        char nbd_client_name[128];

        //for write command
        ULONG write_len;
        LARGE_INTEGER write_from;
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
        void handle_write(char *qbuffer, ULONG receive);
        void handle_read(struct nbd_request *req);

        void send_reply( uint32_t opt, uint32_t reply_type, ssize_t datasize, const void* data);

        static MainWindow* getInstance() {
            // !NOT thread safe  - first call from main only
            if (!instance)
                instance = new MainWindow();
            return instance;
        }

        ~MainWindow();
        void closeEvent(QCloseEvent *event);
        enum Status {STATUS_IDLE=0, STATUS_READING, STATUS_WRITING, STATUS_VERIFYING, STATUS_SERVER, STATUS_CLIENT, STATUS_IMAGE, STATUS_DEVICE, STATUS_EXIT, STATUS_CANCELED};
        bool nativeEvent(const QByteArray &type, void *vMsg, long *result);
    protected slots:
        void on_tbBrowse_clicked();
        void on_bCancel_clicked();
        void on_bSearch_clicked();
        void onBinaryMessageReceived(QByteArray message);
        void onConnected();
        void onDisconnected();
private slots:
        void on_bStart_clicked();

        void on_bExit_clicked();

        void on_lineEdit_IP_textChanged(const QString &arg1);

        void on_bMountUSB_clicked();

        void on_bUmountUSB_clicked();

        void on_lineEdit_PID_editingFinished();

        void on_lineEdit_PPW_editingFinished();

        void on_lineEdit_PID_textEdited(const QString &arg1);

        void on_lineEdit_PPW_textEdited(const QString &arg1);

protected:
        MainWindow(QWidget* = NULL);
private:
        QLabel *passphrase;
        static MainWindow* instance;
        // find attached devices
        void getLogicalDrives();
        void setReadWriteButtonState();
        void check_devie_file();
        void saveSettings();
        void loadSettings();
        void initializeHomeDir();
        int  send_ARP_to_IP(string DestIP_Start);

        HANDLE hVolume;
        HANDLE hFile;
        HANDLE hRawDisk;
        static const unsigned short ONE_SEC_IN_MS = 1000;
        unsigned long long sectorsize;
        int status;
        int search_live_rms_done;
        char *sectorData;
        char *sectorData2; //for verify
        QTime update_timer;
        ElapsedTimer *elapsed_timer = NULL;
        QClipboard *clipboard;
        QString myHomeDir;


};

#endif // MAINWINDOW_H
