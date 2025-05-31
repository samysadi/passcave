#include "loadingdialog.h"
#include "ui_loadingdialog.h"

LoadingDialog::LoadingDialog(QWidget *parent, QString title) :
	QDialog(parent),
	ui(new Ui::LoadingDialog) {
	ui->setupUi(this);

	this->setWindowTitle(title);
	ui->label_info->setText(title);
	this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
}

LoadingDialog::~LoadingDialog() {
	delete ui;
}

void LoadingDialog::closeEvent(QCloseEvent* e) {
	if (e->spontaneous()) {
		e->ignore();
		return;
	}

	QDialog::closeEvent(e);
}
