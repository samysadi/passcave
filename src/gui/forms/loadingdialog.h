#pragma once

#include <QCloseEvent>
#include <QDialog>

namespace Ui {
class LoadingDialog;
}

class LoadingDialog : public QDialog
{
	Q_OBJECT

public:
	LoadingDialog(QWidget *parent, QString title);
	~LoadingDialog();

protected:
	void closeEvent(QCloseEvent* e);

private:
	Ui::LoadingDialog *ui;
};
