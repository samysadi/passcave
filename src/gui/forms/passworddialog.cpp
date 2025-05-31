#include "passworddialog.h"
#include "ui_passworddialog.h"

#include "gcry.h"

#include <QMessageBox>

using namespace passcave;

QColor const colorWar0(255,0,0);

PasswordDialog::PasswordDialog(QWidget* parent, QString title, bool passwordOnly) :
	QDialog(parent),
	ui(new Ui::PasswordDialog) {
	ui->setupUi(this);

	if (title.isEmpty())
		title = passwordOnly ? tr("Enter password") : tr("Choose encryption method");
	setWindowTitle(title);

	if (passwordOnly) {
		ui->label_password->setVisible(false);

		ui->label_cipher->setVisible(false);
		ui->label_cipher_mode->setVisible(false);
		ui->label_md->setVisible(false);
		ui->label_iterations->setVisible(false);

		ui->comboBox_cipher->setVisible(false);
		ui->comboBox_cipher_mode->setVisible(false);
		ui->comboBox_md->setVisible(false);
		ui->widget_iterations->setVisible(false);

		ui->label_warning0->setVisible(false);
	} else {
		updateComboboxes();
	}

	int v = genRandomMdIterations();
	ui->spinBox_iterations->setValue(v);
	ui->checkBox_iterations->setChecked(true);

	this->setFixedSize(this->size().width(), this->sizeHint().height());
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

	ui->lineEdit_password->setFocus();
	ui->pushButton_ok->setDefault(true);
}

PasswordDialog::~PasswordDialog() {
	delete ui;
}

std::string PasswordDialog::getPassword() {
	return ui->lineEdit_password->text().toStdString();
}

gcry_cipher_algos PasswordDialog::getCipher() {
	if (ui->comboBox_cipher->count() == 0 || ui->comboBox_cipher->currentIndex() < 0)
		return GCRY_CIPHER_NONE;
	return static_cast<gcry_cipher_algos>(ui->comboBox_cipher->currentData().toInt());
}

gcry_cipher_modes PasswordDialog::getCipherMode() {
	if (ui->comboBox_cipher_mode->count() == 0 || ui->comboBox_cipher_mode->currentIndex() < 0)
		return GCRY_CIPHER_MODE_NONE;
	return static_cast<gcry_cipher_modes>(ui->comboBox_cipher_mode->currentData().toInt());
}

gcry_md_algos PasswordDialog::getMessageDigest() {
	if (ui->comboBox_md->count() == 0 || ui->comboBox_md->currentIndex() < 0)
		return GCRY_MD_NONE;
	return static_cast<gcry_md_algos>(ui->comboBox_md->currentData().toInt());
}

int PasswordDialog::getMessageDigestIterations() {
	return ui->spinBox_iterations->value();
}

bool PasswordDialog::isApplied() {
	return applied;
}

bool PasswordDialog::isMessageDigestIterationsAuto() {
	return ui->checkBox_iterations->isChecked();
}

void PasswordDialog::updateComboboxes() {
	bool selected;
	int pref;

	gcry_cipher_algos s0 = getCipher();
	gcry_cipher_modes s1 = getCipherMode();
	gcry_md_algos s2 = getMessageDigest();

	ui->comboBox_cipher->clear();
	ui->comboBox_cipher_mode->clear();
	ui->comboBox_md->clear();

	selected = false;
	pref = 0;
	ui->comboBox_cipher->blockSignals(true);
	for (auto v: gcrypt_getCipherAlgos(true)) {
		ui->comboBox_cipher->addItem(QString::fromStdString(toString(v)), QVariant(static_cast<int>(v)));
		if (v == s0) {
			ui->comboBox_cipher->setCurrentIndex(ui->comboBox_cipher->count() - 1);
			selected = true;
		}
		if (v == GCRY_CIPHER_AES256)
			pref = ui->comboBox_cipher->count() - 1;
	}
	if (!selected) {
		ui->comboBox_cipher->setCurrentIndex(pref);
		s0 = getCipher();
	}
	ui->comboBox_cipher->blockSignals(false);


	selected = false;
	pref = 0;
	ui->comboBox_cipher_mode->blockSignals(true);
	for (auto v: gcrypt_getCipherModes(true)) {
		ui->comboBox_cipher_mode->addItem(QString::fromStdString(toString(v)), QVariant(static_cast<int>(v)));
		if (v == s1) {
			ui->comboBox_cipher_mode->setCurrentIndex(ui->comboBox_cipher_mode->count() - 1);
			selected = true;
		}
		if (v == GCRY_CIPHER_MODE_GCM)
			pref = ui->comboBox_cipher_mode->count() - 1;
	}
	if (!selected) {
		ui->comboBox_cipher_mode->setCurrentIndex(pref);
		s1 = getCipherMode();
	}
	ui->comboBox_cipher_mode->blockSignals(false);

	int keyLength = gcrypt_algo_keyLength(s0);

	selected = false;
	pref = 0;
	ui->comboBox_md->blockSignals(true);
	for (auto v: gcrypt_getMdAlgos(true)) {
		int hashSize = gcrypt_mdSize(v);
		if (hashSize < keyLength)
			continue;
		ui->comboBox_md->addItem(QString::fromStdString(toString(v)), QVariant(static_cast<int>(v)));

		if (hashSize > keyLength)
			ui->comboBox_md->setItemData(ui->comboBox_md->count() - 1, colorWar0, Qt::TextColorRole);

		if (v == s2) {
			ui->comboBox_md->setCurrentIndex(ui->comboBox_md->count() - 1);
			selected = true;
		}
		if (v == GCRY_MD_SHA256)
			pref = ui->comboBox_md->count() - 1;
	}
	if (!selected) {
		ui->comboBox_md->setCurrentIndex(pref);
		s2 = getMessageDigest();
	}
	ui->comboBox_md->blockSignals(false);

	on_comboBox_md_currentIndexChanged(0);
}

void PasswordDialog::on_pushButton_ok_clicked() {
	if (ui->lineEdit_password->text().length() < 6) {
		QMessageBox box(QMessageBox::Question, tr("Error"), tr("Please, enter a password of at least %n characters.", "", 6), QMessageBox::Ok, this);
		box.exec();
		return;
	}

	this->applied = true;
	this->close();
}

void PasswordDialog::on_pushButton_cancel_clicked() {
	this->close();
}

void PasswordDialog::on_comboBox_cipher_currentIndexChanged(int index) {
	updateComboboxes();
}

void PasswordDialog::on_comboBox_md_currentIndexChanged(int index) {
	ui->label_warning0->setVisible(ui->comboBox_md->currentData(Qt::TextColorRole) == colorWar0);
}

void PasswordDialog::on_pushButton_toggled(bool checked) {
	ui->lineEdit_password->setEchoMode(ui->pushButton->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
}

void PasswordDialog::on_checkBox_iterations_toggled(bool checked) {
	ui->spinBox_iterations->setEnabled(!ui->checkBox_iterations->isChecked());
}
