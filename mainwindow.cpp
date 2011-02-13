#include <QtGui>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hidapi.h"
#include "fcd.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QSettings settings;

    ui->setupUi(this);

    ui->lineEditFreq->setText(settings.value("Frequency","97,300.000").toString());
    ui->lineEditStep->setText(settings.value("Step","25,000").toString());
    ui->spinBoxCorr->setValue(settings.value("Correction","-120").toInt());
    EnableControls();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(EnableControls()));
    timer->start(1000);
}

MainWindow::~MainWindow()
{
    QSettings settings;

    timer->stop();
    delete timer;

    settings.setValue("Frequency",ui->lineEditFreq->text());
    settings.setValue("Step",ui->lineEditStep->text());
    settings.setValue("Correction",ui->spinBoxCorr->value());

    delete ui;
}

double MainWindow::StrToDouble(QString s)
{
    int i;
    QString s2="";

    for (i=0;i<s.length();i++)
    {
        QChar c=s[i];
        if (c>='0' and c<='9')
        {
            s2+=c;
        }
    }
    s2=s2.left(10);

    return s2.toDouble();
}

void MainWindow::EnableControls()
{
    FCDMODEENUM fme;

    ui->plainTextEdit->clear();

/*
    {
        struct hid_device_info *devs,*cur_dev;

        devs=hid_enumerate(0x04D8,0xFB56);
        cur_dev=devs;
        while (cur_dev)
        {
            ui->plainTextEdit->appendPlainText(QString::number(cur_dev->vendor_id,16));
            ui->plainTextEdit->appendPlainText(QString::number(cur_dev->product_id,16));
            ui->plainTextEdit->appendPlainText(QString::fromAscii(cur_dev->path));
            if (cur_dev->serial_number==NULL)
            {
                ui->plainTextEdit->appendPlainText("NULL");
            }
            else
            {
                ui->plainTextEdit->appendPlainText(QString::fromWCharArray(cur_dev->serial_number));
            }
            ui->plainTextEdit->appendPlainText(QString::fromWCharArray(cur_dev->manufacturer_string));
            ui->plainTextEdit->appendPlainText(QString::fromWCharArray(cur_dev->product_string));
            cur_dev=cur_dev->next;
        }
        hid_free_enumeration(devs);
    }
*/


    fme=FCDGetMode();
    switch (fme)
    {
        case FME_APP:
            {
                QPalette p=ui->plainTextEdit->palette();
                p.setColor(QPalette::Base, QColor(0,255,0));//green color
                ui->plainTextEdit->setPalette(p);
            }
            ui->plainTextEdit->appendPlainText("FCD active");
            break;
        case FME_BL:
            {
                QPalette p=ui->plainTextEdit->palette();
                p.setColor(QPalette::Base, QColor(255,191,0));//amber color
                ui->plainTextEdit->setPalette(p);
            }
            ui->plainTextEdit->appendPlainText("FCD bootloader");
            break;
        case FME_NONE:
            {
                QPalette p=ui->plainTextEdit->palette();
                p.setColor(QPalette::Base, QColor(255,0,0));//red color
                ui->plainTextEdit->setPalette(p);
            }
            ui->plainTextEdit->appendPlainText("No FCD detected");
            break;
    }
    ui->pushButtonUpdateFirmware->setEnabled(fme==FME_BL);
    ui->pushButtonVerifyFirmware->setEnabled(fme==FME_BL);
    ui->pushButtonBLReset->setEnabled(fme==FME_BL);
    ui->pushButtonAppReset->setEnabled(fme==FME_APP);
    ui->lineEditFreq->setEnabled(fme==FME_APP);
    ui->lineEditStep->setEnabled(fme==FME_APP);
    ui->pushButtonUp->setEnabled(fme==FME_APP);
    ui->pushButtonDown->setEnabled(fme==FME_APP);
    ui->spinBoxCorr->setEnabled(fme==FME_APP);
}

void MainWindow::on_pushButtonAppReset_clicked()
{
    FCDAppReset();
}

void MainWindow::on_pushButtonBLReset_clicked()
{
    FCDBLReset();
}

void MainWindow::on_pushButtonUpdateFirmware_clicked()
{
    QString fileName = QFileDialog::getOpenFileName
    (
        this,
        tr("Open FCD firmware"),
        QDir::currentPath(),
        tr("FCD firmware files (*.bin)")
    );
    if( !fileName.isNull() )
    {
        QFile qf(fileName);
        qint64 qn64size=qf.size();
        char *buf=new char[qn64size];

        qDebug( fileName.toAscii() );

        if (buf==NULL)
        {
            QMessageBox::critical
            (
                this,
                tr("FCD"),
                tr("Unable to allocate memory for firmware image")
            );
            return;
        }

        if (!qf.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical
            (
                this,
                tr("FCD"),
                tr("Unable to open file")
            );
            delete buf;
            return;
        }
        else
        {
            if (qf.read(buf,qn64size)!=qn64size)
            {
                QMessageBox::critical
                (
                    this,
                    tr("FCD"),
                    tr("Unable to read file")
                );
                delete buf;
                qf.close();
                return;

            }
        }
        qf.close();

        if (FCDBLErase()!=FME_BL)
        {
            QMessageBox::critical
            (
                this,
                tr("FCD"),
                tr("Flash erase failed")
            );
            delete buf;
            return;

        }

        if (FCDBLWriteFirmware(buf,(int64_t)qn64size)!=FME_BL)
        {
            QMessageBox::critical
            (
                this,
                tr("FCD"),
                tr("Write firmware failed")
            );
            delete buf;
            return;
        }

        delete buf;
        QMessageBox::information
        (
            this,
            tr("FCD"),
            tr("Firmware successfully written!")
        );
    }
}

