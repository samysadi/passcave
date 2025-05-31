#pragma once

#include "gcry.h"

#include <QDialog>

namespace Ui {
class PropertiesDialog;
}

class PropertiesDialog : public QDialog
{
	Q_OBJECT
public:
	PropertiesDialog(QWidget* parent,
					 std::string const& filename,
					 passcave::gcrypt_FileInfo const& info,
					 int numNodes,
					 int numPropertyDefinitions);
	~PropertiesDialog();

private:
	Ui::PropertiesDialog* ui;

private slots:
	void on_pushButton_clicked();
	void on_pushButton_2_clicked();
};
