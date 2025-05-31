#include "datamodel.h"
#include "utils.h"

#include "includes/preferences.h"

#include <QFont>
#include <QColor>

using namespace passcave;

void DataModel::updateNodeIds() {
	int i = 0;
	this->nodeIdsMap.clear();
	this->nodeIdsMapRev.clear();
	auto a = this->document->getNodeIds();
	this->nodeIdsMap.reserve(a.size());
	this->nodeIdsMapRev.reserve(a.size());
	for (auto const& v: a) {
		if (filteredNodes != NULL && filteredNodes->find(v) == filteredNodes->end())
			continue;
		this->nodeIdsMap[i] = v;
		this->nodeIdsMapRev[v] = i;
		i++;
	}
}

DataModel::DataModel(passcave::Document* document, std::string seqPropertyName) {
	beginResetModel();

	this->filteredNodes = NULL;

	this->document = document;
	this->seqPropertyName = normalizePropertyName(seqPropertyName);

	{
		int i = 0;
		this->propertyNamesMap.clear();
		this->propertyNamesMapRev.clear();
		auto a = this->document->getPropertyDefinitions();
		this->propertyNamesMap.reserve(a.size());
		this->propertyNamesMapRev.reserve(a.size());
		for (auto const& v: a) {
			this->propertyNamesMap[i] = v.name;
			this->propertyNamesMapRev[v.name] = i;
			i++;
		}
	}

	updateNodeIds();

	endResetModel();
}

DataModel::~DataModel() {
	delete this->document;
	if (this->filteredNodes != NULL)
		delete this->filteredNodes;
}

bool DataModel::addMissingDocumentProperties() {
    bool modified = this->document->addDefaultPropertyDefinitions();
    if (!modified) {
        return false;
    }

    emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, columnCount() - 1));
    allNodesModified = true;
    return true;
}

int DataModel::columnCount(QModelIndex const& parent) const {
	return this->document->getPropertyDefinitionsCount();
}

QVariant DataModel::data(QModelIndex const& index, int role) const {
	if (!index.isValid())
		return QVariant::Invalid;

	if (role == Qt::TextAlignmentRole) {
		return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
	} else if (role == Qt::DisplayRole || role == Qt::UserRole || role == Qt::EditRole) {
		DocumentNodeId nodeId = getNodeIdFromRowIndex(index.row());
		std::string propertyName = getPropertyNameFromColumnIndex(index.column());

		try {
            std::string value = this->document->getProperty(nodeId, propertyName);

			DocumentPropertyDefinition propertyDefinition = this->document->getPropertyDefinition(propertyName);
			if (role == Qt::DisplayRole) {
                if (propertyDefinition.type == DocumentPropertyType::DPT_PASSWORD || propertyDefinition.type == DocumentPropertyType::DPT_OTPAUTH) {
                    if (Preferences::isObscurePasswords())
                        return value.empty() ? "" : "******";
                } else if (propertyDefinition.type == DocumentPropertyType::DPT_LONGTEXT) {
					return tr("...");
				}
			}

			if (propertyDefinition.type == DocumentPropertyType::DPT_TEXTARRAY)
				for (char& c: value)
					if (c == '\n')
						c = ';';
			QString qValue = QString::fromStdString(value);
			secureErase(value);
			return qValue;
		} catch (DocumentException const& e) {
			return QVariant::Invalid;
		}
	} else if (role == Qt::ToolTipRole) {
		std::string propertyName = getPropertyNameFromColumnIndex(index.column());

		return QString::fromStdString(this->document->getPropertyDefinition(propertyName).description);
	} else if (role == Qt::TextAlignmentRole) {
		std::string propertyName = getPropertyNameFromColumnIndex(index.column());
		try {
			DocumentPropertyDefinition propertyDefinition = this->document->getPropertyDefinition(propertyName);
			if (propertyDefinition.type == DocumentPropertyType::DPT_LONGTEXT) {
				return Qt::AlignCenter;
			}
		} catch (DocumentException const& e) {
			return QVariant::Invalid;
		}
		return Qt::AlignLeft;
	} else if (role == Qt::BackgroundColorRole) {
		DocumentNodeId nodeId = getNodeIdFromRowIndex(index.row());
		std::string propertyName = getPropertyNameFromColumnIndex(index.column());

		bool modified = false;
		if (allNodesModified && isSequenceColumn(propertyName))
			modified = true;
		else {
			auto it = modifiedNodeIds.find(nodeId);
			if (it != modifiedNodeIds.end()) {
				if (it->second.find(propertyName) != it->second.end())
					modified = true;
			}
		}

		if (modified) {
			if (index.row() % 2 == 0)
				return QColor(255, 232, 216);
			else
				return QColor(248, 225, 209);
		}
	}
	return QVariant::Invalid;
}

