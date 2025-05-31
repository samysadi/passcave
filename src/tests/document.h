#pragma once
#include <QtTest/QtTest>

namespace passcave {
namespace test{
class Document;
}
}

class passcave::test::Document: public QObject
{
    Q_OBJECT
private slots:
    void toUpper();
};
