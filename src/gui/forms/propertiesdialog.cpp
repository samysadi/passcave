#include "propertiesdialog.h"
#include "ui_propertiesdialog.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

using namespace passcave;

PropertiesDialog::PropertiesDialog(QWidget* parent,
								   std::string const& filename,
								   passcave::gcrypt_FileInfo const& info,
								   int numNodes,
								   int numPropertyDefinitions) :
	QDialog(parent),
	ui(new Ui::PropertiesDialog) {
	ui->setupUi(this);

	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

	ui->lineEdit_filename->setText(QString::fromStdString(filename));
	ui->lineEdit_cipher->setText(QString::fromStdString(toString(info.gcry_cipher_algo)));
	ui->lineEdit_cipher_mode->setText(QString::fromStdString(toString(info.gcry_cipher_mode)));
	ui->lineEdit_md->setText(QString::fromStdString(toString(info.gcry_md_algo)));
	ui->lineEdit_md_iterations->setText((info.gcry_md_iterations_auto ? tr("Auto: ") : QString()) + QString::number(info.gcry_md_iterations));
	ui->lineEdit_num_nodes->setText(QString::number(numNodes));
	ui->lineEdit_num_pdef->setText(QString::number(numPropertyDefinitions));
}

PropertiesDialog::~PropertiesDialog() {
	delete ui;
}

void PropertiesDialog::on_pushButton_clicked() {
    this->close();
}

void PropertiesDialog::on_pushButton_2_clicked() {
	QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEdit_filename->text()).absolutePath()));
}
