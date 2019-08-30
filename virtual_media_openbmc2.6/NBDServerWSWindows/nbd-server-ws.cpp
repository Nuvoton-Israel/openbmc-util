#include <string.h>
#include <cstdlib>
#include <iostream>
#include <errno.h>
#include <winsock2.h>
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <getopt.h>
#include <fstream>
#include <algorithm>
#include <stdint.h>
#include <cstdarg>
#include <vector>
#include <QElapsedTimer>
#include <QQueue>
#include <QtCore>
#include <QPushButton>
#include <QMutex>
#include "globaldebug.h"
#include "mainwindow.h"
#include "NBDProtocol.h"

QT_USE_NAMESPACE

using namespace std;
bool debug=false;
bool quiet=false;
bool bMemory = false;

int partitionNo=0;
ofstream debugFile;
QMutex mutex;
QElapsedTimer timer;
//pmem windows memory driver defines
#define PMEM_DEVICE_NAME "pmem"
#define PMEM_WRITE_MODE 1
// ioctl to get memory ranges from winpmem driver.
#define PMEM_INFO_IOCTRL CTL_CODE(0x22, 0x100, 0, 3)
#define PMEM_CTRL_IOCTRL CTL_CODE(0x22, 0x101, 0, 3)
#define PMEM_WRITE_ENABLE CTL_CODE(0x22, 0x102, 0, 3)
// Available modes
#define PMEM_MODE_IOSPACE 0
#define PMEM_MODE_PHYSICAL 1
#pragma pack(2)
struct pmem_info_runs {
	__int64 start;
	__int64 length;
};

#pragma pack(2)
struct pmem_info_ioctrl {
    __int64 cr3;
    __int64 kdbg;
    __int32 number_of_runs;
    struct pmem_info_runs runs[1];
};


union MyUnion {
   int64_t i64;
   int32_t i32[2];
};

int64_t htonll(int64_t hostFormatInt64)
{
   MyUnion u;
   u.i64 = hostFormatInt64;
   int32_t temp = u.i32[0];
   u.i32[0] = htonl(u.i32[1]);
   u.i32[1] = htonl(temp);
   return u.i64;
}

int64_t ntohll(int64_t networkFormatInt64)
{
   MyUnion u;
   u.i64 = networkFormatInt64;
   int32_t temp = u.i32[0];
   u.i32[0] = ntohl(u.i32[1]);
   u.i32[1] = ntohl(temp);
   return u.i64;
}

bool logged_oversized=false;
/** global flags */
int glob_flags=0;


void usage(char *prog)
{
     cout<< prog << " v3.1"<<endl;
     cout<<" -c     Client IP address to accept connections from"<<endl;
     cout<<" -p     Port to listen on (60000 by default)"<<endl;
     cout<<" -f     File to serve ( \\\\.\\PHYSICALDRIVE0 or \\\\.\\pmem for example)"<<endl;  //escaping \'s should be read as \\.\:
     cout<<" -n     Partition on disk to serve (0 if not specified), -n all to serve all partitions"<<endl;
     cout<<" -w     Enable writing (disabled by default)"<<endl;
     cout<<" -d     Enable debug messages"<<endl;
     cout<<" -q     Be Quiet..no messages"<<endl;
     cout<<" -h     This help text"<<endl;
}

int error_mapper(DWORD winerr)
{
    switch(winerr){
    case ERROR_ACCESS_DENIED:
    case ERROR_WRITE_PROTECT:
        return EACCES;

    case ERROR_WRITE_FAULT:
    case ERROR_READ_FAULT:
    case ERROR_GEN_FAILURE:
        return EIO;

    case ERROR_SEEK:
    case ERROR_NEGATIVE_SEEK:
        return ERANGE;

    case ERROR_BAD_UNIT:
    case ERROR_NOT_READY:
    case ERROR_CRC:
    case ERROR_SECTOR_NOT_FOUND:
    case ERROR_DEV_NOT_EXIST:
    case ERROR_DISK_CHANGE:
    case ERROR_BUSY:
    case ERROR_CAN_NOT_COMPLETE:
    case ERROR_UNRECOGNIZED_VOLUME:
    case ERROR_DISK_RECALIBRATE_FAILED:
    case ERROR_DISK_OPERATION_FAILED:
    case ERROR_DISK_RESET_FAILED:
        return EIO;
    }

    return EINVAL; /* what else? */
}

