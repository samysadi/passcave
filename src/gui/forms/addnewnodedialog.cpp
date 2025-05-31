#include "addnewnodedialog.h"

#include "gcry.h"

#include "includes/preferences.h"
#include "forms/clickableqlabel.h"
#include "forms/longtextdialog.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <QPlainTextEdit>

#include <ctime>
#include <vector>

AddNewNodeDialog::AddNewNodeDialog(MainWindow* parent,
					   DataModel* dataModel,
					   DocumentNodeId nodeId) :
	QDialog(parent) {
	this->dataModel = dataModel;
	this->nodeId = nodeId;

	node = new SafeDocumentNode();
	if (nodeId != InvalidDocumentNodeId) {
		*node = this->dataModel->getDocument()->getDocumentNode(nodeId);
	} else {
		*node = this->dataModel->getDocument()->getEmptyDocumentNode();
	}

	initialNode = new SafeDocumentNode(*node);

	initUI();
}

AddNewNodeDialog::~AddNewNodeDialog() {
	for (auto& it: node->propertyValues)
		secureErase(it.second);
	delete node;

	for (auto& it: initialNode->propertyValues)
		secureErase(it.second);
	delete initialNode;
}

MainWindow* AddNewNodeDialog::getMainWindow() const {
	return static_cast<MainWindow*>(this->parent());
}

