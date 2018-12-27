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
 *  Copyright (C) 2009-2017 ImageWriter developers                    *
 *                 https://sourceforge.net/projects/win32diskimager/  *
 **********************************************************************/

#ifndef WINVER
#define WINVER 0x0601
#endif

#include <Qstring>
#include <QList>
#include <winsock2.h>
//#include <QNetworkInterface>
#include <iphlpapi.h>//或者#pragma comment(lib, "iphlpapi.lib")

#include <QtWidgets>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDirIterator>
#include <QClipboard>
#include <cstdio>
#include <cstdlib>

#include <winioctl.h>
#include <dbt.h>
#include <shlobj.h>
#include <iostream>
#include <sstream>
#include <qstring.h>
#include <iomanip>
#include <thread>
#include <mutex>

#include <string.h>
#include <cstdlib>
#include <errno.h>
#include <QNetworkInterface>

#include <stdio.h>
#include <getopt.h>
#include <fstream>

//#include <winsock2.h>
#include <windows.h>
#include <algorithm>
#include <stdint.h>
#include <cstdarg>
#include <vector>

#include "disk.h"
#include "mainwindow.h"
#include "elapsedtimer.h"
#include "globaldebug.h"

MainWindow* MainWindow::instance = NULL;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    search_live_rms_done=0;
    setupUi(this);
    elapsed_timer = new ElapsedTimer();
    statusbar->addPermanentWidget(elapsed_timer);   // "addpermanent" puts it on the RHS of the statusbar
    getLogicalDrives();
    status = STATUS_IDLE;
    clipboard = QApplication::clipboard();
    statusbar->showMessage(tr("Waiting for a task."));
    hVolume = INVALID_HANDLE_VALUE;
    hFile = INVALID_HANDLE_VALUE;
    hRawDisk = INVALID_HANDLE_VALUE;
    if (QCoreApplication::arguments().count() > 1)
    {
        QString fileLocation = QApplication::arguments().at(1);
        QFileInfo fileInfo(fileLocation);
        leFile->setText(fileInfo.absoluteFilePath());
    }
    lineEdit_Scan_End->setText("192.168.1.127");
    lineEdit_IP->setText("192.168.1.127");
    lineEdit_RMS_Port->setText("443");
    // Add supported Port
    comboBox_Port->addItem("443",443);
    comboBox_Port->addItem("8000",8000);
    comboBox_Port->addItem("80",80);
    comboBox_Port->addItem("1026",1026);
    comboBox_Port->setCurrentIndex(0);
    check_devie_file();
    setReadWriteButtonState();
    comboBox_Live_RMS->setEnabled(false);
    comboBox_Export_Type->setEnabled(false);
    comboBox_Port->setEnabled(false);
    sectorData = NULL;
    sectorsize = 0ul;
    loadSettings();
    if (myHomeDir.isEmpty()){
        initializeHomeDir();
    }
    lineEdit_PPW->setEchoMode(QLineEdit::Password);

    wsSocket_vmws = new QtWebsocket::QWsSocket(this, NULL, QtWebsocket::WS_V13);
    wsSocket_vmws->setWSPath("/vmws");
    allowWrite=true;
    login_get_sid=0;
    connected_to_wss=0;
}

MainWindow::~MainWindow()
{
    saveSettings();
    if (hRawDisk != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hRawDisk);
        hRawDisk = INVALID_HANDLE_VALUE;
    }
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }
    if (hVolume != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hVolume);
        hVolume = INVALID_HANDLE_VALUE;
    }
    if (sectorData != NULL)
    {
        delete[] sectorData;
        sectorData = NULL;
    }
    if (sectorData2 != NULL)
    {
        delete[] sectorData2;
        sectorData2 = NULL;
    }
    if (elapsed_timer != NULL)
    {
        delete elapsed_timer;
        elapsed_timer = NULL;
    }
}


void MainWindow::saveSettings()
{
    QSettings userSettings("HKEY_CURRENT_USER\\Software\\RemoteMediaClient", QSettings::NativeFormat);
    userSettings.beginGroup("Settings");
    userSettings.setValue("ImageDir", myHomeDir);
    userSettings.endGroup();
}

