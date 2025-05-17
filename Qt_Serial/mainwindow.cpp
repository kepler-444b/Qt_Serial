#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDateTime>

// 构造函数
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化UI
    ui->progressBar->setVisible(false);
    ui->serailCb->clear();

    // 扫描所有可用的串口
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->serailCb->addItem(info.portName());
    }

    // 初始化串口
    serialPort = new QSerialPort(this);
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::manual_serialPortReadyRead);

    // 初始化计数
    sendNum = 0;
    recvNum = 0;

    // 状态栏
    QStatusBar *sBar = statusBar();
    lblSendNum = new QLabel(this);
    lblRecvNum = new QLabel(this);
    lblPortState = new QLabel(this);

    lblPortState->setText("Disconnected");
    lblPortState->setStyleSheet("color:red");
    lblSendNum->setMinimumSize(100, 20);
    lblRecvNum->setMinimumSize(100, 20);
    lblPortState->setMinimumSize(550, 20);

    setNumOnLabel(lblSendNum, "S: ", sendNum);
    setNumOnLabel(lblRecvNum, "R: ", recvNum);

    sBar->addPermanentWidget(lblPortState);
    sBar->addPermanentWidget(lblSendNum);
    sBar->addPermanentWidget(lblRecvNum);

    // 定时发送
    timSend = new QTimer(this);
    timSend->setInterval(1000);
    connect(timSend, &QTimer::timeout, this, [=]()
    {
        on_sendBt_clicked();
    });

    // ACK超时定时器
    ackTimer = new QTimer(this);
    ackTimer->setInterval(3000);    // 3秒超时
    ackTimer->setSingleShot(true);  // 单次触发模式
    connect(ackTimer, &QTimer::timeout, this, &MainWindow::handleSendTimeout);  // 信号槽连接

    // 连接ACK信号
    connect(this, &MainWindow::ackReceived, this, &MainWindow::handleAckReceived);
}

MainWindow::~MainWindow()
{
    if(serialPort->isOpen())
    {
        serialPort->close();
    }
    delete ui;
}


// 打开串口
void MainWindow::on_openBt_clicked()
{
    if(ui->openBt->text() == "打开串口")
    {
        // 配置串口参数
        serialPort->setPortName(ui->serailCb->currentText());

        // 波特率
        if(ui->baundrateCb->currentText() == "1200") serialPort->setBaudRate(QSerialPort::Baud1200);
        else if(ui->baundrateCb->currentText() == "2400") serialPort->setBaudRate(QSerialPort::Baud2400);
        else if(ui->baundrateCb->currentText() == "4800") serialPort->setBaudRate(QSerialPort::Baud4800);
        else if(ui->baundrateCb->currentText() == "9600") serialPort->setBaudRate(QSerialPort::Baud9600);
        else if(ui->baundrateCb->currentText() == "19200") serialPort->setBaudRate(QSerialPort::Baud19200);
        else if(ui->baundrateCb->currentText() == "38400") serialPort->setBaudRate(QSerialPort::Baud38400);
        else if(ui->baundrateCb->currentText() == "57600") serialPort->setBaudRate(QSerialPort::Baud57600);
        else if(ui->baundrateCb->currentText() == "115200") serialPort->setBaudRate(QSerialPort::Baud115200);
        else if(ui->baundrateCb->currentText() == "256000") serialPort->setBaudRate(QSerialPort::Baud256000);

        // 数据位
        if(ui->databitCb->currentText() == "5") serialPort->setDataBits(QSerialPort::Data5);
        else if(ui->databitCb->currentText() == "6") serialPort->setDataBits(QSerialPort::Data6);
        else if(ui->databitCb->currentText() == "7") serialPort->setDataBits(QSerialPort::Data7);
        else if(ui->databitCb->currentText() == "8") serialPort->setDataBits(QSerialPort::Data8);

        // 停止位
        if(ui->stopbitCb->currentText() == "1") serialPort->setStopBits(QSerialPort::OneStop);
        else if(ui->stopbitCb->currentText() == "1.5") serialPort->setStopBits(QSerialPort::OneAndHalfStop);
        else if(ui->stopbitCb->currentText() == "2") serialPort->setStopBits(QSerialPort::TwoStop);

        // 校验位
        if(ui->checkbitCb->currentText() == "none") serialPort->setParity(QSerialPort::NoParity);
        else if(ui->checkbitCb->currentText() == "奇校验") serialPort->setParity(QSerialPort::OddParity);
        else if(ui->checkbitCb->currentText() == "偶校验") serialPort->setParity(QSerialPort::EvenParity);

        // 打开串口
        if(serialPort->open(QIODevice::ReadWrite))
        {
            ui->openBt->setText("关闭串口");
            ui->serailCb->setEnabled(false);
            lblPortState->setText(serialPort->portName() + " OPENED, " +
                                QString::number(serialPort->baudRate()) + ", 8, NONE, 1");
            lblPortState->setStyleSheet("color:green");
        } else
        {
            QMessageBox::critical(this, "错误", "串口打开失败！");
        }
    }
    else
    {
        serialPort->close();
        ui->openBt->setText("打开串口");
        ui->serailCb->setEnabled(true);
        lblPortState->setText("Disconnected");
        lblPortState->setStyleSheet("color:red");
    }
}