void AddNewNodeDialog::closeEvent(QCloseEvent* e) {
	if (!this->applied && this->isModified) {
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

QFlags<Qt::AlignmentFlag> const COL0_ALIGN = Qt::AlignVCenter | Qt::AlignRight;
QFlags<Qt::AlignmentFlag> const COL1_ALIGN = Qt::AlignVCenter | Qt::AlignLeft;
QFlags<Qt::AlignmentFlag> const COL2_ALIGN = Qt::AlignVCenter| Qt::AlignRight;
QFlags<Qt::AlignmentFlag> const COL12_ALIGN = Qt::AlignVCenter | Qt::AlignLeft;
int const FORM_WIDTH = 500;
int const COL0_STRETCH = 1;
int const COL1_STRETCH = 4;
int const COL2_STRETCH = 1;

std::string const DEFAULT_PASSWORD_GEN = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&()*+,-./:;<=>?@[]_{|}\\'\"~`^";
int const DEFAULT_PASSWORD_LEN = 12;

QLabel* AddNewNodeDialog::createQLabel(DocumentPropertyDefinition pDef) {
	ClickableQLabel* w = new ClickableQLabel(QString::fromStdString(DataModel::formatHeader(pDef.name)), this);
	QFont font = w->font();
	font.setBold(true);
	font.setPointSize(9);
	w->setCursor(QCursor(Qt::PointingHandCursor));
	w->setFont(font);
	w->setToolTip(QString::fromStdString(pDef.description));
	w->setAlignment(COL0_ALIGN);
	w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	w->setFixedWidth(FORM_WIDTH * COL0_STRETCH / (COL0_STRETCH + COL1_STRETCH + COL2_STRETCH));
	w->setWordWrap(true);
	return w;
}

QLineEdit* AddNewNodeDialog::createQLineEdit(DocumentPropertyDefinition pDef) {
	QLineEdit* w = new QLineEdit(this);
	w->setPlaceholderText(tr("Enter %1").arg(QString::fromStdString(DataModel::formatHeader(pDef.name))));
	w->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
	return w;
}

inline QString getTextArrayValue(SafeDocumentNode* node, std::string propertyName) {
	std::vector<std::string> v = Document::convertTextArray(node->propertyValues[propertyName]);
	QString r;
	for (std::string s: v) {
		r.append(QString::fromStdString(s));
		r.append("; ");
	}
	return r;
}

void AddNewNodeDialog::initUI() {
	bool readOnly0 = Preferences::isDefaultOpenModeReadOnly();

	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setModal(true);
	this->setWindowTitle(nodeId == InvalidDocumentNodeId ? tr("Adding node") : tr("Editing Node"));

	QGridLayout* layout = new QGridLayout(this);
	layout->setSizeConstraint(QLayout::SetDefaultConstraint);
	layout->setColumnStretch(0, COL0_STRETCH);
	layout->setColumnStretch(1, COL1_STRETCH);
	layout->setColumnStretch(2, COL2_STRETCH);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setHorizontalSpacing(4);
	layout->setVerticalSpacing(4);
	setLayout(layout);

	int const W1 = FORM_WIDTH * COL1_STRETCH / (COL0_STRETCH + COL1_STRETCH + COL2_STRETCH);
	int const W2 = FORM_WIDTH * COL2_STRETCH / (COL0_STRETCH + COL1_STRETCH + COL2_STRETCH);
	int const W12 = W1 + W2 + layout->horizontalSpacing();

	int row = 0;

	for (DocumentPropertyDefinition pDef: this->dataModel->getDocument()->getPropertyDefinitions()) {
		std::vector<QWidget*> g;

		if (dataModel->isSequenceColumn(pDef.name))
			continue;

		bool readOnly = readOnly0;
		switch (pDef.type) {
		case DocumentPropertyType::DPT_BOOL:
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QComboBox* w = new QComboBox(this);
				connect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(onChange()));
				w->setEnabled(!readOnly0);
				w->addItem(tr("False"));
				w->addItem(tr("True"));
				w->setFixedWidth(W12);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				w->setCurrentIndex(Document::convertBool(node->propertyValues[pDef.name]) ? 1 : 0);
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, 1, 2, COL12_ALIGN);
			}
			break;
		case DocumentPropertyType::DPT_NUMBER:
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QSpinBox* w = new QSpinBox(this);
				connect(w, SIGNAL(valueChanged(int)), this, SLOT(onChange()));
				w->setReadOnly(readOnly);
				w->setFixedWidth(W12);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				w->setValue(static_cast<int>(Document::convertNumber(node->propertyValues[pDef.name])));
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, 1, 2, COL12_ALIGN);
			}
			break;
		case DocumentPropertyType::DPT_DATE:
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QDateEdit* w = new QDateEdit(this);
				connect(w, SIGNAL(dateChanged(QDate)), this, SLOT(onChange()));
				w->setDisplayFormat("yyyy-M-d");
				w->setCalendarPopup(true);
				w->setReadOnly(readOnly);
				w->setFixedWidth(W12);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				struct tm t = Document::convertDate(node->propertyValues[pDef.name]);
				w->setDate(QDate(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday));
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, 1, 2, COL12_ALIGN);
			}
			break;
		case DocumentPropertyType::DPT_DATETIME:
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QDateTimeEdit* w = new QDateTimeEdit(this);
				connect(w, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(onChange()));
				w->setDisplayFormat("yyyy-M-dTH:m:s");
				w->setCalendarPopup(true);
				w->setReadOnly(readOnly);
				w->setFixedWidth(W12);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				struct tm t = Document::convertDate(node->propertyValues[pDef.name]);
				w->setDateTime(QDateTime(QDate(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday),
										 QTime(t.tm_hour, t.tm_min, t.tm_sec)));
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, 1, 2, COL12_ALIGN);
			}
			break;
		case DocumentPropertyType::DPT_TIME:
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QTimeEdit* w = new QTimeEdit(this);
				connect(w, SIGNAL(timeChanged(QTime)), this, SLOT(onChange()));
				w->setCalendarPopup(true);
				w->setDisplayFormat("H:m:s");
				w->setReadOnly(readOnly);
				w->setFixedWidth(W12);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				struct tm t = Document::convertDate(node->propertyValues[pDef.name]);
				w->setTime(QTime(t.tm_hour, t.tm_min, t.tm_sec));
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, 1, 2, COL12_ALIGN);
			}
			break;
		case DocumentPropertyType::DPT_LONGTEXT:
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QPushButton* w = new QPushButton(this);
				w->setText(readOnly ? tr("Click to show") : tr("Click to edit"));
				w->setFixedWidth(W1);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, COL1_ALIGN);

				connect(w, SIGNAL(clicked()), this, SLOT(onDisplayLongTextClicked()));
			}

			{
				QLabel* w = new QLabel(this);
				w->setText(tr("[unmodified]"));
				w->setFixedWidth(W2);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				g.push_back(w);
				layout->addWidget(w, row, 2, COL2_ALIGN);
			}
			break;
		case DocumentPropertyType::DPT_TEXTARRAY: {
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QLineEdit* w = createQLineEdit(pDef);
				w->setPlaceholderText(tr("Use + and - to add / remove %1").arg(QString::fromStdString(DataModel::formatHeader(pDef.name))));
				w->selectAll();
				w->setReadOnly(true);
				w->setFixedWidth(readOnly ? W12 : W1);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				w->setText(getTextArrayValue(node, pDef.name));
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, 1, readOnly ? 2 : 1, readOnly ? COL12_ALIGN : COL1_ALIGN);
			}

			if (!readOnly) {
				QHBoxLayout* layout2 = new QHBoxLayout();
				layout->addLayout(layout2, row, 2, COL2_ALIGN);

				int bw = W2 / 2;

				QPushButton* b1 = new QPushButton("-", this);
				layout2->addWidget(b1);
				b1->setFixedWidth(bw);
				b1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				g.push_back(b1);
				connect(b1, SIGNAL(clicked(bool)), this, SLOT(onTextArrayRemoveClicked()));
				b1->setEnabled(!node->propertyValues[pDef.name].empty());
				QPushButton* b2 = new QPushButton("+", this);
				layout2->addWidget(b2);
				b2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				b2->setFixedWidth(bw);
				g.push_back(b2);
				connect(b2, SIGNAL(clicked(bool)), this, SLOT(onTextArrayAddClicked()));
			}

			break;
		}

		case DocumentPropertyType::DPT_PASSWORD:
			if (nodeId == InvalidDocumentNodeId) {
				node->propertyValues[pDef.name] = genPassword(QString::fromStdString(DEFAULT_PASSWORD_GEN), DEFAULT_PASSWORD_LEN).toStdString();
			}

			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QLineEdit* w = createQLineEdit(pDef);
				connect(w, SIGNAL(textChanged(QString)), this, SLOT(onChange()));
				w->setReadOnly(readOnly);
				w->setInputMethodHints(
					w->inputMethodHints() |
					Qt::InputMethodHint::ImhHiddenText |
					Qt::InputMethodHint::ImhSensitiveData |
					Qt::InputMethodHint::ImhNoPredictiveText
				);
				w->setEchoMode(QLineEdit::Normal);
				w->setFixedWidth(W12);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				w->setText(QString::fromStdString(Document::convertPassword(node->propertyValues[pDef.name])));
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, COL12_ALIGN);
			}

			if (!readOnly) {
				row++;

				{
					QHBoxLayout* layout2 = new QHBoxLayout();
					layout->addLayout(layout2, row, 1, 1, 2, COL12_ALIGN);

					{
						QLineEdit* w = createQLineEdit(pDef);
						w->setReadOnly(readOnly);
						w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
						w->setText(QString::fromStdString(DEFAULT_PASSWORD_GEN));
						g.push_back(w);
						layout2->addWidget(w);
					}

					{
						QSpinBox* w = new QSpinBox(this);
						w->setReadOnly(readOnly);
						w->setFixedWidth(50);
						w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
						w->setValue(DEFAULT_PASSWORD_LEN);
						w->setMinimum(4);
						w->setMaximum(255);
						g.push_back(w);
						layout2->addWidget(w);
					}

					{
						QPushButton* w = new QPushButton(tr("Gen"), this);
						g.push_back(w);
						layout2->addWidget(w);
						connect(w, SIGNAL(clicked()), this, SLOT(onGenPasswordClicked()));
					}
				}
			}

			break;

        case DocumentPropertyType::DPT_OTPAUTH:
            {
                QLabel* w = createQLabel(pDef);
                g.push_back(w);
                layout->addWidget(w, row, 0, COL0_ALIGN);
            }

            {
                QLineEdit* w = createQLineEdit(pDef);
                connect(w, SIGNAL(textChanged(QString)), this, SLOT(onChange()));
                w->setReadOnly(readOnly);
                w->setInputMethodHints(
                    w->inputMethodHints() |
                    Qt::InputMethodHint::ImhHiddenText |
                    Qt::InputMethodHint::ImhSensitiveData |
                    Qt::InputMethodHint::ImhNoPredictiveText
                    );
                w->setEchoMode(QLineEdit::Normal);
                w->setFixedWidth(W12);
                w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
                w->setText(QString::fromStdString(Document::convertURI(node->propertyValues[pDef.name])));
                linkQLabel(static_cast<QLabel*>(g[0]), w);
                g.push_back(w);
                layout->addWidget(w, row, 1, COL12_ALIGN);
            }

            break;
		case DocumentPropertyType::DPT_URI:
		case DocumentPropertyType::DPT_TEXT:
		default:
			{
				QLabel* w = createQLabel(pDef);
				g.push_back(w);
				layout->addWidget(w, row, 0, COL0_ALIGN);
			}

			{
				QLineEdit* w = createQLineEdit(pDef);
				connect(w, SIGNAL(textChanged(QString)), this, SLOT(onChange()));
				w->setReadOnly(readOnly);
				w->setFixedWidth(W12);
				w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
				w->setText(QString::fromStdString(DocumentPropertyType::DPT_URI == pDef.type ? Document::convertURI(node->propertyValues[pDef.name])
							 : Document::convertText(node->propertyValues[pDef.name])));
				linkQLabel(static_cast<QLabel*>(g[0]), w);
				g.push_back(w);
				layout->addWidget(w, row, 1, 1, 2, COL12_ALIGN);
			}
			break;
		}

		row++;

		this->propertyWidgets[pDef.name] = g;
	}

	{
		QHBoxLayout* layout2 = new QHBoxLayout();
		layout->addLayout(layout2, row++, 1, 1, 2, Qt::AlignBottom | Qt::AlignRight);

		bool setDefault = false;
		if (!readOnly0) {
			QPushButton* b1 = new QPushButton(nodeId == InvalidDocumentNodeId ? tr("Create") : tr("Update"), this);
			layout2->addWidget(b1);
			//if (nodeId == InvalidDocumentNodeId) {
				b1->setDefault(true);
				setDefault = true;
			//}
			connect(b1, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
		}
		{
			QPushButton* b2 = new QPushButton(tr("Cancel"), this);
			layout2->addWidget(b2);
			if (!setDefault) {
				b2->setDefault(true);
				//setDefault = true;
			}
			connect(b2, SIGNAL(clicked()), this, SLOT(onCancelClicked()));
		}
	}

	int left, right, top, bottom;
	layout->getContentsMargins(&left, &top, &right, &bottom);
	this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	this->setFixedSize(FORM_WIDTH + left + right + layout->horizontalSpacing() * 2, this->sizeHint().height());
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}

