#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QString>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTimer>
#include <QPainter>
#include <QFile>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void manual_serialPortReadyRead();
    void on_openBt_clicked();
    void on_sendBt_clicked();
    void on_clearBt_clicked();
    void on_btnClearSend_clicked();
    void on_chkTimSend_stateChanged(int arg1);
    void on_btnSerialCheck_clicked();
    void on_CTP_modle_clicked();
    void on_UpData_modle_clicked();
    void handleAckReceived();
    void handleSendTimeout();
//    void sendNextChunk();

private:
    Ui::MainWindow *ui;
    QSerialPort *serialPort;

    // 计数和状态显示
    long sendNum, recvNum;
    QLabel *lblSendNum;
    QLabel *lblRecvNum;
    QLabel *lblPortState;
    void setNumOnLabel(QLabel *lbl, QString strS, long num);

    // 定时器
    QTimer *timSend;
    QTimer *ackTimer;

    // 固件升级相关
    bool updating = false;
    bool waitingForAck = false;
    QFile *file = nullptr;
    qint64 currentSize = 0;
    long seq_counter = 0;

    // CRC校验
    uint16_t CheckData(const QByteArray array);
    uint16_t crc16(const uint8_t* pData, int bytes, uint16_t* crcInitValue);
    uint16_t SendData(QByteArray array);
    bool prepareUpdate();
    void startFirmwareUpdate();
    void sendNextFileChunk();
    void finishUpdate(bool success);


signals:
    // 定义信号
    void ackReceived();
};

#endif // MAINWINDOW_H
