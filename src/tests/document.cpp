#include "document.h"
#include <QXmlStreamReader>

using namespace passcave::test;

void Document::toUpper()
{
    QString str = "Hello";
    QVERIFY(str.toUpper() == "HELLO");
}

QTEST_MAIN( Document )

#include "moc_document.cpp"
