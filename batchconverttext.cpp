#include "batchconverttext.h"
#include "ui_batchconverttext.h"

#include <QFile>
#include <QDir>
#include <QTextCodec>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QMimeData>
#include <thread>

BatchConvertText::BatchConvertText(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BatchConvertText)
{
    ui->setupUi(this);
    qRegisterMetaType<CodeMode>("CodeMode");
    qRegisterMetaType<EnterMode>("EnterMode");
    qRegisterMetaType<BatchConvertText::CodeMode>("BatchConvertText::CodeMode");
    qRegisterMetaType<BatchConvertText::EnterMode>("BatchConvertText::EnterMode");

    connect(this, &BatchConvertText::sigStartConvert,
            this, &BatchConvertText::startConvert, Qt::QueuedConnection);
    setAcceptDrops(true);
}

BatchConvertText::~BatchConvertText()
{
    delete ui;
}

void BatchConvertText::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        QList<QUrl> urls = mimeData->urls();
        if(urls.count() == 1) {
            QFileInfo info(urls[0].toLocalFile());
            if(info.isDir()) {
                event->acceptProposedAction();
                event->setDropAction(Qt::LinkAction);
                event->accept();
                return;
            }
        }
    }
    event->ignore();
}

void BatchConvertText::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        QList<QUrl> urls = mimeData->urls();
        if(urls.count() == 1) {
            QFileInfo info(urls[0].toLocalFile());
            if(info.isDir()) {
                ui->lineEdit_floder->setText(info.absoluteFilePath());
            }
        }
    }
}

void BatchConvertText::appendLog(QString log)
{
    ui->plainTextEdit_log->moveCursor(QTextCursor::End);
    ui->plainTextEdit_log->textCursor().insertText(log);
    ui->plainTextEdit_log->textCursor().insertText("\n");
}
void BatchConvertText::on_pushButton_select_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this,
        u8"选择要转换的目标文件夹", QString(), QFileDialog::ShowDirsOnly);
    if (folder.isEmpty())
    {
        return;
    }
    ui->lineEdit_floder->setText(folder);
}
void BatchConvertText::on_pushButton_start_clicked()
{
    QString folderName = ui->lineEdit_floder->text();
    if (folderName.isEmpty())
    {
        appendLog(u8"未设置转换文件夹");
        return;
    }
    folderName.replace('\\', '/');
    QDir folder(folderName);
    if (!folder.exists())
    {
        appendLog(u8"要转换的文件夹不存在");
    }
    //获得文件类型
    QString filter = ui->lineEdit_filter->text();
    if (filter.isEmpty())
    {
        filter = "*.*";
    }
    QStringList filterList = filter.split(';');
    //获得要转换的类型
    CodeMode cm = (CodeMode)ui->comboBox_code->currentIndex();
    EnterMode em = (EnterMode)ui->comboBox_enter->currentIndex();
    //获得文件数量
    m_curFileId = 0;
    m_allFiles = countFiles(folderName, filterList);
    //获得文件夹文件
    emit sigStartConvert(folderName, filterList, cm, em);
}
void BatchConvertText::startConvert(
    const QString& folderName, const QStringList& filterList,
    const CodeMode& codeMode, const EnterMode& enterMode)
{
    ui->pushButton_start->setEnabled(false);
    convertDir(folderName, filterList, codeMode, enterMode,
               ui->checkBox->isChecked());
    ui->pushButton_start->setEnabled(true);
    appendLog(u8"文件转换完成");
}
int BatchConvertText::countFiles(
    const QString& folderName, const QStringList& filterList)
{
    int allCount = 0;
    QDir folder(folderName);
    QStringList dirList = folder.entryList(QDir::Dirs);
    dirList.removeOne(".");
    dirList.removeOne("..");
    for (auto& i : dirList)
    {
        allCount += countFiles(folderName + "/" + i, filterList);
    }
    folder.setNameFilters(filterList);
    QStringList fileList = folder.entryList(QDir::Files);
    allCount += fileList.count();
    return allCount;
}
void BatchConvertText::convertDir(const QString& folderName, const QStringList& filterList,
    const CodeMode& codeMode, const EnterMode& enterMode, bool includeSubDir)
{
    appendLog(QString(u8"准备遍历文件夹[%1]").arg(folderName));
    QDir folder(folderName);
    if(includeSubDir) {
        QStringList dirList = folder.entryList(QDir::Dirs);
        dirList.removeOne(".");
        dirList.removeOne("..");
        for (auto& i : dirList) {
            convertDir(folderName + "/" + i, filterList, codeMode, enterMode);
        }
    }
    folder.setNameFilters(filterList);
    QStringList fileList = folder.entryList(QDir::Files);
    for (auto& i : fileList) {
        convertFile(folderName + "/" + i, codeMode, enterMode);
        ++m_curFileId;
        float fValue = (float)m_curFileId / m_allFiles;
        ui->progressBar->setValue((int)(fValue * 100));
    }
}
void BatchConvertText::convertFile(const QString& fileName,
    const CodeMode& codeMode, const EnterMode& enterMode)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        appendLog(QString(u8"无法打开转换文件[%1]").arg(fileName));
        return;
    }
    //获得文件编码
    QByteArray buff = file.readAll();
    QString codeFormat = getCorrectUnicode(buff);
    file.close();
    //读取文件内容
    if (!file.open(QFile::ReadOnly))
    {
        appendLog(QString(u8"无法打开转换文件[%1]").arg(fileName));
        return;
    }
    QTextStream out(&file);
    out.setCodec(codeFormat.toStdString().c_str());
    QString allText = out.readAll();
    file.close();
    //替换换行符
    allText.replace("\r\n", "\n");
    allText.replace("\r", "\n");
    switch (enterMode)
    {
    case EM_WinCRLF:
        allText.replace("\n", "\r\n");
        break;
    case EM_MacCR:
        allText.replace("\n", "\r");
        break;
    default:
        break;
    }
    if (!file.open(QFile::WriteOnly | QIODevice::Truncate))
    {
        appendLog(QString(u8"无法保存转换文件[%1]").arg(fileName));
        return;
    }
    QTextStream streamFileOut(&file);
    switch (codeMode)
    {
    case CM_Ansi:
        break;
    case CM_Unicode:
        streamFileOut.setGenerateByteOrderMark(true);
        streamFileOut.setCodec("UTF-16");
        break;
    case CM_UnicodeBE:
        streamFileOut.setGenerateByteOrderMark(true);
        streamFileOut.setCodec("UTF-16BE");
        break;
    case CM_Utf8:
        streamFileOut.setGenerateByteOrderMark(false);
        streamFileOut.setCodec("UTF-8");
        break;
    case CM_Utf8Bom:
        streamFileOut.setGenerateByteOrderMark(true);
        streamFileOut.setCodec("UTF-8");
        break;
    default:
        break;
    }
    streamFileOut << allText;
    file.close();
    appendLog(QString(u8"文件[%1]转换完成").arg(fileName));
}
QString BatchConvertText::getCorrectUnicode(const QByteArray& ba)
{
    QTextCodec::ConverterState state;
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    QString text = codec->toUnicode(ba.constData(), ba.size(), &state);
    if (state.invalidChars > 0)
    {
        text = QTextCodec::codecForName("GBK")->toUnicode(ba);
        return QString("GBK");
    }
    else
    {
        text = ba;
        return QString("UTF8");
    }
}