void MainWindow::loadSettings()
{
    QSettings userSettings("HKEY_CURRENT_USER\\Software\\RemoteMediaClient", QSettings::NativeFormat);
    userSettings.beginGroup("Settings");
    myHomeDir = userSettings.value("ImageDir").toString();
}

void MainWindow::initializeHomeDir()
{
    myHomeDir = QDir::homePath();
    if (myHomeDir == NULL){
        myHomeDir = qgetenv("USERPROFILE");
    }
    /* Get Downloads the Windows way */
    QString downloadPath = qgetenv("DiskImagesDir");
    if (downloadPath.isEmpty()) {
        PWSTR pPath = NULL;
        static GUID downloads = {0x374de290, 0x123f, 0x4565, 0x91, 0x64, 0x39,
                                 0xc4, 0x92, 0x5e, 0x46, 0x7b};
        if (SHGetKnownFolderPath(downloads, 0, 0, &pPath) == S_OK) {
            downloadPath = QDir::fromNativeSeparators(QString::fromWCharArray(pPath));
            LocalFree(pPath);
            if (downloadPath.isEmpty() || !QDir(downloadPath).exists()) {
                downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
            }
        }
    }
    if (downloadPath.isEmpty())
        downloadPath = QDir::currentPath();
    myHomeDir =  QDir::currentPath();
}

void MainWindow::check_devie_file()
{
    bool fileSelected = !(leFile->text().isEmpty());
    bool deviceSelected = (cboxDevice->count() > 0);
    comboBox_Export_Type->clear();
    if(deviceSelected&&fileSelected){
        comboBox_Export_Type->addItem("Device", 0);
        comboBox_Export_Type->addItem("Image", 1);
        comboBox_Export_Type->setCurrentIndex(0);
    }else if(deviceSelected){
        comboBox_Export_Type->addItem("Device", 0);
        comboBox_Export_Type->setCurrentIndex(0);
    }else if(fileSelected){
        comboBox_Export_Type->addItem("Image", 1);
        comboBox_Export_Type->setCurrentIndex(1);
    }

}

