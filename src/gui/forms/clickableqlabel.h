#pragma once

#include <QObject>
#include <QWidget>
#include <QLabel>

class ClickableQLabel : public QLabel
{
	Q_OBJECT
public:
    explicit ClickableQLabel(QWidget* parent = 0);
    explicit ClickableQLabel(QString const& text, QWidget* parent = 0);
    ~ClickableQLabel();

protected:
    void mousePressEvent(QMouseEvent* event);

signals:
    void clicked();

public slots:
};
