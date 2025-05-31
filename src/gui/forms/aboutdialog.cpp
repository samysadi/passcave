#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "config.h"

#include <QLayout>
#include <QMessageBox>

AboutDialog::AboutDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog) {
	ui->setupUi(this);

	this->setWindowTitle(tr("About %1").arg(QString::fromStdString(passcave_APPLICATION_NAME)));
	ui->labelVer->setText(QString::fromStdString(std::string(passcave_APPLICATION_NAME) + " " + std::to_string(passcave_VERSION_MAJOR) + "." + std::to_string(passcave_VERSION_MINOR)));
	ui->labelDesc->setText(tr(
"<p>%5 is a password manager and encrypter which can be used to safely store sensitive information.</p>"
"<p>%5 has been developped by %1 <%2> using gcrypt library and Qt.</p>"
"<p>See <a href=\"http://%3\">%3</a> for more information.</p>"
"<p>© %4 %1.</p>"
).arg(passcave_AUTHOR).arg(passcave_AUTHOR_MAIL).arg(passcave_DOMAIN).arg(passcave_COPYRIGHT_YEAR).arg(QString::fromStdString(passcave_APPLICATION_NAME)));
	ui->labelDesc->setTextFormat(Qt::RichText);

	this->setFixedSize(this->size());
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	//this->layout()->setSizeConstraint(QLayout::SetFixedSize);

	connect(ui->aboutQtButton, SIGNAL(clicked(bool)), this, SLOT(onAboutQt()));
}

AboutDialog::~AboutDialog() {
	delete ui;
}

void AboutDialog::onAboutQt() {
	close();
	QMessageBox::aboutQt(static_cast<QWidget*>(this->parent()));
}
