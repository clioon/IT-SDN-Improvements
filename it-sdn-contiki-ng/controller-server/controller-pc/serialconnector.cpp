/*
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE SOFTWARE PROVIDER OR DEVELOPERS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "serialconnector.h"
#include "sdn-serial-send.h"
#include "sdn-reliability.h"

#include <QTcpSocket>
#include <QSerialPort>

#include <iostream>
#include <iomanip>

SerialConnector::SerialConnector(QObject *parent) : QObject(parent) {

    connect(&serializer, &Serializer::new_packet_available, [this](QByteArray packet){
        if(packet.size() == sizeof(sdn_serial_packet_t)) {
            decodeSerialPacket(SerialConnector::castToPacketType(packet));
        }
    });
}

SerialConnector::SerialConnector(const QString& serial_port, QObject *parent) : SerialConnector(parent) {

    initConnection(serial_port);
}

SerialConnector::SerialConnector(const QString &ip, quint16 port, QObject *parent) : SerialConnector(parent) {

    initConnection(ip, port);
}

SerialConnector::~SerialConnector() {

    if(device) {
        delete device;
    }
}

bool SerialConnector::ready() {

    return isReady;
}

void SerialConnector::requestNodeId() {

    if(ready()) {
        sdn_serial_packet_t packet;
        memset(&packet, 0, sizeof(sdn_serial_packet_t));
        packet.header.msg_type = SDN_SERIAL_MSG_TYPE_ID_REQUEST;
        packet.header.payload_len = 0;
        QByteArray rawBytes = serializer.packetizer(&packet);
        device->write(rawBytes);
    }
}

void SerialConnector::sendGenericPacket(sdn_serial_packet_t *packet) {

    if(ready()) {
        QByteArray rawBytes = serializer.packetizer(packet);
        // unsigned int i;
        // for (i=0; i < rawBytes.length(); i++) {
        //     printf("%02X ",((unsigned char*)rawBytes.constData())[i]);
        // }
        // printf("\n");
        qint64 ret = device->write(rawBytes);
        if (ret == -1) {
            std::cout << "ERROR on device->write" << std::endl;
        //} else {
        //     std::cout << "device->write " << ret << " bytes" << std::endl;
        }
    }
}

void SerialConnector::initConnection(const QString& serial_port) {

    if(device) {
        device->close();
        delete device;
    }

    QSerialPort *serial = new QSerialPort(serial_port);
    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);

    connect(serial, &QSerialPort::errorOccurred, [this]() {
        isReady = false;
        emit state_changed_stringfy("Serial error ocurred.");
    });

    isReady = serial->open(QIODevice::ReadWrite);
    device = serial;
    if (isReady) {
        emit state_changed_stringfy("Serial connected");
    }

    connect(device, &QIODevice::readyRead, this, [this]() {
        data_received();
    });
}

void SerialConnector::initConnection(const QString &ip, quint16 port) {

    if(device) {
        delete device;
    }

    QTcpSocket *socket = new QTcpSocket();

    connect(socket, &QTcpSocket::stateChanged, [this](QAbstractSocket::SocketState state) {
        QString stateString = QString(state == QAbstractSocket::UnconnectedState ? "UnconnectedState" :
                                 state == QAbstractSocket::HostLookupState ? "HostLookupState" :
                                 state == QAbstractSocket::ConnectingState ? "ConnectingState" :
                                 state == QAbstractSocket::ConnectedState ? "ConnectedState" :
                                 state == QAbstractSocket::BoundState ? "BoundState" :
                                 state == QAbstractSocket::ListeningState ? "ListeningState" :
                                 state == QAbstractSocket::ClosingState ? "ClosingState" : "");

        if(state == QAbstractSocket::ConnectedState) {
            isReady = true;
        }
        else {
            isReady = false;
        }

        emit state_changed_stringfy(stateString);
    });

    socket->connectToHost(ip, port);

    device = socket;

    connect(device, &QIODevice::readyRead, this, [this]() {
        data_received();
    });
}

void SerialConnector::data_received() {

    if(device) {
        while(device->bytesAvailable() > 0) {
            char max_data[512];
            int len = device->read(max_data, 512);
            serializer.processBytes(max_data, len);
        }
    }
}

void SerialConnector::decodeSerialPacket(sdn_serial_packet_t *packet) {

//    std::cout << "decodeSerialPacket" << std::endl;

//    std::cout << std::setw(2) << std::setfill('0') << std::hex << (0xFF & packet->header.node_addr.u8[0]);
//    std::cout << ' ' << std::setw(2) << std::setfill('0') << std::hex << (0xFF & packet->header.node_addr.u8[1]);
//    std::cout << ' ' << std::setw(2) << std::setfill('0') << std::hex << (0xFF & packet->header.msg_type);
//    std::cout << " len:" << (unsigned int) packet->header.payload_len << std::endl;
//    std::cout << 1 << std::endl;
//    for (int i = 0; i < packet->header.payload_len; i++) {
//        std::cout << std::setw(2) << std::setfill('0') << std::hex << (0xFF & packet->payload[i]);
//        std::cout << ' ';
//    }
//    std::cout << "decodeSerialPacket end print" << std::endl;


    //qDebug() << "[MainWindow::decodePacket] New Packet";
    if(packet->header.msg_type == SDN_SERIAL_MSG_TYPE_PRINT) {

        QByteArray string((char*)packet->payload, packet->header.payload_len);
//        QString text = QString("ID %1: %2").arg((int)packet->header.node_addr.u8[1]).arg(QString(string).remove("\n"));
        QString text = QString("%1").arg(QString(string));
        emit printfString(text);

    } else if(packet->header.msg_type == SDN_SERIAL_MSG_TYPE_ID_REQUEST){

         emit nodeIdReceived((sdnaddr_t *)packet->header.node_addr.u8);

    } else if(packet->header.msg_type == SDN_SERIAL_MSG_TYPE_RADIO){

        emit defaultPacket(QByteArray((char*)packet, sizeof(sdn_serial_packet_t)));

    } else if(packet->header.msg_type == SDN_SERIAL_MSG_TYPE_RADIO_ACK){

        emit ackReceived(SERIAL_TX_ACK);

    } else if(packet->header.msg_type == SDN_SERIAL_MSG_TYPE_RADIO_NACK){

        emit nackReceived(SERIAL_TX_NACK);

    } else if(packet->header.msg_type == SDN_SERIAL_MSG_TYPE_TIMER){

        sdn_reliability_timer_event();

    } else if(packet->header.msg_type != SDN_SERIAL_MSG_TYPE_EMPTY){

        emit customPacket(QByteArray((char*)packet, sizeof(sdn_serial_packet_t)));
    }
}

void SerialConnector::close() {

    device->close();
}
