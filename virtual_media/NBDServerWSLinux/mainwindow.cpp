#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "nbdprotocol.h"

#include <QFileInfo>
#include <QDirIterator>
#include <QtWidgets>

void MainWindow::check_usb_device(QString device_list){
    //QMessageBox::information(this, tr("Info"), tr("New USB"));
    //qInfo("%s",device_list.toUtf8().constData());
    SDDevices->clear();
    QDir *DevDir=new QDir("/dev","sd*",QDir::Name,QDir::System);
    SDDevices->addItems(DevDir->entryList());
}

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent)
 //   ,ui(new Ui::MainWindow)
{
    //ui->setupUi(this);
    setupUi(this);

    wsSocket_vmws = new QtWebsocket::QWsSocket(this, NULL, QtWebsocket::WS_V13);
    wsSocket_vmws->setWSPath("/vmws");
    allowWrite=true;

    QDir *DevDir=new QDir("/dev","sd*",QDir::Name,QDir::System);
    SDDevices->addItems(DevDir->entryList());
    login_get_sid=0;

    QFileSystemWatcher *FSwatcher = new QFileSystemWatcher(this);
    FSwatcher->addPath("/dev");
    QObject::connect(FSwatcher, SIGNAL(directoryChanged(QString)), this, SLOT(check_usb_device(QString)));
    connected_to_wss=0;
}

MainWindow::~MainWindow()
{
    //delete ui;
}


void MainWindow::on_StartB_clicked()
{

     //QMessageBox::information(this, tr("Info"), tr("start NBD Server"));
     //QString device = "/home/medad/QT/sd922.img";
     //QString device = "/dev/sdb";

    StartB->setEnabled(false);
    comboBoxBMC->setEnabled(false);
    Line_Account->setEnabled(false);
    Line_Password->setEnabled(false);
    login_get_sid=0;
    connected_to_wss=0;

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
     if(radioImage->isChecked())
        opt_string = "-f" + le_file->text();
     else{
        QString device = SDDevices->currentText();
        opt_string = "-f/dev/" + device;
     }
     argv_nbd[1] = new char[opt_string.capacity()];
     strncpy(argv_nbd[1],opt_string.toLocal8Bit().data(),opt_string.capacity());
     arg_counter++;

     //IP opt
     opt_string = "-c" + comboBoxBMC->currentText();
     argv_nbd[2] = new char[opt_string.capacity()];
     strncpy(argv_nbd[2],opt_string.toLocal8Bit().data(),opt_string.capacity());
     arg_counter++;

     //Port opt
     opt_string = "-p443" ;
     argv_nbd[3] = new char[opt_string.capacity()];
     strncpy(argv_nbd[3],opt_string.toLocal8Bit().data(),opt_string.capacity());
     arg_counter++;

#if 1
     for(int i=0;i<arg_counter;i++)
        qInfo("argv_nbd(%d):%s ",i,argv_nbd[i]);
#endif

     if(nbd_server_start(arg_counter,argv_nbd)<0){
         StartB->setEnabled(true);
         StopB->setEnabled(false);
         MountB->setEnabled(false);
         UnmountB->setEnabled(false);
         comboBoxBMC->setEnabled(true);
         Line_Account->setEnabled(true);
         Line_Password->setEnabled(true);
         return;
     }else{
         qInfo("NBD Server Start successfully");

     }


     //StartB->setEnabled(false);
     //StopB->setEnabled(true);
     //MountB->setEnabled(true);
     //UnmountB->setEnabled(false);
}

void MainWindow::on_StopB_clicked()
{
    //QMessageBox::information(this, tr("Info"), tr("stop"));
    wsSocket_vmws->disconnectFromHost();
    wsSocket_vmws->disconnect();
    wsSocket_vmws->abort();
    connected_to_wss=0;
    if(fh){
        qInfo("Close fh when STop VM");
        ::close(fh);
    }
    StartB->setEnabled(true);
    StopB->setEnabled(false);
    MountB->setEnabled(false);
    UnmountB->setEnabled(false);
    comboBoxBMC->setEnabled(true);
    Line_Account->setEnabled(true);
    Line_Password->setEnabled(true);
}

void MainWindow::on_MountB_clicked()
{
    //QMessageBox::information(this, tr("Info"), tr("Mount"));
    qInfo("Enable MSTG:%s",ip_addr);
    http_get(ip_addr,"/enablemstgws");
    StartB->setEnabled(false);
    StopB->setEnabled(true);
    MountB->setEnabled(false);
    UnmountB->setEnabled(true);
}

