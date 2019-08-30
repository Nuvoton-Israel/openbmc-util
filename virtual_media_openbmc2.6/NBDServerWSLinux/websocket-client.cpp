
#include "mainwindow.h"
#include <QtCore/QDebug>
#include <QCoreApplication>
#include <QMessageBox>
#include <QtWidgets>
#include "nbdprotocol.h"

#include <QUrlQuery>

void MainWindow::http_get(char * hostname, char * path){
    char buf[1024];

    int len = snprintf( buf, 1024 ,
                        "GET %s HTTP/1.1\r\n"
                        "User-Agent: NBDServer\r\n"
                        "Host: %s\r\n"
                        "Accept: */*\r\n"
                        "Cookie: %s\r\n"
                        "Content-Type: application/json\r\n\r\n",
                        path, hostname, SID.toUtf8().constData());


    QByteArray http_header_request;
    int i;
    for(i=0;i<len;i++)
        http_header_request.append(buf[i]);
    sslSocket->write(http_header_request);
}

void MainWindow::http_post(char * hostname,char * post_data, char * path){
    char buf[1024];


    int len = snprintf( buf, 1024 ,
                        "POST %s HTTP/1.1\r\n"
                        "User-Agent: NBDServer\r\n"
                        "Host: %s\r\n"
                        "Accept: */*\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %d\r\n\r\n",
                        path, hostname, strlen(post_data));

    qInfo("POST data len:%d\n",len);

    QByteArray http_header_request;
    int i;
    for(i=0;i<len;i++)
        http_header_request.append(buf[i]);
    sslSocket->write(http_header_request);


    QByteArray http_post_body;
    for(i=0;i<strlen(post_data );i++)
        http_post_body.append(post_data[i]);
    sslSocket->write(http_post_body);
}

void MainWindow::onSslMessageReceived(void)
{
    //QMessageBox::information(this, tr("Into"), tr("Got MSG"));
    //qInfo("onSslMessageReceived------------------------------------");
    //login_get_sid=0;
    int response_has_body=0;

    while (sslSocket->canReadLine())
    {
        // read a line
        QString line = QString::fromUtf8(sslSocket->readLine());
        if(line.startsWith("Set-Cookie:",Qt::CaseInsensitive)){
            int lastindex = line.lastIndexOf('"');
            SID = line.mid(12,lastindex+1-12);
            qInfo("SID(%s)",SID.toUtf8().constData());
            wsSocket_vmws->setSIDfromHTTP(SID);
            login_get_sid=1;
        }else if(line.startsWith("Content-Length:",Qt::CaseInsensitive)){
            int lastindex = line.lastIndexOf('\r');
            QString Content_Length = line.mid(16,lastindex-16);
            qInfo("Content Length(%s)",Content_Length.toUtf8().constData());
            response_has_body=1;
        }
        //qInfo("Got(%s)",line.toUtf8().constData());
    }

    if(login_get_sid && !response_has_body && !connected_to_wss){
        connected_to_wss=1;
        ConnectedtoWS();
    }

    //qInfo("**************************************");
}

void MainWindow::onSslDisconnected(){
    //QMessageBox::information(this, tr("Info"), tr("onSslDisconnected"));
    qInfo("onSslDisconnected");
}

void MainWindow::onSslErrors(const QList<QSslError> &errors)
{
    sslSocket->ignoreSslErrors();
    for (int i=0, sz=errors.size(); i<sz; i++)
    {
        QString errorString = errors.at(i).errorString();

        qInfo("onSslErrors_tls:%s",errorString.toStdString().c_str());
    }
}

void MainWindow::onSslConnected()
{
    connect(sslSocket, &QSslSocket::readyRead,    this, &MainWindow::onSslMessageReceived);
    connect(sslSocket, &QSslSocket::disconnected, this, &MainWindow::onSslDisconnected);
    char buf[1024];
#if 1
    int len = snprintf( buf, 1024 ,
                           "{\"data\": [ \"%s\", \"%s\" ] }",
                           Line_Account->text().toLocal8Bit().constData(),Line_Password->text().toLocal8Bit().constData());
#else
    int len = snprintf( buf, 1024 , "{\"data\": [ \"root\", \"0penBmc\" ] }");

#endif
    //qInfo("ip_addr before http_post_login(%s)",ip_addr);
    qInfo("login post buf:%s",buf);

    http_post(ip_addr,buf,"/login");
}