int MainWindow::WRITE_TEXT(char *wherefrom, int howmuch)
{
    char *tmp=(char *)malloc(howmuch+1);
    memset(tmp,0,howmuch+1);
    memcpy(tmp,wherefrom,howmuch);
    QString qtStrData;
    qtStrData = QString::fromUtf8(tmp);
    wsSocket_vmws->write(qtStrData);
    free(tmp);
    return howmuch;
}

int MainWindow::WRITE_BINARY(char *wherefrom, int howmuch)
{
    QByteArray sendoutdata;
    int i;
    for(i=0;i<howmuch;i++)
        sendoutdata.append(wherefrom[i]);
    wsSocket_vmws->write(sendoutdata);

    return howmuch;
}

void MainWindow::send_reply(uint32_t opt, uint32_t reply_type, ssize_t datasize, const void* data) {
    struct {
        uint64_t magic;
        uint32_t opt;
        uint32_t reply_type;
        uint32_t datasize;
    } __attribute__ ((packed)) header = {
        htonll(0x3e889045565a9LL),
        htonl(opt),
        htonl(reply_type),
        htonl(datasize),
    };

    if(datasize < 0) {
        datasize = strlen((char*)data);
        header.datasize = htonl(datasize);
    }
    WRITE_BINARY((char*)&header, sizeof(header));
    if(datasize != 0) {
        WRITE_BINARY((char*)data, datasize);
    }
}


/**
 * Consume data from a socket that we don't want
 *
 * @param c the client to read from
 * @param len the number of bytes to consume
 * @param buf a buffer
 * @param bufsiz the size of the buffer
 **/
static inline void consume(struct ws_client * clp, size_t len) {


}

/**
 * Consume a length field and corresponding payload that we don't want
 *
 * @param c the client to read from
 **/
static inline void consume_len(struct ws_client * clp) {
    uint32_t len;

    //READ(clp, (char *)&len, sizeof(len));
    len = ntohl(len);
    consume(clp, len);
}

int MainWindow::negotiate(void) {
	uint16_t smallflags = NBD_FLAG_FIXED_NEWSTYLE | NBD_FLAG_NO_ZEROES;  
	UCHAR magic[8];

    if (WRITE_BINARY((char *)"NBDMAGIC", 8) != 8) {
        qInfo("Failed to send NBDMAGIC string");
	}

	// some other magic value
	magic[0] = 0x49;
	magic[1] = 0x48;
	magic[2] = 0x41;
	magic[3] = 0x56;
	magic[4] = 0x45;
	magic[5] = 0x4f;
    magic[6] = 0x50;
	magic[7] = 0x54;

    if (WRITE_BINARY((char *)magic, 8) != 8) {
        qInfo("Failed to send 2nd magic string.");
        return -1;
	}

    smallflags = htons(smallflags);
    if (WRITE_BINARY((char *)&smallflags, sizeof(smallflags)) < 0) {
        qInfo("Failed to send smallflags.");
        return -1;
	}
    nego_state=1;
    return 1;

}

#define rbuf_size  128 * 1024
UCHAR *rbuf_ptr =NULL;

int handle_lockio(HANDLE fh)
{
    int ret = 0;
    int counter = 70;
    DWORD bytesReturned;
    do {
        ret = DeviceIoControl(
                fh,                 // handle to a volume
                FSCTL_LOCK_VOLUME, // dwIoControlCode
                NULL,              // lpInBuffer
                0,                 // nInBufferSize
                NULL,              // lpOutBuffer
                0,                 // nOutBufferSize
                &bytesReturned,    // number of bytes returned
                NULL        // OVERLAPPED structure
        );
       Sleep(100);
       counter--;
    } while (ret == 0 && counter > 0);
    return ret;
}

