#ifndef BATCHCONVERTTEXT_H
#define BATCHCONVERTTEXT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class BatchConvertText; }
QT_END_NAMESPACE

class BatchConvertText : public QWidget
{
    Q_OBJECT

public:
    BatchConvertText(QWidget *parent = nullptr);
    ~BatchConvertText();

    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
private:
    Ui::BatchConvertText *ui;
    int m_curFileId = 0;
    int m_allFiles = 0;
    void appendLog(QString log);
    //编码类型
    enum CodeMode
    {
        CM_Ansi,			//ASCII和GBK码
        CM_Unicode,		//小端字节序(将高位的字节放在高地址表示)
        CM_UnicodeBE,		//大端字节序(将高位的字节放在低地址表示)
        CM_Utf8,
        CM_Utf8Bom
    };
    //回车符类型
    enum EnterMode
    {
        EM_WinCRLF,		//\r\n
        EM_UnixLF,		//\n
        EM_MacCR		//\r
    };
    //转换文件
    void convertFile(const QString& fileName,
                     const CodeMode& codeMode = CodeMode::CM_Utf8Bom,
                     const EnterMode& enterMode = EnterMode::EM_WinCRLF);
    //获取文本编码
    QString getCorrectUnicode(const QByteArray& ba);
    //遍历文件夹
    void convertDir(const QString& folderName,
                    const QStringList& filterList,
                    const CodeMode& codeMode = CodeMode::CM_Utf8Bom,
                    const EnterMode& enterMode = EnterMode::EM_WinCRLF,
                    bool includeSubDir = true);
    //遍历文件个数
    int countFiles(const QString& folderName,
                   const QStringList& filterList);
Q_SIGNALS:
    void sigStartConvert(const QString& folderName,
                         const QStringList& filterList,
                         const CodeMode& codeMode = CodeMode::CM_Utf8Bom,
                         const EnterMode& enterMode = EnterMode::EM_WinCRLF);
private Q_SLOTS:
    //开始转换
    void startConvert(const QString& folderName,
                      const QStringList& filterList,
                      const CodeMode& codeMode = CodeMode::CM_Utf8Bom,
                      const EnterMode& enterMode = EnterMode::EM_WinCRLF);

    void on_pushButton_select_clicked();
    void on_pushButton_start_clicked();
};
#endif // BATCHCONVERTTEXT_H