void MainWindow::Connectedto(QUrl &url)
{
    /**
     * @brief login to poleg
     */
    sslSocket = new QSslSocket;
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    connect(sslSocket, &QSslSocket::connected, this, &MainWindow::onSslConnected);
    typedef void (QSslSocket:: *sslErrorsSignalTLS)(const QList<QSslError> &);
    connect(sslSocket, static_cast<sslErrorsSignalTLS>(&QSslSocket::sslErrors),
               this, &MainWindow::onSslErrors);
    qInfo("In Connectedto:%s(%d)",ip_addr,port);
    sslSocket->connectToHostEncrypted(ip_addr ,port);
}

void MainWindow::ConnectedtoWS(){
    qInfo("In ConnectedtoWS");
    /**
     * @brief Connect to websocket
     */
    QObject::connect(wsSocket_vmws, SIGNAL(frameReceived(QByteArray)), this, SLOT(onBinaryMessageReceived(QByteArray)));
    QObject::connect(wsSocket_vmws, SIGNAL(connected()), this, SLOT(onConnected()));
    QObject::connect(wsSocket_vmws, SIGNAL(disconnected()), this, SLOT(onDisconnected()));

    char buf[1024]={0};
    int len = snprintf( buf, 1024 ,"wss://%s",ip_addr);

    qInfo("ConnectedtoWS url buf:%s(%d)",buf,len);

    wsSocket_vmws->connectToHost(buf, port);
}

void MainWindow::onDisconnected(){
    qInfo("WebSocket onDisconnected");
}

void MainWindow::onConnected()
{
    //QMessageBox::information(this, tr("Info"), tr("WebSocket Connected Successfully"));
    qInfo("WebSocket Connected Successfully");

    connection_flag=1;
    if(opendevice()<0){
       qInfo("Open Device Fail");
    }else{
        if(negotiate()<0){
            qInfo("send magic and small flag Fail");
        }
    }
}