void AddNewNodeDialog::linkQLabel(QLabel* l, QWidget* w) {
	l->setBuddy(w);
}

QString AddNewNodeDialog::genPassword(QString chars, int len) {
	QString r;
	std::vector<char> v = genRandom(len);
	for (char& vv: v)
		r.push_back(chars[static_cast<unsigned int>(vv) % chars.length()]);
	secureErase(v);
	return r;
}

std::string AddNewNodeDialog::getPropertyNameFromSender(QObject* sender) {
	for (auto it: this->propertyWidgets) {
		for (QWidget* w: it.second)
			if (w == sender)
				return it.first;
	}
	return "";
}

bool AddNewNodeDialog::isDataPropertyModified(std::string propertyName) {
	auto it = node->propertyValues.find(propertyName);
	auto itInit = initialNode->propertyValues.find(propertyName);

	if (it == node->propertyValues.end()) {
		if (itInit == initialNode->propertyValues.end())
			return false;
		else
			return !itInit->second.empty();
	} else {
		if (itInit == initialNode->propertyValues.end())
			return !it->second.empty();
		else
			return it->second.compare(itInit->second) != 0;
	}
}

void AddNewNodeDialog::onApplyClicked() {
	this->applied = true;

	if (this->nodeId == InvalidDocumentNodeId) {
		this->nodeId = this->dataModel->insertData(this->node);
	} else {
		this->dataModel->updateData(this->nodeId, this->node);
	}

	this->close();
}

