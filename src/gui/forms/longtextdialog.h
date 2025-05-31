#pragma once

#include <QString>
#include <QDialog>

namespace Ui {
class LongTextDialog;
}

class LongTextDialog : public QDialog
{
	Q_OBJECT

public:
	explicit LongTextDialog(QWidget* parent, bool editable,
							QString propertyName,
							QString initialText);
	~LongTextDialog();

	QString getContents();
	bool isApplied();

protected:
	void closeEvent(QCloseEvent* e);

private slots:
	void on_applyButton_clicked();
	void on_cancelButton_clicked();

private:
	Ui::LongTextDialog* ui;
	bool applied;
	QString initialText;
};