// 接收数据
void MainWindow::manual_serialPortReadyRead()
{
    while(serialPort->bytesAvailable()) {
        QByteArray recBuf = serialPort->readAll();
        QString str_rev;

        // 更新接收计数
        recvNum += recBuf.size();
        setNumOnLabel(lblRecvNum, "R: ", recvNum);

        // 查找升级固件时的 ACK
        if(recBuf.contains("cmd=readFile"))
        {
            emit ackReceived(); // 发送信号
        }

        if(ui->chk_rev_hex->checkState() == false)
        {
            // 普通接收
            if(ui->chk_rev_time->checkState() == Qt::Checked)   // 显示时间戳
            {
                QDateTime nowtime = QDateTime::currentDateTime();
                str_rev = "[" + nowtime.toString("yyyy-MM-dd hh:mm:ss") + "] " + QString(recBuf);
            }
            else
            {
                str_rev = QString(recBuf);
            }
            if(ui->chk_rev_line->checkState() == Qt::Checked)   // 自动换行
            {
                str_rev += "\r\n";
            }
        }
        else
        {
            // Hex 接收
            QString hexStr = recBuf.toHex().toUpper();
            QString formattedStr;
            for(int i = 0; i < hexStr.length(); i += 2)
            {
                formattedStr += hexStr.mid(i, 2) + " ";
            }
            if(ui->chk_rev_time->checkState() == Qt::Checked)
            {
                QDateTime nowtime = QDateTime::currentDateTime();
                str_rev = "[" + nowtime.toString("yyyy-MM-dd hh:mm:ss") + "] " + formattedStr;
            } else
            {
                str_rev = formattedStr;
            }
            if(ui->chk_rev_line->checkState() == Qt::Checked)
            {
                str_rev += "\r\n";
            }
        }

        ui->recvEdit->insertPlainText(str_rev);
        ui->recvEdit->moveCursor(QTextCursor::End);
    }
}

// 发送数据
void MainWindow::on_sendBt_clicked()
{
    if(!serialPort->isOpen())
    {
        QMessageBox::warning(this, "警告", "请先打开串口！");
        return;
    }

    // 升级固件
    if(ui->UpData_modle->checkState() == Qt::Checked)
    {
        if(!updating)
        {
            startFirmwareUpdate();
        }
        return;
    }

    QByteArray data;
    // Hex 发送
    if(ui->chk_send_hex->checkState() == Qt::Checked)
    {
        data = QByteArray::fromHex(ui->sendEdit->toPlainText().toUtf8());
    }
    // CTP 发送
    else if(ui->CTP_modle->checkState() == Qt::Checked)
    {
        data = ui->sendEdit->toPlainText().toLocal8Bit();
        data.replace("\n", "\r\n");
        seq_counter++;
        data.append("\r\nseq=").append(QString::number(seq_counter).toUtf8()).append("\r\n");
        uint16_t crc = CheckData(data);
        data.append("crc=").append(QString::asprintf("%04X", crc).toUtf8()).append("\r\n\r\n");
    }
    // 普通发送
    else
    {
        data = ui->sendEdit->toPlainText().toLocal8Bit();
        if(ui->chk_send_line->checkState() == Qt::Checked)
        {
            data.append("\r\n");
        }
    }

    SendData(data);
}

// 开始升级固件
void MainWindow::startFirmwareUpdate()
{
    updating = true;
    ui->progressBar->setVisible(true);

    if(!prepareUpdate())
    {
        finishUpdate(false);
        return;
    }

    file = new QFile("../SerialPort/updata/F4A0.binX");
    if(!file->open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开固件文件！");
        finishUpdate(false);
        return;
    }

    ui->progressBar->setRange(0, file->size() * 2);
    currentSize = 0;

    // 从接收数据中等待第一个ACK
    waitingForAck = true;
    ackTimer->start();
}

