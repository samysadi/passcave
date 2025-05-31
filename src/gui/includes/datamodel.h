#pragma once

#include "document.h"

#include <QAbstractTableModel>
#include <QRegularExpression>

#include <string>

using namespace passcave;

class DataModel: public QAbstractTableModel {
	Q_OBJECT
private:
	Document* document;
	std::string seqPropertyName;
	std::unordered_map<int, std::string> propertyNamesMap;
	std::unordered_map<std::string, int> propertyNamesMapRev;
	std::unordered_map<int, DocumentNodeId> nodeIdsMap;
	std::unordered_map<DocumentNodeId, int> nodeIdsMapRev;
	std::unordered_map<DocumentNodeId, std::unordered_map<std::string, bool>> modifiedNodeIds;
	bool allNodesModified = false;
	std::vector<std::string> sortFields;
	std::vector<DocumentSortType> sortOrders;
	std::unordered_map<DocumentNodeId, bool>* filteredNodes;
	void updateNodeIds();
public:
	DataModel(Document* document, std::string seqPropertyName = DefaultDocumentPropertyDefinitions::SEQUENCE.name);
	~DataModel();
    bool addMissingDocumentProperties();
	int columnCount(QModelIndex const& parent = QModelIndex()) const;
	QVariant data(QModelIndex const& index, int role = Qt::DisplayRole) const;
	void deleteIndexes(std::vector<int> indexes);
	bool filterData(QString pattern, QString column = "", bool caseSensitive = true);
	bool filterData(QString pattern, QStringList columns, bool caseSensitive = true);
	Qt::ItemFlags flags(QModelIndex const& index) const;
	static std::string formatHeader(std::string header);
	int getColumnIndexFromPropertyName(std::string propertyName) const;
	Document* getDocument();
	QModelIndex getModelIndex(int row, int col);
	DocumentNodeId getNodeIdFromRowIndex(int rowIndex) const;
	std::string getPropertyNameFromColumnIndex(int colIndex) const;
	int getRowIndexFromNodeId(DocumentNodeId nodeId) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	DocumentPropertyType headerType(int section) const;
	DocumentNodeId insertData(SafeDocumentNode const* node);
	bool isColumnHidden(int column) const;
	bool isSequenceColumn(int columnIndex) const;
	bool isSequenceColumn(std::string name) const;
	std::vector<DocumentNodeId> moveData(std::vector<int> indexes, int direction);
	std::vector<DocumentNodeId> moveDataBottom(std::vector<int> indexes);
	std::vector<DocumentNodeId> moveDataTop(std::vector<int> indexes);
	void reset();
	int rowCount(QModelIndex const& parent = QModelIndex()) const;
	void saved();
	void setColumnHidden(int column, bool isHidden);
	bool setData(QModelIndex const& index, QVariant const& value, int role = Qt::EditRole);
	bool setHeaderData(int section, Qt::Orientation orientation, QVariant const& value, int role = Qt::EditRole);
	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
	void updateData(DocumentNodeId nodeId, SafeDocumentNode const* node);
signals:
	void beforeSort();
	void afterSort();
};