void AddNewNodeDialog::onCancelClicked() {
	this->close();
}

inline void setStylesheetIfModified(QWidget* w, bool currentDataModified) {
	w->setStyleSheet(currentDataModified ? "background-color:#FFe0ef;" : "");
}

void AddNewNodeDialog::onChange(QWidget* sender) {
	if (sender == nullptr)
		sender = dynamic_cast<QWidget*>(QObject::sender());

	std::string propertyName = getPropertyNameFromSender(sender);
	if (propertyName.empty())
		return;
	std::vector<QWidget*> const& g = this->propertyWidgets[propertyName];
	DocumentPropertyDefinition pDef = this->dataModel->getDocument()->getPropertyDefinition(propertyName);

	switch (pDef.type) {
	case DocumentPropertyType::DPT_BOOL: {
		QComboBox* w = dynamic_cast<QComboBox*>(g[1]);
		node->propertyValues[pDef.name] = validateDocumentPropertyValue(pDef.type, w->currentIndex() == 0 ? "0" : "1");
		setStylesheetIfModified(w, isDataPropertyModified(propertyName));
		break;
	} case DocumentPropertyType::DPT_NUMBER: {
		QSpinBox* w = dynamic_cast<QSpinBox*>(g[1]);
		node->propertyValues[pDef.name] = validateDocumentPropertyValue(pDef.type, std::to_string(w->value()));
		setStylesheetIfModified(w, isDataPropertyModified(propertyName));
		break;
	} case DocumentPropertyType::DPT_DATE: {
		QDateEdit* w = dynamic_cast<QDateEdit*>(g[1]);
		QDate v = w->date();
		struct tm t;
		t.tm_hour	= -1;
		t.tm_min	= -1;
		t.tm_sec	= -1;
		t.tm_year	= v.year() - 1900;
		t.tm_mon	= v.month() - 1;
		t.tm_mday	= v.day();
		t.tm_wday	= v.dayOfWeek();
		t.tm_yday	= v.dayOfYear();
		t.tm_isdst	= -1;
		node->propertyValues[pDef.name] = validateDocumentPropertyValue(pDef.type, dateToStr(t));
		setStylesheetIfModified(w, isDataPropertyModified(propertyName));
		break;
	} case DocumentPropertyType::DPT_DATETIME: {
		QDateTimeEdit* w = dynamic_cast<QDateTimeEdit*>(g[1]);
		QDateTime v = w->dateTime();
		struct tm t;
		t.tm_hour	= v.time().hour();
		t.tm_min	= v.time().minute();
		t.tm_sec	= v.time().second();
		t.tm_year	= v.date().year() - 1900;
		t.tm_mon	= v.date().month() - 1;
		t.tm_mday	= v.date().day();
		t.tm_wday	= v.date().dayOfWeek();
		t.tm_yday	= v.date().dayOfYear();
		t.tm_isdst	= -1;
		node->propertyValues[pDef.name] = validateDocumentPropertyValue(pDef.type, dateTimeToStr(t));
		setStylesheetIfModified(w, isDataPropertyModified(propertyName));
		break;
	} case DocumentPropertyType::DPT_TIME:{
		QTimeEdit* w = dynamic_cast<QTimeEdit*>(g[1]);
		QTime v = w->time();
		struct tm t;
		t.tm_hour	= v.hour();
		t.tm_min	= v.minute();
		t.tm_sec	= v.second();
		t.tm_year	= -1;
		t.tm_mon	= -1;
		t.tm_mday	= -1;
		t.tm_wday	= -1;
		t.tm_yday	= -1;
		t.tm_isdst	= -1;
		node->propertyValues[pDef.name] = validateDocumentPropertyValue(pDef.type, timeToStr(t));
		setStylesheetIfModified(w, isDataPropertyModified(propertyName));
		break;
	} case DocumentPropertyType::DPT_LONGTEXT: {
		QFont f;
		if (isDataPropertyModified(propertyName)) {
			f.setBold(true);
			dynamic_cast<QLabel*>(g[2])->setFont(f);
			dynamic_cast<QLabel*>(g[2])->setText(tr("[modified]"));
		} else {
			dynamic_cast<QLabel*>(g[2])->setFont(f);
			dynamic_cast<QLabel*>(g[2])->setText(tr("[unmodified]"));
		}
		break;
	} case DocumentPropertyType::DPT_TEXTARRAY: {
		QLineEdit* w = dynamic_cast<QLineEdit*>(g[1]);
		setStylesheetIfModified(w, isDataPropertyModified(propertyName));
		g[2]->setEnabled(!node->propertyValues[pDef.name].empty());
		break;
	} case DocumentPropertyType::DPT_PASSWORD:
    //case DocumentPropertyType::DPT_OTPAUTH:
	//case DocumentPropertyType::DPT_URI:
	//case DocumentPropertyType::DPT_TEXT:
	default: {
		QLineEdit* w = dynamic_cast<QLineEdit*>(g[1]);
		node->propertyValues[pDef.name] = validateDocumentPropertyValue(pDef.type, w->text().toStdString());
		setStylesheetIfModified(w, isDataPropertyModified(propertyName));
		break;
	}
	}

	this->isModified = false;
	for (auto v: node->propertyValues)
		if (isDataPropertyModified(v.first)) {
			isModified = true;
			break;
		}
}

