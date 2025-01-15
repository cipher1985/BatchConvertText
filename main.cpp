#include "batchconverttext.h"

#include <QApplication>
#include <QTranslator>

#include <qdarkstyle.h>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //加载文本
    QTranslator translator;
    translator.load(":/qt_zh_CN.qm");
    qApp->installTranslator(&translator);
    //设置样式
    SetQDarkStyleSheet();
    //启动窗口
    BatchConvertText w;
    w.show();
    return a.exec();
}