void DataModel::deleteIndexes(std::vector<int> indexes) {
	if (indexes.size() == 0)
		return;
	emit layoutAboutToBeChanged();

	std::vector<long long> seqs;
	for (int i: indexes) {
		DocumentNodeId id = getNodeIdFromRowIndex(i);
		std::string p = this->document->getProperty(id, this->seqPropertyName);
		seqs.push_back(strtoll(p.c_str(), NULL, 10));
		this->document->removeNode(id);
	}

	std::vector<DocumentNodeId> nodeIds;

	for (DocumentNodeId id: this->document->getNodeIds()) {
		std::string p = this->document->getProperty(id, this->seqPropertyName);
		long long oldSeq = strtoll(p.c_str(), NULL, 10);
		long long seq = oldSeq;
		for (long long seq0: seqs)
			if (oldSeq > seq0)
				seq--;
		if (seq != oldSeq) {
			this->document->setProperty(id, this->seqPropertyName, std::to_string(seq));
			nodeIds.push_back(id);
		}
	}

	updateNodeIds();

	int seqCol = getColumnIndexFromPropertyName(this->seqPropertyName);

	for (DocumentNodeId id: nodeIds) {
		int row = getRowIndexFromNodeId(id);
		emit dataChanged(createIndex(row, seqCol), createIndex(row, seqCol));
		if (!allNodesModified)
			modifiedNodeIds[id][this->seqPropertyName] = true;
	}

	emit dataChanged(createIndex(rowCount(), 0), createIndex(rowCount() + indexes.size() - 1, columnCount() - 1));

	emit layoutChanged();
}

bool DataModel::filterData(QString pattern, QString column, bool caseSensitive) {
	QStringList columns;
	if (!column.isEmpty())
		columns.append(column);
	return filterData(pattern, columns, caseSensitive);
}

bool DataModel::filterData(QString pattern, QStringList columns, bool caseSensitive) {
	emit beginResetModel();
	if (filteredNodes != NULL) {
		delete filteredNodes;
		filteredNodes = NULL;
	}

	if (pattern.isEmpty()) {
		updateNodeIds();
		emit endResetModel();
		return true;
	}

	filteredNodes = new std::unordered_map<DocumentNodeId, bool>();

	QRegularExpression regExp(pattern, (caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption) |
							  QRegularExpression::DotMatchesEverythingOption |
							  QRegularExpression::MultilineOption |
							  QRegularExpression::DontCaptureOption |
							  QRegularExpression::UseUnicodePropertiesOption);
	if (!regExp.isValid()) {
		updateNodeIds();
		emit endResetModel();
		return false;
	}
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	regExp.optimize();
#endif

	std::vector<std::string> cols;
	if (columns.isEmpty()) {
		for (DocumentPropertyDefinition const& pDef: document->getPropertyDefinitions())
			cols.push_back(pDef.name);
	} else {
		for (QString const& s: columns)
			cols.push_back(document->getPropertyDefinition(s.toStdString()).name);
	}

	for (DocumentNodeId nodeId: document->getNodeIds()) {
		bool matches = false;
		for (std::string const& propertyName: cols) {
			std::string const val = document->getProperty(nodeId, propertyName);
			QRegularExpressionMatch match = regExp.match(QString::fromStdString(val));
			if (match.hasMatch()) {
				matches = true;
				break;
			}
		}
		if (matches)
			(*filteredNodes)[nodeId] = true;
	}

	updateNodeIds();

	emit endResetModel();
	return true;
}