void handle_unlockio(HANDLE fh)
{
    DWORD bytesReturned;


    (
        fh,              // handle to a volume
        FSCTL_UNLOCK_VOLUME, // dwIoControlCode
        NULL,              // lpInBuffer
        0,                 // nInBufferSize
        NULL,              // lpOutBuffer
        0,                 // nOutBufferSize
        &bytesReturned,    // number of bytes returned
        NULL        // OVERLAPPED structure
    );
}
int total_write =0;
void MainWindow::handle_write(char *qbuffer, ULONG receive)
{
    struct nbd_reply reply;
    const int writeBlockSize = 512;

    ULONG bWritten;
    ULONG buflen = 0;
    ULONG total_write_this_time=receive;
    UCHAR buffer[BUFSIZE];
    ULONG nb;
    int err = 0;

    //mutex.lock();

    reply.magic = htonl(NBD_REPLY_MAGIC);
    reply.error = 0;
    memcpy(reply.handle, write_handle, sizeof(reply.handle));

    if(receive>BUFSIZE){
        QMessageBox::information(this, tr("TODO"), tr("Receive too many data when handle write cmd"));
    }
    memset(buffer, 0x00, BUFSIZE);
    memcpy(buffer, qbuffer, receive);

    while (total_write_this_time > 0)
    {
        buflen = total_write_this_time;

        // write to file;
        if (!allowWrite) {
            qInfo("ignoring write request due to failure of locking device or being pointed at memory");
            total_write_this_time = 0;
            break;
        } else if (total_write_this_time >= writeBlockSize) {
            nb = total_write_this_time & ~(writeBlockSize - 1); //floor to previous 512
            //qInfo("write size(nb):%ld",nb);
            total_write++;
            qInfo("total_write times:%d (%ld)(from:%lld)",total_write,total_write_this_time,write_from.QuadPart);
            if (WriteFile(fh, buffer, nb, &bWritten, NULL) == 0 || nb != bWritten) {
                qInfo("Failed to write %ld bytes  (%d", nb, GetLastError());
                err = error_mapper(GetLastError());
                reply.error = htonl(err);
                goto error;
            }

            //adjust buffer (pending data)
            if (total_write_this_time > bWritten) {
                qInfo("total_write_this_time(%ld) is bigger than bWritten(%ld)",total_write_this_time,bWritten);
                memmove(&buffer[0], &buffer[bWritten], total_write_this_time - bWritten);
            }

            total_write_this_time -= bWritten;
            //qInfo("(last:%ld) (bWritten:%ld)",total_write_this_time,bWritten);

        }else if( total_write_this_time > 0) {
            qInfo("write size(last):%ld",total_write_this_time);
            if (WriteFile(fh, buffer, total_write_this_time, &bWritten, NULL) == 0 || bWritten != total_write_this_time) {
                qInfo("------------------------------Failed to write the reset %ld bytes to %d", total_write_this_time, GetLastError());
                err = error_mapper(GetLastError());
                reply.error = htonl(err);
                goto error;
            }
            total_write_this_time = 0;
        }

    }
    write_len -= receive;
    //qInfo("write_len:%ld",write_len);
    if(write_len==0){
        if (WRITE_BINARY((char *)&reply, sizeof(reply)) < 0) {
            qInfo("Failed to send through socket.");
        }
        nego_state=6;
        qInfo("change state back to 6");
    }
    //mutex.unlock();
    return;
error:
    // send 'ack'
    if (WRITE_BINARY((char *)&reply, sizeof(reply)) < 0) {
        qInfo("Failed to send through socket.");
    }
    nego_state=6;
    mutex.unlock();
}