void MainWindow::on_UnmountB_clicked()
{
    //QMessageBox::information(this, tr("Info"), tr("UnMount"));
    qInfo("Disable MSTG:%s",ip_addr);
    http_get(ip_addr,"/disablemstgws");
    StartB->setEnabled(false);
    StopB->setEnabled(true);
    MountB->setEnabled(true);
    UnmountB->setEnabled(false);
}

void MainWindow::on_BrowseB_clicked()
{

    // Use the location of already entered file
       QString fileLocation = le_file->text();
       QFileInfo fileinfo(fileLocation);

       // See if there is a user-defined file extension.
       QString fileType;
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
       //else
       //{
       //    dialog.setDirectory(myHomeDir);
       //}

       if (dialog.exec())
       {
           // selectedFiles returns a QStringList - we just want 1 filename,
           //	so use the zero'th element from that list as the filename
           fileLocation = (dialog.selectedFiles())[0];

           if (!fileLocation.isNull())
           {
               radioImage->click();
               le_file->setText(fileLocation);
               //QFileInfo newFileInfo(fileLocation);
               //myHomeDir = newFileInfo.absolutePath();
           }
       }

}

char* AdvGetNextIPPointer(const char* address_string)
{
        // convert the input IP address to an integer
        in_addr_t address = inet_addr(address_string);

        // add one to the value (making sure to get the correct byte orders)
        address = ntohl(address);
        address += 1;
        address = htonl(address);

        // pack the address into the struct inet_ntoa expects
        struct in_addr address_struct;
        address_struct.s_addr = address;

        // convert back to a string
        return inet_ntoa(address_struct);
}

int check_ip_alive(const char* ip){
#if 0
    int exitCode = QProcess::execute("ping", QStringList() << "-cl " << ip);//-cl
    if (0 == exitCode) {
        // it's alive
        qInfo("%s alive",ip);
        return 1;
    } else {
        // it's dead
        qInfo("%s dead",ip);
        return -1;
    }
#else
    QProcess pingProcess;
    QString exec = "nmap";
    QStringList params;
    params << "-p 443 " << ip ;

    pingProcess.start(exec, params);
    pingProcess.waitForFinished(); // sets current thread to sleep and waits for pingProcess end
    QString output(pingProcess.readAllStandardOutput());
    //qInfo("%s",output.toUtf8().constData());

    bool test = output.contains("443/tcp open  https",Qt::CaseInsensitive);   //test=true  CaseSensitive
    if (test){
        qInfo("%s 443 is opened",ip);
        return 1;
    }
    else{
        qInfo("%s 443 is closed",ip);
        return 0;
    }
 #endif

}

void MainWindow::on_SearchB_clicked()
{
    //Line_IP_Start->setEnabled(false);
    //Line_IP_End->setEnabled(false);
    string DestIP_Start = Line_IP_Start->text().toLocal8Bit().constData();
    string DestIP_End   = Line_IP_End->text().toLocal8Bit().constData();

    int i =0,exist=0,last_one=0;
    comboBoxBMC->clear();
    const char *cStr=DestIP_Start.c_str();
    while(1){

           if(strcmp(cStr,DestIP_End.c_str())==0)
               last_one=1;

           //QMessageBox::information(this, tr("DestIpString"), DestIP_Start.c_str());
           if(check_ip_alive(cStr)==1){
               //QMessageBox::information(this, tr("Live Poleg"), tr("IP is alive"));

               comboBoxBMC->addItem(cStr,i);
               comboBoxBMC->setCurrentIndex(0);
               exist++;
           }

           if(last_one==1)
               break;
           else
               i++;
           cStr = AdvGetNextIPPointer(cStr);
    }
    if(exist)
        StartB->setEnabled(true);

    //Line_IP_Start->setEnabled(true);
    //Line_IP_End->setEnabled(true);
}

void MainWindow::on_ExitB_clicked()
{
    //QMessageBox::information(this, tr("Into"), tr("Exit"));
    wsSocket_vmws->disconnectFromHost();
    wsSocket_vmws->disconnect();
    wsSocket_vmws->abort();
    if(fh){
        qInfo("Close fh when Exit VM");
        ::close(fh);
    }
    QCoreApplication::quit();
}