Qt::ItemFlags DataModel::flags(QModelIndex const& index) const {
	std::string propertyName = getPropertyNameFromColumnIndex(index.column());

	if (isSequenceColumn(propertyName))
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	try {
		DocumentPropertyDefinition propertyDefinition = this->document->getPropertyDefinition(propertyName);
		switch (propertyDefinition.type) {
		case DocumentPropertyType::DPT_LONGTEXT:
		case DocumentPropertyType::DPT_TEXTARRAY:
			return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
		}
		return Qt::ItemIsSelectable  | Qt::ItemIsEnabled |  Qt::ItemIsEditable;
	} catch (DocumentException const& e) {
		return Qt::ItemIsSelectable;
	}
}

std::string DataModel::formatHeader(std::string header) {
	header[0] = ::toupper(header[0]);
	return header;
}

int DataModel::getColumnIndexFromPropertyName(std::string propertyName) const {
	propertyName = normalizePropertyName(propertyName);
	auto f = this->propertyNamesMapRev.find(propertyName);
	if (f == this->propertyNamesMapRev.end())
		return -1;
	return f->second;
}

Document* DataModel::getDocument() {
	return this->document;
}

QModelIndex DataModel::getModelIndex(int row, int col) {
	return createIndex(row, col);
}

DocumentNodeId DataModel::getNodeIdFromRowIndex(int rowIndex) const {
	auto f = this->nodeIdsMap.find(rowIndex);
	if (f == this->nodeIdsMap.end())
		return InvalidDocumentNodeId;
	return f->second;
}

std::string DataModel::getPropertyNameFromColumnIndex(int colIndex) const {
	auto f = this->propertyNamesMap.find(colIndex);
	if (f == this->propertyNamesMap.end())
		return std::string();
	return f->second;
}

int DataModel::getRowIndexFromNodeId(DocumentNodeId nodeId) const {
	auto f = this->nodeIdsMapRev.find(nodeId);
	if (f == this->nodeIdsMapRev.end())
		return -1;
	return f->second;
}

QVariant DataModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Orientation::Vertical) {
		return QVariant::Invalid;
	} else {
		if (role == Qt::DisplayRole)
			return QString::fromStdString(formatHeader(getPropertyNameFromColumnIndex(section)));
		else if (role == Qt::TextAlignmentRole)
			return Qt::AlignLeft;
	}
	return QVariant::Invalid;
}

DocumentPropertyType DataModel::headerType(int section) const {
	std::string propertyName = getPropertyNameFromColumnIndex(section);
	try {
		DocumentPropertyDefinition propertyDefinition = this->document->getPropertyDefinition(propertyName);
		return propertyDefinition.type;
	} catch (DocumentException const& e) {
		return DocumentPropertyType::DPT_TEXT;
	}
}

DocumentNodeId DataModel::insertData(SafeDocumentNode const* node) {
	DocumentNodeId nodeId = this->document->addNode();

	int maxSeq = -1;
	for (DocumentNodeId id: this->document->getNodeIds()) {
		std::string p = this->document->getProperty(id, this->seqPropertyName);
		int seq = static_cast<int>(strtoll(p.c_str(), nullptr, 10));
		if (seq > maxSeq)
			maxSeq = seq;
	}

	this->document->setProperty(nodeId, this->seqPropertyName, std::to_string(maxSeq + 1));
	updateNodeIds();

	int row = getRowIndexFromNodeId(nodeId);
	int col = getColumnIndexFromPropertyName(this->seqPropertyName);
	emit dataChanged(createIndex(row, col), createIndex(row, col));

	updateData(nodeId, node);

	sort(-1);
	reset();

	return nodeId;
}