void MainWindow::handle_read(struct nbd_request *req)
{
    struct nbd_reply reply;
    ULONG len;
    ULONG dummy;
    ULONG nb;
    UCHAR rbuffer[rbuf_size];
    LARGE_INTEGER from;

    reply.magic = htonl(0x67446698);
    reply.error = 0;
    memcpy(reply.handle, req->handle, sizeof(reply.handle));
    if (WRITE_BINARY((char *)&reply, sizeof(reply)) <0) {
        qInfo("Failed to send through socket.");
        goto error;
    }

    mutex.lock();

    from.QuadPart = ntohll(req->from);
    len = ntohl(req->len);
    //qInfo("------------------------REQ from 0x%lx len %ld", from.QuadPart, len);

    while (len > 0) {
        nb = len;

        SetFilePointerEx(fh, from, NULL, FILE_BEGIN);
        // read nb to buffer;
        if (ReadFile(fh, rbuffer, nb, &dummy, NULL) == 0 && dummy != nb) {
            qInfo("Failed to read %u", GetLastError());
            goto error;
        }

        rbuf_ptr = &rbuffer[0];

        //qInfo("sending Rdata %d", nb);

        // send through socket
        dummy = WRITE_BINARY((char *)rbuf_ptr, nb);
        if (dummy != nb) {
            qInfo("Connection dropped while sending block.");
            goto error;
        }

        len -= nb;
    }

error:
    mutex.unlock();
}

static int check_length(struct nbd_request *req, uint64_t fsize)
{
    uint64_t from = ntohll(req->from);
    uint32_t len = ntohl(req->len);

    if (from + len < from) {
        qInfo("[64 bit overflow!]");
        return -1;
    }

    if (len > BUFSIZE - sizeof(struct nbd_reply)) {
        len = BUFSIZE - sizeof(struct nbd_reply);
        qInfo("[over size!]");
        return -1;
    }

    if (len == 0 ||
        len % 512 != 0 ||
        len + from > fsize) {
        qInfo("Invalid request: From:%lld Len:%lu", from, len);
        return -1;
    }

    return 0;
}

int MainWindow::handle_request(struct nbd_request req)
{
    uint16_t command;
    int ret = 0;

    if (req.magic != htonl(NBD_REQUEST_MAGIC)) {
        QMessageBox::information(this, tr("Error"), tr("Request Magic Not Match"));
        qInfo("Unexpected protocol version! (got: %x, expected: 0x25609513) \n", req.magic);
        return -1;
    }

    command = ntohl(req.type) & 0x0000ffff;

    if (command != NBD_CMD_DISC && check_length(&req, fsize.QuadPart) < 0){
        QMessageBox::information(this, tr("Error"), tr("Request Range Error"));
        return -1;
    }

    switch(command) {
        case NBD_CMD_WRITE:
            write_from.QuadPart = ntohll(req.from);
            write_len = ntohl(req.len);
            memcpy(write_handle, req.handle, sizeof(req.handle));
            //qInfo("(qws)Write REQ (write_from.QuadPart: 0x%llx) len %lx", write_from.QuadPart, write_len);
            //qInfo("(qws)Write REQ (write_from.HighPart: 0x%lx)(write_from.LowPart: 0x%lx)", write_from.HighPart, write_from.LowPart);
            SetFilePointerEx(fh, write_from, NULL, FILE_BEGIN);
            nego_state=7;
            break;
        case NBD_CMD_READ:
            handle_read(&req);
            break;
        case NBD_CMD_DISC:
            wsSocket_vmws->disconnectFromHost();
            //m_webSocket.close();
            if(lock_status!=0)
                handle_unlockio(fh);
            if(fh)
                CloseHandle(fh);
            QMessageBox::information(this, tr("Info"), tr("Receive Disconnected Request"));
            break;
        default:
            break;
    }

     return ret;
}

