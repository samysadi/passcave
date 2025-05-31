#include "longtextdialog.h"
#include "ui_longtextdialog.h"

#include "includes/datamodel.h"

#include <QMessageBox>

LongTextDialog::LongTextDialog(QWidget* parent, bool editable,
							   QString propertyName,
							   QString initialText) :
	QDialog(parent),
	ui(new Ui::LongTextDialog) {
	ui->setupUi(this);

	ui->plainTextEdit->setReadOnly(!editable);
	ui->applyButton->setVisible(editable);


	this->applied = false;
	this->initialText = initialText;

	QString p = QString::fromStdString(DataModel::formatHeader(propertyName.toStdString()));
	this->setWindowTitle(editable ? tr("Editing %1").arg(p) :
									tr("Displaying %1").arg(p));

	ui->plainTextEdit->setPlainText(initialText);
	ui->plainTextEdit->setPlaceholderText(editable ?  tr("Enter %1").arg(p) :
													  tr("%1 is empty").arg(p));
}

LongTextDialog::~LongTextDialog() {
	delete ui;
}

QString LongTextDialog::getContents() {
	return ui->plainTextEdit->toPlainText();
}

bool LongTextDialog::isApplied() {
	return applied;
}

void LongTextDialog::closeEvent(QCloseEvent* e) {
	if (!this->applied && this->initialText.compare(ui->plainTextEdit->toPlainText()) != 0) {
		QString m1 = tr("There are unnapplied modifications.");
		QString m2 = tr("Are you sure you want to close this window and lose them?");

		QMessageBox box(QMessageBox::Question, tr("Confirmation"), m1, QMessageBox::Discard | QMessageBox::Cancel, this);
		box.setDefaultButton(QMessageBox::Cancel);
		box.setInformativeText(m2);
		int reply = box.exec();
		if (reply != QMessageBox::Discard) {
			e->ignore();
			return;
		}
	}
}

void LongTextDialog::on_applyButton_clicked() {
	this->applied = true;
	this->close();
}

void LongTextDialog::on_cancelButton_clicked() {
	this->applied = false;
	this->close();
}