void MainWindow::on_pushButtonVerifyFirmware_clicked()
{
    QString fileName = QFileDialog::getOpenFileName
    (
        this,
        tr("Open FCD firmware"),
        QDir::currentPath(),
        tr("FCD firmware files (*.bin)")
    );
    if( !fileName.isNull() )
    {
        QFile qf(fileName);
        qint64 qn64size=qf.size();
        char *buf=new char[qn64size];

        qDebug( fileName.toAscii() );

        if (buf==NULL)
        {
            QMessageBox::critical
            (
                this,
                tr("FCD"),
                tr("Unable to allocate memory for firmware image")
            );
            return;
        }

        if (!qf.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical
            (
                this,
                tr("FCD"),
                tr("Unable to open file")
            );
            delete buf;
            return;
        }
        else
        {
            if (qf.read(buf,qn64size)!=qn64size)
            {
                QMessageBox::critical
                (
                    this,
                    tr("FCD"),
                    tr("Unable to read file")
                );
                delete buf;
                qf.close();
                return;

            }
        }
        qf.close();

        if (FCDBLVerifyFirmware(buf,(int64_t)qn64size)!=FME_BL)
        {
            QMessageBox::critical
            (
                this,
                tr("FCD"),
                tr("Verify firmware failed")
            );
            delete buf;
            return;
        }

        delete buf;
        QMessageBox::information
        (
            this,
            tr("FCD"),
            tr("Firmware successfully verified!")
        );
    }

}

void MainWindow::on_lineEditFreq_textChanged(QString s)
{
    double d=StrToDouble(s);
    int nCursor=ui->lineEditFreq->cursorPosition();
    QString s2=QLocale(QLocale()).toString(d,'f',0);

    nCursor-=s.mid(0,nCursor).count(QLocale().groupSeparator());
    nCursor+=s2.mid(0,nCursor).count(QLocale().groupSeparator());

    ui->lineEditFreq->setText(s2);
    ui->lineEditFreq->setCursorPosition(nCursor);
    if (d<50000000.0 || d>2100000000.0)
    {
        QPalette p=ui->lineEditFreq->palette();
        p.setColor(QPalette::Base, QColor(255,0,0));//red color
        ui->lineEditFreq->setPalette(p);
    }
    else
    {
        QPalette p=ui->lineEditFreq->palette();
        p.setColor(QPalette::Base, QColor(0,255,0));//green color
        ui->lineEditFreq->setPalette(p);
    }

    d*=1.0+ui->spinBoxCorr->value()/1000000.0;

    FCDAppSetFreqkHz((int)(d/1000.0));
}

void MainWindow::on_lineEditStep_textChanged(QString s)
{
    double d=StrToDouble(s);
    int nCursor=ui->lineEditStep->cursorPosition();
    QString s2=QLocale(QLocale()).toString(d,'f',0);

    nCursor-=s.mid(0,nCursor).count(QLocale().groupSeparator());
    nCursor+=s2.mid(0,nCursor).count(QLocale().groupSeparator());

    ui->lineEditStep->setText(s2);
    ui->lineEditStep->setCursorPosition(nCursor);
    if (d<1.0 || d>1000000000.0)
    {
        QPalette p=ui->lineEditStep->palette();
        p.setColor(QPalette::Base, QColor(255,0,0));//red color
        ui->lineEditStep->setPalette(p);
    }
    else
    {
        QPalette p=ui->lineEditStep->palette();
        p.setColor(QPalette::Base, QColor(0,255,0));//green color
        ui->lineEditStep->setPalette(p);
    }
}

void MainWindow::on_pushButtonUp_clicked()
{
    double dStep=StrToDouble(ui->lineEditStep->text());
    double dFreq=StrToDouble(ui->lineEditFreq->text());

    dFreq+=dStep;

    if (dFreq<0.0)
    {
        dFreq=0.0;
    }
    if (dFreq>2000000000.0)
    {
        dFreq=2000000000.0;
    }
    ui->lineEditFreq->setText(QLocale(QLocale()).toString(dFreq,'f',0));
}

void MainWindow::on_pushButtonDown_clicked()
{
    double dStep=StrToDouble(ui->lineEditStep->text());
    double dFreq=StrToDouble(ui->lineEditFreq->text());

    dFreq-=dStep;

    if (dFreq<0.0)
    {
        dFreq=0.0;
    }
    if (dFreq>2000000000.0)
    {
        dFreq=2000000000.0;
    }
    ui->lineEditFreq->setText(QLocale(QLocale()).toString(dFreq,'f',0));
}

void MainWindow::on_spinBoxCorr_valueChanged(int n)
{
    double d=StrToDouble(ui->lineEditFreq->text());

    d*=1.0+n/1000000.0;

    FCDAppSetFreqkHz((int)(d/1000.0));
}