bool DataModel::isColumnHidden(int column) const {
	return this->document->getPropertyDefinition(getPropertyNameFromColumnIndex(column)).isHidden;
}

bool DataModel::isSequenceColumn(int columnIndex) const {
	std::string name = getPropertyNameFromColumnIndex(columnIndex);
	return isSequenceColumn(name);
}

bool DataModel::isSequenceColumn(std::string name) const {
	if (this->seqPropertyName.empty())
		return false;
	name = normalizePropertyName(name);
	return (name.compare(this->seqPropertyName) == 0);
}

std::vector<DocumentNodeId> DataModel::moveData(std::vector<int> indexes, int direction) {
	sort(getColumnIndexFromPropertyName(this->seqPropertyName));

	int maxRow = -1;
	int minRow = rowCount();

	int maxSeq = 0;
	std::unordered_map<int, DocumentNodeId> map;
	std::unordered_map<DocumentNodeId, int> mapRev;
	for (DocumentNodeId const& id: this->document->getNodeIds()) {
		std::string p = this->document->getProperty(id, this->seqPropertyName);
		int pI = strtoll(p.c_str(), NULL, 10);
		map[pI] = id;
		mapRev[id] = pI;
		if (pI > maxSeq)
			maxSeq = pI;
	}

	int inc;
	if (direction > 0) {
		inc = 1;
	} else {
		inc = -1;
		maxSeq = 0;
	}

	std::vector<DocumentNodeId> nodeIndexes;
	for (int const& i:indexes) {
		nodeIndexes.push_back(getNodeIdFromRowIndex(i));
		if (minRow > i)
			minRow = i;
		if (maxRow < i)
			maxRow = i;
	}

	if (direction < 0) {
		minRow = minRow + direction;
		if (minRow < 0)
			minRow = 0;
	} else {
		maxRow = maxRow + direction;
		if (maxRow >= rowCount())
			maxRow = rowCount() - 1;
	}

	std::sort(nodeIndexes.data(), nodeIndexes.data() + nodeIndexes.size(), [&mapRev, &direction](DocumentNodeId const& i1, DocumentNodeId const& i2) {
		return direction < 0 ? (mapRev[i1] < mapRev[i2]) : (mapRev[i1] > mapRev[i2]);
	});

	for (DocumentNodeId const& nodeId: nodeIndexes) {
		int seq = mapRev[nodeId];
		int seqEnd = seq+direction;
		if ((direction > 0 && seqEnd > maxSeq) || (direction < 0 && seqEnd < maxSeq))
			seqEnd = maxSeq;
		for (int i = seq; i != seqEnd; i=i+inc) {
			auto it1 = map.find(i);
			if (it1 == map.end())
				continue;
			auto it2 = map.find(i + inc);
			if (it1 == map.end())
				continue;

			DocumentNodeId& n1 = it1->second;
			DocumentNodeId& n2 = it2->second;

			map[i] = n2;
			mapRev[n2] = i;
		}

		map[seqEnd] = nodeId;
		mapRev[nodeId] = seqEnd;
		maxSeq -= inc;
	}

	for (auto const& it: map) {
		std::string oldV = this->document->getProperty(it.second, this->seqPropertyName);
		std::string newV = std::to_string(it.first);
		this->document->setProperty(it.second, this->seqPropertyName, newV);
		if (!allNodesModified && oldV.compare(newV) != 0)
			modifiedNodeIds[it.second][this->seqPropertyName] = true;
	}

	int const colId = getColumnIndexFromPropertyName(this->seqPropertyName);

	emit dataChanged(createIndex(minRow, colId), createIndex(maxRow, colId));

	return nodeIndexes;
}

std::vector<DocumentNodeId> DataModel::moveDataBottom(std::vector<int> indexes) {
	return moveData(indexes, rowCount());
}

