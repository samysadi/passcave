#pragma once

#include "gcry.h"

#include <QDialog>

#include <string>

namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
	Q_OBJECT

public:
	PasswordDialog(QWidget* parent, QString title, bool passwordOnly = false);
	~PasswordDialog();

	std::string getPassword();
	gcry_cipher_algos getCipher();
	gcry_cipher_modes getCipherMode();
	gcry_md_algos getMessageDigest();
	int getMessageDigestIterations();
	bool isApplied();
	bool isMessageDigestIterationsAuto();

private:
	Ui::PasswordDialog* ui;
	bool applied = false;

	void updateComboboxes();

private slots:
	void on_pushButton_ok_clicked();
	void on_pushButton_cancel_clicked();
	void on_comboBox_cipher_currentIndexChanged(int index);
	void on_comboBox_md_currentIndexChanged(int index);
	void on_pushButton_toggled(bool checked);
	void on_checkBox_iterations_toggled(bool checked);
};
