#include "clickableqlabel.h"

#include <QComboBox>
#include <QAbstractSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>

ClickableQLabel::ClickableQLabel(QWidget* parent)
    : QLabel(parent) {

}

ClickableQLabel::ClickableQLabel(QString const& text, QWidget* parent)
    : QLabel(parent) {
    setText(text);
}

ClickableQLabel::~ClickableQLabel() {
}

template<typename T>
bool trySelectAll(QWidget* o) {
	T* c = dynamic_cast<T*>(o);
	if (nullptr != c) {
		c->selectAll();
		return true;
    }
	return false;
}

void ClickableQLabel::mousePressEvent(QMouseEvent* event) {
    QLabel::mousePressEvent(event);
    if (this->buddy() != NULL) {
        this->buddy()->setFocus();

		trySelectAll<QAbstractSpinBox>(this->buddy()) ||
				trySelectAll<QLineEdit>(this->buddy())	 ||
				trySelectAll<QPlainTextEdit>(this->buddy())		 ||
				trySelectAll<QTextEdit>(this->buddy());
    }
    emit clicked();
}