void AddNewNodeDialog::onDisplayLongTextClicked() {
	std::string propertyName = getPropertyNameFromSender(sender());
	if (propertyName.empty())
		return;
	std::vector<QWidget*> const& g = this->propertyWidgets[propertyName];

	DocumentPropertyDefinition pDef = this->dataModel->getDocument()->getPropertyDefinition(propertyName);

	QString hd = QString::fromStdString(DataModel::formatHeader(propertyName));
	QString val = QString::fromStdString(node->propertyValues[propertyName]);

	LongTextDialog d(this,
					   !Preferences::isDefaultOpenModeReadOnly(),
					   hd,
					   val);
	d.exec();

	if (d.isApplied()) {
		if (d.getContents().compare(val) == 0)
			return;
		node->propertyValues[propertyName] = validateDocumentPropertyValue(pDef.type, d.getContents().toStdString());
		onChange(g[0]);
	}
}

void AddNewNodeDialog::onGenPasswordClicked() {
	std::string propertyName = getPropertyNameFromSender(sender());
	if (propertyName.empty())
		return;
	std::vector<QWidget*> const& g = this->propertyWidgets[propertyName];

	QString chars = dynamic_cast<QLineEdit*>(g[2])->text();
	if (chars.length() == 0)
		chars = QString::fromStdString(DEFAULT_PASSWORD_GEN);

	int len = dynamic_cast<QSpinBox*>(g[3])->value();
	if (len <= 0)
		len = DEFAULT_PASSWORD_LEN;

	dynamic_cast<QLineEdit*>(g[1])->setText(genPassword(chars, len));
	onChange(g[1]);
}