// 准备升级固件，先发送固件信息
bool MainWindow::prepareUpdate()
{
    QFile file("../SerialPort/updata/F4A0.binX");
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    // 读出文件的总大小
    qint64 totalBytes = file.size();
    qDebug() << "文件总大小:" << totalBytes << "bytes";

    // 检查文件是否足够大以包含最后512字节
    if (totalBytes < 512)
    {
        qDebug() << "文件太小，无法检查最后512字节";
        file.close();
        return false;
    }

    // 查找魔术字符串 "jjyMagic" (8字节)
    const QByteArray magicString = "jjyMagic";
    const int searchSize = 512;
    const int magicSize = magicString.size();

    // 移动到文件末尾前512字节处
    if (!file.seek(totalBytes - searchSize))
    {
        qDebug() << "无法定位到文件末尾前512字节";
        file.close();
        return false;
    }

    // 读取最后512字节
    QByteArray last512Bytes = file.read(searchSize);
    bool magicFound = false;
    int magicPos = -1;

    // 在最后512字节中查找魔术字符串
    for (int i = 0; i <= searchSize - magicSize; ++i)
    {
        if (memcmp(last512Bytes.constData() + i, magicString.constData(), magicSize) == 0)
        {
            magicFound = true;
            magicPos = i;
            qDebug() << "找到魔术字符串 'jjyMagic' 在位置" << (totalBytes - searchSize + i);
            break;
        }
    }

    if (!magicFound)
    {
        qDebug() << "未找到魔术字符串 'jjyMagic'";
        file.close();
        return false;
    }

    // 在魔术字符串后查找 "cpu=" 和 "model="
    const QByteArray cpuPrefix = "cpu=";
    const QByteArray modelPrefix = "model=";
    const int prefixSize = 5; // "cpu=" 和 "model=" 都是5个字符
    QByteArray cpuValue;
    QByteArray modelValue;

    // 从魔术字符串结束位置开始搜索
    for (int i = magicPos + magicSize; i <= searchSize - prefixSize - 1; ++i)
    {
        // 查找cpu
        if (cpuValue.isEmpty() && memcmp(last512Bytes.constData() + i, cpuPrefix.constData(), cpuPrefix.size()) == 0)
        {
            // 找到 "cpu=" 前缀，提取后面的字符串直到逗号
            int start = i + cpuPrefix.size();
            int end = start;
            while (end < searchSize && last512Bytes[end] != ',')
            {
                end++;
            }

            if (end > start && end < searchSize) {
                cpuValue = last512Bytes.mid(start, end - start);
                qDebug() << "找到CPU字符串:" << cpuValue;
            } else {
                qDebug() << "CPU字符串格式无效";
            }
        }

        // 查找model
        if (modelValue.isEmpty() && memcmp(last512Bytes.constData() + i, modelPrefix.constData(), modelPrefix.size()) == 0)
        {
            // 找到 "model=" 前缀，提取后面的字符串直到逗号
            int start = i + modelPrefix.size();
            int end = start;
            while (end < searchSize && last512Bytes[end] != ',')
            {
                end++;
            }

            if (end > start && end < searchSize)
            {
                modelValue = last512Bytes.mid(start, end - start);
                qDebug() << "找到Model字符串:" << modelValue;
            }
            else
            {
                qDebug() << "Model字符串格式无效";
            }
        }

        // 如果两个值都找到了，可以提前退出循环
        if (!cpuValue.isEmpty() && !modelValue.isEmpty())
        {
            break;
        }
    }

    if (cpuValue.isEmpty())
    {
        qDebug() << "未找到有效的CPU字符串";
        file.close();
        return false;
    }

    // 重置文件指针到开头进行CRC计算
    file.seek(0);

    uint16_t crc16Value = 0xFFFF;
    char buffer[4096];
    qint64 bytesRead;

    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0)
    {
        crc16((uint8_t*)buffer, bytesRead, &crc16Value);
    }
    file.close();

    seq_counter++;
    QByteArray data = "cmd=openFileAck\r\nlength=";
    data.append(QString::number(totalBytes)).append("\r\n");
    data.append("cpu=").append(cpuValue).append("\r\n");  // 追加CPU字符串到data
    if (!modelValue.isEmpty())
    {
        data.append("model=").append(modelValue).append("\r\n");  // 追加Model字符串到data
    }
    data.append("crc16=").append(QString::asprintf("%04X", crc16Value)).append("\r\n");
    data.append("seq=").append(QString::number(seq_counter)).append("\r\n");


    uint16_t crc = CheckData(data);  // 计算CRC（确保包含cpu和model字段）
    data.append("crc=").append(QString::asprintf("%04X", crc)).append("\r\n\r\n");

    qDebug() << "data" << data;
    SendData(data);
    return true;
}