void MainWindow::setReadWriteButtonState()
{
    bool fileSelected = !(leFile->text().isEmpty());
    bool deviceSelected = (cboxDevice->count() > 0);
    bool ipSelected =  !(lineEdit_IP->text().isEmpty());
    bool id =  !(lineEdit_PID->text().isEmpty());
    bool passwd =  !(lineEdit_PPW->text().isEmpty());

    QFileInfo fi(leFile->text());
    // set read and write buttons according to status of file/device
    bStart->setEnabled(
                       (deviceSelected||( fileSelected && fi.isReadable() ) ) &&
                       ipSelected && id && passwd &&
                       search_live_rms_done);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (status == STATUS_READING)
    {
        if (QMessageBox::warning(this, tr("Exit?"), tr("Exiting now will result in a corrupt image file.\n"
                                                       "Are you sure you want to exit?"),
                                 QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        {
            status = STATUS_EXIT;
        }
        event->ignore();
    }
    else if (status == STATUS_WRITING)
    {
        if (QMessageBox::warning(this, tr("Exit?"), tr("Exiting now will result in a corrupt disk.\n"
                                                       "Are you sure you want to exit?"),
                                 QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        {
            status = STATUS_EXIT;
        }
        event->ignore();
    }
    else if (status == STATUS_VERIFYING)
    {
        if (QMessageBox::warning(this, tr("Exit?"), tr("Exiting now will cancel verifying image.\n"
                                                       "Are you sure you want to exit?"),
                                 QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        {
            status = STATUS_EXIT;
        }
        event->ignore();
    }
}

void MainWindow::on_tbBrowse_clicked()
{
    // Use the location of already entered file
    QString fileLocation = leFile->text();
    QFileInfo fileinfo(fileLocation);

    // See if there is a user-defined file extension.
    QString fileType = qgetenv("DiskImagerFiles");
    if (fileType.length() && !fileType.endsWith(";;"))
    {
        fileType.append(";;");
    }
    fileType.append(tr("Disk Images (*.img *.IMG);;*.*"));
    // create a generic FileDialog
    QFileDialog dialog(this, tr("Select a disk image"));
    dialog.setNameFilter(fileType);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setConfirmOverwrite(false);
    if (fileinfo.exists())
    {
        dialog.selectFile(fileLocation);
    }
    else
    {
        dialog.setDirectory(myHomeDir);
    }

    if (dialog.exec())
    {
        // selectedFiles returns a QStringList - we just want 1 filename,
        //	so use the zero'th element from that list as the filename
        fileLocation = (dialog.selectedFiles())[0];

        if (!fileLocation.isNull())
        {
            leFile->setText(fileLocation);
            QFileInfo newFileInfo(fileLocation);
            myHomeDir = newFileInfo.absolutePath();
            check_devie_file();
        }
    }
}



void AdvGetNextIPPointer(string *host)
{
    struct in_addr paddr;

    // Convert String To ULONG
    u_long addr1 = inet_addr(host->c_str());

    // Convert ULONG To Network Byte Order
    addr1 = ntohl(addr1);

    // Incremement By 1
    addr1 += 1;

    // Convert Network Byte Order Back To ULONG
    addr1 = htonl(addr1);

    // Convert ULONG Back To in_addr Struct
    paddr.S_un.S_addr = addr1;

    // Convert Back To String
    *host = inet_ntoa(paddr);
}

int MainWindow::send_ARP_to_IP(string DestIP_Start){
    DWORD dwRetVal = 0;
    IPAddr DestIp = 0;
    IPAddr SrcIp = 0; /* default for src ip */
    ULONG MacAddr[2] = {0}; /* for 6-byte hardware addresses */
    ULONG PhysAddrLen = 6; /* default to length of six bytes */

    vector<char> writable(DestIP_Start.begin(), DestIP_Start.end());
    writable.push_back('\0');
    char * DestIpString = &writable[0];

    BYTE *bPhysAddr;
    int i;

       DestIp = inet_addr(DestIpString);
       //printf("Sending ARP request for IP address: %s\n", DestIpString->c_str());
       cout<<"Sending ARP request for IP address: "<<DestIpString<<endl;
       PhysAddrLen = sizeof(MacAddr);
       memset(&MacAddr, 0xff, sizeof (MacAddr));
       dwRetVal = SendARP(DestIp, SrcIp, &MacAddr, &PhysAddrLen);
       if (dwRetVal == NO_ERROR)
       {
           //QMessageBox::information(this, tr("SendARP"), tr("NO_ERROR"));
           cout<<"Sending ARP request NO_ERROR"<<endl;
#if 1
           bPhysAddr = (BYTE *) &MacAddr;
           if (PhysAddrLen)
           {

               for (i = 0; i < (int) PhysAddrLen; i++)
               {
                   //cout <<  "i:" << i <<endl;
                   if (i == ((int)PhysAddrLen - 1))
                   {
                       printf("%.2X\n", (int) bPhysAddr[i]);
                       //cout << hex <<  uppercase << bPhysAddr[i] <<endl;//hex <<  uppercase <<
                   }
                   else
                   {
                       printf("%.2X-", (int) bPhysAddr[i]);
                       //cout << hex <<  uppercase << bPhysAddr[i]<<"-";//
                   }
               }

           }
           else
           {
               cout << "Warning: SendArp completed successfully, but returned length=0" <<endl;
               //QMessageBox::information(this, tr("SendARP"), tr("returned length=0"));
           }
#endif
           return 1;
       }
       else
       {
           cout << "Error: SendArp failed with error:"<< dwRetVal <<endl;
           switch (dwRetVal)
           {
           case ERROR_GEN_FAILURE:
               /*This error can occur if destination IPv4 address could not be reached
                * because it is not on the same subnet or
                * the destination computer is not operating.*/
               //QMessageBox::information(this, tr("SendARP"), tr("ERROR_GEN_FAILURE"));
               cout << "ERROR_GEN_FAILURE" <<endl;
               break;
           case ERROR_INVALID_PARAMETER:
               //QMessageBox::information(this, tr("SendARP"), tr("ERROR_INVALID_PARAMETER"));
               cout << "ERROR_INVALID_PARAMETER" <<endl;
               break;
           case ERROR_INVALID_USER_BUFFER:
               //QMessageBox::information(this, tr("SendARP"), tr("ERROR_INVALID_USER_BUFFER"));
               cout << "ERROR_INVALID_USER_BUFFER" <<endl;
               break;
           case ERROR_BAD_NET_NAME:
               //QMessageBox::information(this, tr("SendARP"), tr("ERROR_GEN_FAILURE"));
               cout << "ERROR_GEN_FAILURE" <<endl;
               break;
           case ERROR_BUFFER_OVERFLOW:
               //QMessageBox::information(this, tr("SendARP"), tr("ERROR_BUFFER_OVERFLOW"));
               cout << "ERROR_BUFFER_OVERFLOW" <<endl;
               break;
           case ERROR_NOT_FOUND:
               //QMessageBox::information(this, tr("SendARP"), tr("ERROR_NOT_FOUND"));
               cout << "ERROR_NOT_FOUND" <<endl;
               break;
           default:
               cout << endl;
               break;
           }
       }
       return 0;

}

// This Function Checks If A Remote Port Is Open
int PortCheck(string ip, int port)
{
    int iError;
    SOCKADDR_IN  sin;
    unsigned long       blockcmd = 1;
    SOCKET sock;
    WSADATA wsdata;
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD(2,2);
    iError=WSAStartup(wVersionRequested,&wsdata);
    if (iError != NO_ERROR || iError==1){
        cerr <<"Error initializing winsock.dll" << endl;
        WSACleanup();
        return(-1);
    }
    // Define Socket & Make Sure Its Valid
    sock = socket(AF_INET, SOCK_STREAM, 0);
    //sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        cout << "Bad Socket, Was Winsock Initialized?" << endl;
        return -1;
    }

    // Setup Winsock Struct
    memset(&sin,0,sizeof(sin));
    sin.sin_family              = AF_INET;
    sin.sin_addr.S_un.S_addr    = inet_addr(ip.c_str());    // Update this function to be safe
    sin.sin_port                = htons(port);

    // Set Socket To Non-Blocking
    ioctlsocket(sock, FIONBIO, &blockcmd);

    // Make Connection
    connect(sock, (struct sockaddr *)&sin, sizeof(sin));

    // Setup Timeout
    TIMEVAL tv;
    tv.tv_sec = 1; // Seconds
    tv.tv_usec = 0;

    // Setup Result Set
    FD_SET  rset;
    FD_ZERO(&rset);
    FD_SET(sock, &rset);

    // Move Pointer
    int i = select(0, 0, &rset, 0, &tv);

    // Close Socket
    closesocket(sock);

    // Return Result
    if (i <= 0) {
        cout << "Check Port false" << endl;
        return -2;
    }
    else {
        cout << "Check Port true" << endl;
        return 1;
    }
}

void MainWindow::on_bSearch_clicked()
{
    //lineEdit_IP->setEnabled(false);
    //lineEdit_Scan_End->setEnabled(false);
    string DestIP_Start = lineEdit_IP->text().toLocal8Bit().constData();
    string DestIP_End   = lineEdit_Scan_End->text().toLocal8Bit().constData();
    int RMS_Port = lineEdit_RMS_Port->text().toInt();
    int i =0,exist=0,last_one=0;
    comboBox_Live_RMS->clear();
    //mutex m;
    while(1){
        //m.lock();
        if(DestIP_Start.compare(DestIP_End)==0)
            last_one=1;
        //m.unlock();
        cout<< "Before send_ARP_to_IP()" << endl;
        //QMessageBox::information(this, tr("DestIpString"), DestIP_Start.c_str());
        if(send_ARP_to_IP(DestIP_Start)==1){
            cout<< "send_ARP_to_IP() ret 1" << endl;
            //QMessageBox::information(this, tr("Live Poleg"), tr("IP is alive"));
            if(PortCheck(DestIP_Start,RMS_Port) == 1){
                cout<< "Port: ["<< RMS_Port <<"] is opened" << endl;
                //QMessageBox::information(this, tr("Live Poleg"), tr("RMS is working"));
                comboBox_Live_RMS->addItem(DestIP_Start.c_str(),i);
                comboBox_Live_RMS->setCurrentIndex(0);
                exist++;
            }else
                cout<< "Port is NOT opened: " << RMS_Port <<endl;
        }
        //m.lock();
        if(last_one==1)
            break;
        else
            i++;
        AdvGetNextIPPointer(&DestIP_Start);
        //m.unlock();
    }

    if(exist>0){
        search_live_rms_done=1;
        check_devie_file();
        comboBox_Live_RMS->setEnabled(true);
        comboBox_Export_Type->setEnabled(true);
        comboBox_Port->setEnabled(true);
        //bSearch->setEnabled(false);
        setReadWriteButtonState();
    }else
        QMessageBox::information(this, tr("Info"), tr("No RMS is on line"));
}

// getLogicalDrives sets cBoxDevice with any logical drives found, as long
// as they indicate that they're either removable, or fixed and on USB bus
void MainWindow::getLogicalDrives()
{
    // GetLogicalDrives returns 0 on failure, or a bitmask representing
    // the drives available on the system (bit 0 = A:, bit 1 = B:, etc)
    unsigned long driveMask = GetLogicalDrives();
    int i = 0;
    ULONG pID;

    cboxDevice->clear();

    while (driveMask != 0)
    {
        if (driveMask & 1)
        {
            // the "A" in drivename will get incremented by the # of bits
            // we've shifted
            char drivename[] = "\\\\.\\A:\\";
            drivename[4] += i;
            if (checkDriveType(drivename, &pID))
            {
                cboxDevice->addItem(QString("[%1:\\]").arg(drivename[4]), (qulonglong)pID);
            }
        }
        driveMask >>= 1;
        cboxDevice->setCurrentIndex(0);
        ++i;
    }
}

// support routine for winEvent - returns the drive letter for a given mask
//   taken from http://support.microsoft.com/kb/163503
char FirstDriveFromMask (ULONG unitmask)
{
    char i;

    for (i = 0; i < 26; ++i)
    {
        if (unitmask & 0x1)
        {
            break;
        }
        unitmask = unitmask >> 1;
    }

    return (i + 'A');
}

// register to receive notifications when USB devices are inserted or removed
// adapted from http://www.known-issues.net/qt/qt-detect-event-windows.html
bool MainWindow::nativeEvent(const QByteArray &type, void *vMsg, long *result)
{
    Q_UNUSED(type);
    MSG *msg = (MSG*)vMsg;
    if(msg->message == WM_DEVICECHANGE)
    {
        PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;
        switch(msg->wParam)
        {
        case DBT_DEVICEARRIVAL:
            if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                if(DBTF_NET)
                {
                    char ALET = FirstDriveFromMask(lpdbv->dbcv_unitmask);
                    // add device to combo box (after sanity check that
                    // it's not already there, which it shouldn't be)
                    QString qs = QString("[%1:\\]").arg(ALET);
                    if (cboxDevice->findText(qs) == -1)
                    {
                        ULONG pID;
                        char longname[] = "\\\\.\\A:\\";
                        longname[4] = ALET;
                        // checkDriveType gets the physicalID
                        if (checkDriveType(longname, &pID))
                        {
                            cboxDevice->addItem(qs, (qulonglong)pID);
                            //setReadWriteButtonState();
                        }
                    }
                }
            }
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                if(DBTF_NET)
                {
                    char ALET = FirstDriveFromMask(lpdbv->dbcv_unitmask);
                    //  find the device that was removed in the combo box,
                    //  and remove it from there....
                    //  "removeItem" ignores the request if the index is
                    //  out of range, and findText returns -1 if the item isn't found.
                    cboxDevice->removeItem(cboxDevice->findText(QString("[%1:\\]").arg(ALET)));
                    //setReadWriteButtonState();
                }
            }
            break;
        } // skip the rest
    } // end of if msg->message
    *result = 0; //get rid of obnoxious compiler warning
    return false; // let qt handle the rest
}

void get_request_ip(string *host, int n)
{
    struct in_addr paddr;

    // Convert String To ULONG
    u_long addr1 = inet_addr(host->c_str());

    // Convert ULONG To Network Byte Order
    addr1 = ntohl(addr1);

    // Incremement By n
    addr1 += n;

    // Convert Network Byte Order Back To ULONG
    addr1 = htonl(addr1);

    // Convert ULONG Back To in_addr Struct
    paddr.S_un.S_addr = addr1;

    // Convert Back To String
    *host = inet_ntoa(paddr);
}

string
dvformat (const char *fmt, va_list ap)
{
    // Allocate a buffer on the stack that's big enough for us almost
    // all the time.
    size_t size = 1024;
    char buf[size];

    // Try to vsnprintf into our buffer.
    va_list apcopy;
    va_copy (apcopy, ap);
    int needed = vsnprintf (&buf[0], size, fmt, ap);
    // NB. On Windows, vsnprintf returns -1 if the string didn't fit the
    // buffer.  On Linux & OSX, it returns the length it would have needed.

    if ((size_t)needed <= size && needed >= 0) {
        // It fit fine the first time, we're done.
        return std::string (&buf[0]);
    } else {
        // vsnprintf reported that it wanted to write more characters
        // than we allotted.  So do a malloc of the right size and try again.
        // This doesn't happen very often if we chose our initial size
        // well.
        vector <char> buf;
        size = needed;
        buf.resize (size);
        needed = vsnprintf (&buf[0], size, fmt, apcopy);
        return string (&buf[0]);
    }
}

string
dsformat (const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    string buf = dvformat (fmt, ap);
    va_end (ap);
    return buf;
}

void dLog(string message){
        cerr<<"[*] "<<message<<endl;
}


void MainWindow::on_bStart_clicked()
{
   login_get_sid=0;
   connected_to_wss=0;
   if(comboBox_Export_Type->currentData().toInt()==0){//Export Device
       if(cboxDevice->count() > 0){

            QString device = "\\\\\.\\C:";

            device.replace(QRegExp("C"), cboxDevice->currentText().at(1));

            //QMessageBox::information(this, tr("Info"), device);

            status = STATUS_SERVER;
            string IP = lineEdit_IP->text().toLocal8Bit().constData();
            get_request_ip(&IP,comboBox_Live_RMS->currentData().toInt());

            int arg_counter=0;
            char **argv_nbd;
            argv_nbd = new char *[10];
            QString opt_string;
            //Program Name opt
            opt_string = "nbd-server";
            argv_nbd[0] = new char[opt_string.capacity()];
            strncpy(argv_nbd[0],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

            //device opt
            opt_string = "-f" + device;
            argv_nbd[1] = new char[opt_string.capacity()];
            strncpy(argv_nbd[1],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

            //IP opt
            opt_string = "-c" + lineEdit_IP->text();
            argv_nbd[2] = new char[opt_string.capacity()];
            strncpy(argv_nbd[2],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

            //Port opt
            opt_string = "-p" + comboBox_Port->currentText();
            argv_nbd[3] = new char[opt_string.capacity()];
            strncpy(argv_nbd[3],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

#if 1
            for(int i=0;i<arg_counter;i++)
                dLog(dsformat("argv_nbd(%d):%s ",i,argv_nbd[i]));
#endif

            if(nbd_server_start(arg_counter,argv_nbd)<0){
                return;
            }

            bCancel->setEnabled(true);
            bSearch->setEnabled(false);//Search Live IP
            bStart->setEnabled(false);//Connect to RMS
            comboBox_Export_Type->setEnabled(false);
            comboBox_Port->setEnabled(false);
            comboBox_Live_RMS->setEnabled(false);
            lineEdit_IP->setEnabled(false);
            lineEdit_Scan_End->setEnabled(false);
            lineEdit_RMS_Port->setEnabled(false);
       }else{
           QMessageBox::critical(this, tr("File Info"), tr("Please insert a device to export to poleg"));
       }

    }else if(comboBox_Export_Type->currentData().toInt()==1){//Export Image
        QString myFile;
        if (!leFile->text().isEmpty())
        {
            myFile = leFile->text();
            QFileInfo fileinfo(myFile);
            if (fileinfo.path()=="."){
                myFile=(myHomeDir + "/" + leFile->text());
            }

            //QMessageBox::information(this, tr("Info"), myFile);

            status = STATUS_SERVER;
            string IP = lineEdit_IP->text().toLocal8Bit().constData();
            get_request_ip(&IP,comboBox_Live_RMS->currentData().toInt());

            int arg_counter=0;
            char **argv_nbd;
            argv_nbd = new char *[10];
            QString opt_string;
            //Program Name opt
            opt_string = "nbd-server";
            argv_nbd[0] = new char[opt_string.capacity()];
            strncpy(argv_nbd[0],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

            //export file opt
            opt_string = "-f" + myFile;
            argv_nbd[1] = new char[opt_string.capacity()];
            strncpy(argv_nbd[1],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

            //IP opt
            opt_string = "-c" + lineEdit_IP->text();
            argv_nbd[2] = new char[opt_string.capacity()];
            strncpy(argv_nbd[2],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

            //Port opt
            opt_string = "-p" + comboBox_Port->currentText();
            argv_nbd[3] = new char[opt_string.capacity()];
            strncpy(argv_nbd[3],opt_string.toLocal8Bit().data(),opt_string.capacity());
            arg_counter++;

#if 0
            for(int i=0;i<arg_counter;i++)
                dLog(dsformat("argv_nbd(%d):%s ",i,argv_nbd[i]));
#endif

            nbd_server_start(arg_counter,argv_nbd);

            bCancel->setEnabled(true);
            bSearch->setEnabled(false);
            bStart->setEnabled(false);
            comboBox_Export_Type->setEnabled(false);
            comboBox_Port->setEnabled(false);
            lineEdit_IP->setEnabled(false);
            lineEdit_Scan_End->setEnabled(false);
            lineEdit_RMS_Port->setEnabled(false);
            comboBox_Live_RMS->setEnabled(false);
        }else
        {
            QMessageBox::critical(this, tr("File Info"), tr("Please specify a image file to transfer to poleg"));
        }
    }
}

void MainWindow::on_lineEdit_IP_textChanged(const QString &arg1)
{
    lineEdit_Scan_End->setText(lineEdit_IP->text());
}

void MainWindow::on_bCancel_clicked()
{
    bStart->setEnabled(true);
    comboBox_Export_Type->setEnabled(true);
    bUmountUSB->setEnabled(false);
    bMountUSB->setEnabled(false);
    bCancel->setEnabled(false);
    lineEdit_IP->setEnabled(true);
    lineEdit_Scan_End->setEnabled(true);
    lineEdit_RMS_Port->setEnabled(true);
    bSearch->setEnabled(true);
    comboBox_Live_RMS->setEnabled(true);
    comboBox_Port->setEnabled(true);
    wsSocket_vmws->disconnectFromHost();
    wsSocket_vmws->disconnect();
    wsSocket_vmws->abort();
    sslSocket->close();
    sslSocket->abort();

    if(lock_status!=0)
        handle_unlockio(fh);
    if(fh)
        CloseHandle(fh);
    connected_to_wss=0;
}


void MainWindow::on_bExit_clicked()
{
    wsSocket_vmws->disconnectFromHost();
    wsSocket_vmws->disconnect();
    wsSocket_vmws->abort();
    sslSocket->close();
    sslSocket->abort();
    if(lock_status!=0)
        handle_unlockio(fh);
    if(fh)
        CloseHandle(fh);
    wsSocket_vmws->disconnectFromHost();
    bCancel->setEnabled(false);
    bSearch->setEnabled(true);//Search Live IP
    bStart->setEnabled(true);//Connect to RMS
}


void MainWindow::on_bMountUSB_clicked()
{
    bUmountUSB->setEnabled(true);
    bMountUSB->setEnabled(false);
    http_get(ip_addr,"/enablemstgws");
}

void MainWindow::on_bUmountUSB_clicked()
{
    bUmountUSB->setEnabled(false);
    bMountUSB->setEnabled(true);
    http_get(ip_addr,"/disablemstgws");
}

void MainWindow::on_lineEdit_PID_editingFinished()
{

}

void MainWindow::on_lineEdit_PPW_editingFinished()
{

}

void MainWindow::on_lineEdit_PID_textEdited(const QString &arg1)
{
    setReadWriteButtonState();
}

void MainWindow::on_lineEdit_PPW_textEdited(const QString &arg1)
{
    setReadWriteButtonState();
}