void MainWindow::onBinaryMessageReceived(QByteArray message)
{
    int i;
    int read_len=0;
    char *ch;
    ch = message.data();
    int got_len = message.size();
    int pass_write;

    ////////////////////////////////////////////////////////////////
    /// negotiate state is 1~5
    /// transfer state is 6~7
    ////////////////////////////////////////////////////////////////
    uint32_t cflags = 0;
    uint32_t unknow_opt_len;
    uint64_t magic_r = 0;
    uint32_t opt = 0;
    unsigned char exportsize[8];
    uint16_t flags;
    char zeros[128];
    struct nbd_request req;

    while(read_len<got_len){
        switch(nego_state){
        case 1://required 4 bytes
            qInfo("State 1:Want to read cflags");
            glob_flags=0;

            memcpy(&cflags,ch+read_len,4);
            read_len+=4;
            cflags = htonl(cflags);
            if (cflags & NBD_FLAG_C_NO_ZEROES) {
                glob_flags |= F_NO_ZEROES;
            }
            nego_state=2;

            break;
        case 2://required 8 bytes
            qInfo("State 2:Want to Read IHAVEOPT magic");
            memcpy(&magic_r,ch+read_len,8);
            read_len+=8;
            //C: 64 bits, 0x49484156454F5054 (ASCII 'IHAVEOPT')
            magic_r = ntohll(magic_r);
            nego_state=3;

            break;
        case 3://required 4 bytes
            qInfo("State 3:Read opt 4 bytes\n");
            memcpy(&opt,ch+read_len,4);
            read_len+=4;
            opt = ntohl(opt);
            if(opt==NBD_OPT_EXPORT_NAME){
                qInfo("opt is NBD_OPT_EXPORT_NAME");
                nego_state=4;
            }else if(opt==NBD_OPT_ABORT){
                qInfo("opt is NBD_OPT_ABORT");
                wsSocket_vmws->disconnectFromHost();

                MountB->setEnabled(false);
                StartB->setEnabled(true);
                StopB->setEnabled(false);
                UnmountB->setEnabled(false);
                nego_state=0;
            }else{
                unknow_opt_len=0;
                memcpy(&unknow_opt_len,ch+read_len,4);
                read_len+=4;
                unknow_opt_len = ntohl(unknow_opt_len);
                if((got_len-read_len)>=unknow_opt_len){
                    qInfo("The given option is unknown to this server implementation");
                    read_len+=unknow_opt_len;
                    send_reply(opt, NBD_REP_ERR_UNSUP, -1, "The given option is unknown to this server implementation");
                    nego_state=2;
                }else{
                   qInfo("unknow_opt_len:%d(read_len:%d)(%d)",unknow_opt_len,read_len,got_len);
                   QMessageBox::information(this, tr("TODO"), tr("Need to read more for unkown opt"));
                }
            }
            break;
        case 4://required 4 bytes
            qInfo("State 4:Want to read namesize");
            nbdclient_namesize=0;
            memcpy(&nbdclient_namesize,ch+read_len,4);
            read_len+=4;
            qInfo("Read Name Size:%d",nbdclient_namesize);
            if(nbdclient_namesize>0)
                nego_state=5;
            else{
                qInfo("Name size is 0, export file size(%lld)",fsize);

                // send size of file
                exportsize[7] = (fsize       ) & 255;	// low word
                exportsize[6] = (fsize  >>  8) & 255;
                exportsize[5] = (fsize  >> 16) & 255;
                exportsize[4] = (fsize  >> 24) & 255;
                exportsize[3] = (fsize  >> 32) & 255;	// high word
                exportsize[2] = (fsize  >> 40) & 255;
                exportsize[1] = (fsize  >> 48) & 255;
                exportsize[0] = (fsize  >> 56) & 255;

                if (WRITE_BINARY((char *)exportsize, 8) < 0) {
                    qInfo("Failed to send filesize.");
                }else{

                    flags = NBD_FLAG_HAS_FLAGS;
                    flags = htons(flags);

                    if (WRITE_BINARY((char *)&flags, sizeof(flags)) < 0) {
                        qInfo("Failed to send flags.");
                    }else{
                        if (!(glob_flags & F_NO_ZEROES)) {

                            memset(zeros, '\0', sizeof(zeros));
                            qInfo("Send zeros.\n");
                            if (WRITE_BINARY((char *)zeros, 124) < 0) {
                                qInfo("Failed to send zero.");
                            }
                        }

                        qInfo("set nego_state to 6");
                        nego_state=6;
                    }
                }
            }
            break;

        case 5:
            qInfo("State 5:Want to read nbd_client_name");
            if(nbdclient_namesize>=128)
                nbdclient_namesize=127;

            memset(nbd_client_name,0,128);
            memcpy(nbd_client_name,ch,nbdclient_namesize);
            read_len+=nbdclient_namesize;
            qInfo("export file size(%lld) sizeof(%d)",fsize,sizeof(fsize));

            // send size of file
            exportsize[7] = (fsize       ) & 255;	// low word
            exportsize[6] = (fsize  >>  8) & 255;
            exportsize[5] = (fsize  >> 16) & 255;
            exportsize[4] = (fsize  >> 24) & 255;
            exportsize[3] = (fsize  >> 32) & 255;	// high word
            exportsize[2] = (fsize  >> 40) & 255;
            exportsize[1] = (fsize  >> 48) & 255;
            exportsize[0] = (fsize  >> 56) & 255;

            if (WRITE_BINARY((char *)exportsize, 8) < 0) {
                qInfo("Failed to send filesize.");
            }else{

                flags = NBD_FLAG_HAS_FLAGS;
                flags = htons(flags);
                if (WRITE_BINARY((char *)&flags, sizeof(flags)) < 0) {
                    qInfo("Failed to send flags.");
                }else{

                    if (!(glob_flags & F_NO_ZEROES)) {

                        memset(zeros, '\0', sizeof(zeros));
                        qInfo("Send zeros.\n");
                        if (WRITE_BINARY((char *)zeros, 124) < 0) {
                            qInfo("Failed to send zero.");
                            return;
                        }

                    }

                    nego_state=6;
                    //QMessageBox::information(this, tr("Info"), tr("NBD Negotiate Done"));
                    MountB->setEnabled(true);
                    StopB->setEnabled(true);
                    UnmountB->setEnabled(false);
                }
            }
            break;
        case 6://Start Handle_request
            //qInfo("State 6: handle request(Invalid:%d)(Total:%d)",read_len,got_len);
            memset(&req, '\0', 28);
            memcpy(&req,ch+read_len,28);
            read_len+=28;
            handle_request(req);
            break;
        case 7://receive handle_write data
            if((got_len-read_len)<write_len)
                pass_write = (got_len-read_len);
            else
                pass_write = write_len;
            //qInfo("State 7: handle write data(Invalid:%d)(Total:%d)(write_len:%ld)(pass_write:%d)",read_len,got_len,write_len,pass_write);
            handle_write(ch+read_len, pass_write);
            read_len+=pass_write;
            break;
        case 8:
            memcpy(&unknow_opt_len,ch+read_len,4);
            read_len+=4;
            qInfo("case 8: unknow_opt_len:%d(%d)",unknow_opt_len,got_len);
            break;
        default:
            qInfo("Err:Unknown State");
            return;
        }
    }

}