std::vector<DocumentNodeId> DataModel::moveDataTop(std::vector<int> indexes) {
	return moveData(indexes, -rowCount());
}

void DataModel::reset() {
	beginResetModel();
	endResetModel();
}

int DataModel::rowCount(QModelIndex const& parent) const {
	if (this->filteredNodes != NULL)
		return this->filteredNodes->size();
	return this->document->getNodesCount();
}

void DataModel::saved() {
	beginResetModel();

	modifiedNodeIds.clear();
	allNodesModified = false;

	endResetModel();
}

void DataModel::setColumnHidden(int column, bool isHidden) {
	DocumentPropertyDefinition d = this->document->getPropertyDefinition(getPropertyNameFromColumnIndex(column));
	if (isHidden == d.isHidden)
		return;
	this->document->updatePropertyDefinition(d.name, NULL, NULL, NULL, NULL, &isHidden);
	emit dataChanged(createIndex(0, column), createIndex(rowCount()-1, column));
	allNodesModified = true;
}

bool DataModel::setData(QModelIndex const& index, QVariant const& value, int role) {
	if (index.isValid() && role == Qt::EditRole) {
		DocumentNodeId nodeId = getNodeIdFromRowIndex(index.row());
		std::string propertyName = getPropertyNameFromColumnIndex(index.column());

		if (isSequenceColumn(propertyName))
			return false;

		std::string newValue = value.toString().toStdString();
		std::string oldValue;

		try {
			oldValue = this->document->getProperty(nodeId, propertyName);
		} catch (DocumentException const& e) {
			//
		}

		if (oldValue.compare(newValue) == 0) {
			secureErase(newValue);
			return false;
		}

		try {
			this->document->setProperty(nodeId, propertyName, newValue);
		} catch (DocumentException const& e) {
			secureErase(newValue);
			return false;
		}
		secureErase(newValue);

		emit dataChanged(index, index);
		modifiedNodeIds[nodeId][propertyName] = true;
		return true;
	}
	return false;
}

bool DataModel::setHeaderData(int section, Qt::Orientation orientation, QVariant const& value, int role) {
	return false;
}

void DataModel::sort(int column, Qt::SortOrder order) {
	emit beforeSort();
	emit layoutAboutToBeChanged();

	if (column >= 0) {
		std::string propertyName = getPropertyNameFromColumnIndex(column);

		int i = 0;
		while (i < this->sortFields.size()) {
			if (this->sortFields[i].compare(propertyName) == 0) {
				this->sortFields.erase(this->sortFields.begin() + i);
				this->sortOrders.erase(this->sortOrders.begin() + i);
			}
			i++;
		}

		this->sortFields.insert(this->sortFields.begin(), propertyName);
		this->sortOrders.insert(this->sortOrders.begin(), order == Qt::SortOrder::AscendingOrder ? DocumentSortType::DST_ASC : DocumentSortType::DST_DSC);
	}

	this->document->sort(this->sortFields.data(), this->sortFields.size(), this->sortOrders.data(), this->sortOrders.size());

	updateNodeIds();

	emit layoutChanged();
	emit afterSort();
}

void DataModel::updateData(DocumentNodeId nodeId, SafeDocumentNode const* node) {
	int row = getRowIndexFromNodeId(nodeId);
	for (auto it: node->propertyValues) {
		std::string const& propertyName = it.first;
		if (isSequenceColumn(propertyName))
			continue;
		std::string oldV = this->document->getProperty(nodeId, propertyName);
		std::string& newV = it.second;

		if (oldV.compare(newV) == 0) {
			if (newV.empty())
				this->document->setProperty(nodeId, propertyName, newV);
			continue;
		}

		this->document->setProperty(nodeId, propertyName, newV);
		modifiedNodeIds[nodeId][propertyName] = true;
		int col = getColumnIndexFromPropertyName(propertyName);
		emit dataChanged(createIndex(row, col), createIndex(row, col));
	}
}