void MainWindow::sendNextFileChunk()
{
    if(!updating || !file) return;

    const int chunkSize = 1024;
    QByteArray chunk = file->read(chunkSize);
    QString hexString = chunk.toHex();
    chunk = hexString.toUtf8();

    seq_counter++;
    if(chunk.isEmpty())
    {
        // 发送最后一个包
        QByteArray end_data = "cmd=readFileAck\r\ndata=";
        end_data.append("\r\nseq=").append(QString::number(seq_counter)).append("\r\n");
        uint16_t crc = CheckData(end_data);
        end_data.append("crc=").append(QString::asprintf("%04X", crc)).append("\r\n\r\n");
        SendData(end_data);

        finishUpdate(true);

        return;
    }


    QByteArray data = "cmd=readFileAck\r\ndata=";
    data.append(chunk).append("\r\nseq=").append(QString::number(seq_counter)).append("\r\n");

    uint16_t crc = CheckData(data);

    data.append("crc=").append(QString::asprintf("%04X", crc)).append("\r\n\r\n");

    SendData(data);

    currentSize += chunk.size();
    ui->progressBar->setValue(currentSize);

    // 等待ACK
    waitingForAck = true;
    ackTimer->start();
}

// 当接收到 ACK 信号时被触发
void MainWindow::handleAckReceived()
{
    if(waitingForAck)
    {
        ackTimer->stop();
        waitingForAck = false;
        sendNextFileChunk();
    }
}

void MainWindow::handleSendTimeout()
{
    if(waitingForAck)
    {
        QMessageBox::warning(this, "超时", "等待ACK超时！");
        finishUpdate(false);
    }
}

void MainWindow::finishUpdate(bool success)
{
    qDebug() << "finishUpdate";
    updating = false;
    waitingForAck = false;
    ackTimer->stop();

    if(file) {
        file->close();
        delete file;
        file = nullptr;
    }

    ui->progressBar->setVisible(false);

    if(success) {
        QMessageBox::information(this, "完成", "固件升级成功！");
    }
}

uint16_t MainWindow::SendData(QByteArray array)
{
    int sent = serialPort->write(array);
    if(sent > 0) {
        sendNum += sent;
        setNumOnLabel(lblSendNum, "S: ", sendNum);
    }
    return sent;
}

uint16_t MainWindow::CheckData(const QByteArray array)
{
    uint16_t crc = 0xFFFF;
    QList<QByteArray> lines = array.split('\n');

    for(const QByteArray &line : lines) {
        QByteArray trimmed = line.trimmed();
        int equalPos = trimmed.indexOf('=');

        if(equalPos > 0) {
            QByteArray key = trimmed.left(equalPos).trimmed();
            QByteArray value = trimmed.mid(equalPos + 1).trimmed();

            if(!key.isEmpty()) {
                crc16((uint8_t*)key.data(), key.size(), &crc);
            }
            if(!value.isEmpty()) {
                crc16((uint8_t*)value.data(), value.size(), &crc);
            }
        }
    }

    return crc;
}

uint16_t MainWindow::crc16(const uint8_t* pData, int bytes, uint16_t* crcInitValue)
{
    uint16_t crc = *crcInitValue;

    for(int i = 0; i < bytes; i++) {
        crc ^= pData[i];
        for(int j = 0; j < 8; j++) {
            if(crc & 0x01) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    *crcInitValue = crc;
    return crc;
}

// 清空接收
void MainWindow::on_clearBt_clicked()
{
    ui->recvEdit->clear();
    sendNum = 0;
    recvNum = 0;
    setNumOnLabel(lblSendNum, "S: ", sendNum);
    setNumOnLabel(lblRecvNum, "R: ", recvNum);
}

// 清空发送
void MainWindow::on_btnClearSend_clicked()
{
    ui->sendEdit->clear();
    sendNum = 0;
    setNumOnLabel(lblSendNum, "S: ", sendNum);
}

// 定时发送功能
void MainWindow::on_chkTimSend_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked) {
        if(ui->txtSendMs->text().toInt() >= 10) {
            timSend->start(ui->txtSendMs->text().toInt());
            ui->txtSendMs->setEnabled(false);
        } else {
            ui->chkTimSend->setCheckState(Qt::Unchecked);
            QMessageBox::warning(this, "错误", "定时发送间隔必须≥10ms");
        }
    } else {
        timSend->stop();
        ui->txtSendMs->setEnabled(true);
    }
}

// 扫描可利用的串口
void MainWindow::on_btnSerialCheck_clicked()
{
    ui->serailCb->clear();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->serailCb->addItem(info.portName());
    }
}

// CTP格式发送
void MainWindow::on_CTP_modle_clicked()
{
    if(ui->CTP_modle->checkState() == Qt::Checked) {
        ui->UpData_modle->setChecked(false);
    }
}

// 固件升级
void MainWindow::on_UpData_modle_clicked()
{
    if(ui->UpData_modle->checkState() == Qt::Checked) {
        ui->CTP_modle->setChecked(false);
    }
}

// 显示发送的字节数
void MainWindow::setNumOnLabel(QLabel *lbl, QString strS, long num)
{
    lbl->setText(strS + QString::number(num));
}