void AddNewNodeDialog::onTextArrayAddClicked() {
	std::string propertyName = getPropertyNameFromSender(sender());
	if (propertyName.empty())
		return;
	std::vector<QWidget*> const& g = this->propertyWidgets[propertyName];

	DocumentPropertyDefinition pDef = this->dataModel->getDocument()->getPropertyDefinition(propertyName);

	bool ok;
	QString val = QInputDialog::getText(this,
						  tr("Enter %1 item to add").arg(QString::fromStdString(DataModel::formatHeader(propertyName))),
						  QString::fromStdString(DataModel::formatHeader(propertyName) + ":"),
						  QLineEdit::EchoMode::Normal,
						  "",
						  &ok);

	if (!ok)
		return;

	std::string& v = node->propertyValues[propertyName];
	if (!v.empty())
		v = v + "\n";
	v = v + val.toStdString();
	secureErase(val);
	dynamic_cast<QLineEdit*>(g[1])->setText(getTextArrayValue(node, propertyName));
	onChange(g[1]);
}

void AddNewNodeDialog::onTextArrayRemoveClicked() {
	std::string propertyName = getPropertyNameFromSender(sender());
	if (propertyName.empty())
		return;
	std::vector<QWidget*> const& g = this->propertyWidgets[propertyName];

	QStringList items;

	{
		std::vector<std::string> v = Document::convertTextArray(node->propertyValues[propertyName]);
		for (std::string& s: v) {
			items.push_back(QString::fromStdString(s));
			secureErase(s);
		}
	}

	if (items.empty())
		return;

	bool ok;
	QString val = QInputDialog::getItem(this,
						  tr("Select %1 item to remove").arg(QString::fromStdString(DataModel::formatHeader(propertyName))),
						  QString::fromStdString(DataModel::formatHeader(propertyName) + ":"),
						  items,
						  0,
						  false,
						  &ok);
	if (!ok)
		return;

	std::vector<char> ss;

	for (QString& s: items) {
		if (s.compare(val) == 0) {

		} else {
			std::string t = s.toStdString();
			ss.insert(ss.end(), t.c_str(), t.c_str() + t.size());
			ss.push_back('\n');
			secureErase(t);
		}
		secureErase(s);
	}
	if (!ss.empty())
		ss.pop_back();

	node->propertyValues[propertyName] = std::string(ss.data(), ss.size());
	dynamic_cast<QLineEdit*>(g[1])->setText(getTextArrayValue(node, propertyName));
	onChange(g[1]);
}