int MainWindow::opendevice(void){

    if(connection_flag==0){
        qInfo("NBD Server is not connected with websocket proxy server");
        return -1;
    }

    if (allowWrite)
        fh = CreateFile(dev_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL , NULL);
    else {
        qInfo("opening read-only:%s",dev_name);
        fh = CreateFile(dev_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (fh == INVALID_HANDLE_VALUE) {
        qInfo("Error opening file %s: %u", dev_name, GetLastError());
        return -1;
    }

    if (strncasecmp(dev_name, "\\\\.\\", 4 ) == 0) {
        //assume a volume name like \\.\C: or \\.\HarddiskVolume1
        GET_LENGTH_INFORMATION pLength;
        DWORD dwplSize = sizeof(GET_LENGTH_INFORMATION);
        DWORD dwplBytesReturn = 0;
        if (DeviceIoControl(fh, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,(LPVOID)&pLength,dwplSize,&dwplBytesReturn,NULL)){
            qInfo("VolumeLength--1: %lld",pLength.Length.QuadPart);
            fsize.QuadPart = pLength.Length.QuadPart;
            qInfo("VolumeLength--2: %lld",fsize.QuadPart);
        } else {
            qInfo("Cannot determine Volume length. Error: %u", GetLastError());
            return -1;
        }
    } else {
        /* plain file */
        if (GetFileSizeEx(fh, &fsize) == 0) {
            qInfo("Failed to obtain filesize info: %u", GetLastError());
            return -1;
        }else
            qInfo("VolumeLength of plain file: %lld",fsize);
    }

    return 1;
}

int MainWindow::nbd_server_start(int argc, char *argv[])
{
    /**The variable optind is the index of the next element to be processed
      * in argv.  The system initializes this value to 1.  The caller can
      * reset it to 1 to restart scanning of the same argv, or when scanning
      * a new argument vector.
     **/
    optind =1;
    char ch;
    ifstream nbdfile;
    connection_flag=0;
    nego_done=0;
    writeable=true;
    debug=true;

    while ((ch=getopt(argc,argv,"c::p::f::n:t::e::r::k::hwdq")) != EOF){
    switch(ch)
    {
        case 'c':
            //nbdclient.assign(optarg);
            qInfo("nbdclient:%s ",optarg);
            memset(ip_addr,0,sizeof(ip_addr));
            memcpy(ip_addr, optarg, strlen(optarg));
            break;
        case 'd':
            debug=true;
            break;
        case 'q':
            quiet=true;
            break;
        case 'w':
            qInfo("allowWrite is set...");
            allowWrite=true;
            break;
        case 'p':
            port=atoi(optarg);
            qInfo("1.port:%d",port);
            break;
        case 'f':
            memset(dev_name,0,sizeof(dev_name));
            memcpy(dev_name, optarg, strlen(optarg));
            qInfo("1.nbdfilename:%s",optarg);
            break;
        case 'h':
             usage(argv[0]);
             return -1;
        default:
            usage(argv[0]);
            return -1;
    }}

    qInfo("open:%s", dev_name);
    nbdfile.open(dev_name,ifstream::in|ifstream::binary);

    if ( nbdfile.is_open() )
    {
        qInfo("File opened, valid file");
        nbdfile.close();
    }
    else
    {
        cerr << "Error: " << strerror(errno)<<endl;
        qInfo("Error opening file: %s",dev_name);
        QMessageBox::information(this, tr("Open File Error"), dev_name);
        return -1;
     }

    char ws_srv[128]="";
#ifdef CS20_VM
    snprintf(ws_srv,128,"wss://%s:%d/vmws",ip_addr,port);
    wsSocket_vmws->setWSPath("/vmws");
#else
    snprintf(ws_srv,128,"wss://%s:%d/vm/0/0",ip_addr,port);
    wsSocket_vmws->setWSPath("/vm/0/0");
#endif
    QUrl url;
    url.setUrl(ws_srv);
    qInfo("Connect to websocket server5: %s",ws_srv);
    Connectedto(url);

    return 1;
}






