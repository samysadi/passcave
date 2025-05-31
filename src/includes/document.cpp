#include "document.h"
#include "utils.h"
#include "config.h"

#include <QString>
#include <QFile>
#include <QByteArray>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>
#include <fstream>
#include <ctime>

using namespace passcave;

inline QString myStdStringToQString(std::string const& s) {
	return QString::fromStdString(s);
}

std::string passcave::documentPropertyTypeToString(DocumentPropertyType type) {
	switch (type) {
	case DocumentPropertyType::DPT_BOOL:
		return "bool";
	case DocumentPropertyType::DPT_NUMBER:
		return "number";
	case DocumentPropertyType::DPT_TIME:
		return "time";
	case DocumentPropertyType::DPT_DATE:
		return "date";
	case DocumentPropertyType::DPT_DATETIME:
		return "datetime";
	case DocumentPropertyType::DPT_TEXT:
		return "text";
	case DocumentPropertyType::DPT_TEXTARRAY:
		return "textarray";
	case DocumentPropertyType::DPT_LONGTEXT:
        return "longtext";
    case DocumentPropertyType::DPT_PASSWORD:
        return "password";
    case DocumentPropertyType::DPT_OTPAUTH:
        return "otpauth";
	case DocumentPropertyType::DPT_URI:
		return "uri";
	default:
		return documentPropertyTypeToString(DocumentPropertyType::DPT_TEXT);
	}
}

DocumentPropertyType passcave::documentPropertyTypeFromString(std::string s) {
	s = simple_lowercase(s);
	if (s.compare("bool") == 0)
		return DocumentPropertyType::DPT_BOOL;
	if (s.compare("number") == 0)
		return DocumentPropertyType::DPT_NUMBER;
	if (s.compare("time") == 0)
		return DocumentPropertyType::DPT_TIME;
	if (s.compare("date") == 0)
		return DocumentPropertyType::DPT_DATE;
	if (s.compare("datetime") == 0)
		return DocumentPropertyType::DPT_DATETIME;
	if (s.compare("text") == 0)
		return DocumentPropertyType::DPT_TEXT;
	if (s.compare("textarray") == 0)
		return DocumentPropertyType::DPT_TEXTARRAY;
	if (s.compare("longtext") == 0)
        return DocumentPropertyType::DPT_LONGTEXT;
    if (s.compare("password") == 0)
        return DocumentPropertyType::DPT_PASSWORD;
    if (s.compare("otpauth") == 0)
        return DocumentPropertyType::DPT_OTPAUTH;
	if (s.compare("uri") == 0)
		return DocumentPropertyType::DPT_URI;
	return DocumentPropertyType::DPT_TEXT;
}

std::string passcave::validateDocumentPropertyValue(DocumentPropertyType type, std::string const& value) {
	switch (type) {
	case DocumentPropertyType::DPT_BOOL:
		return Document::convertBool(value) ? "true" : "false";
	case DocumentPropertyType::DPT_NUMBER: {
		long long l = Document::convertNumber(value);
		std::string v = std::to_string(l);
		secureErase(reinterpret_cast<char*>(&l), sizeof(l));
		return v;
	} case DocumentPropertyType::DPT_TIME: {
		if (value.empty())
			return currentTime();
		struct tm t = Document::convertTime(value);
		return timeToStr(t);
	} case DocumentPropertyType::DPT_DATE: {
		if (value.empty())
			return currentDate();
		struct tm t = Document::convertDate(value);
		return dateToStr(t);
	}
	case DocumentPropertyType::DPT_DATETIME: {
		if (value.empty())
			return currentDateTime();
		struct tm t = Document::convertDateTime(value);
		return dateTimeToStr(t);
    } case DocumentPropertyType::DPT_TEXT:
    case DocumentPropertyType::DPT_PASSWORD:
    case DocumentPropertyType::DPT_OTPAUTH:
	case DocumentPropertyType::DPT_URI: {
		return Document::convertText(value);
	} case DocumentPropertyType::DPT_TEXTARRAY: {
		std::vector<std::string> arr = Document::convertTextArray(value);

		if (arr.empty())
			return "";

		std::vector<char> ss;
		for (std::string& s: arr) {
			ss.insert(ss.end(), s.c_str(), s.c_str() + s.length());
			ss.push_back('\n');
			secureErase(s);
		}
		if (ss.size() != 0)
			ss.pop_back();
		std::string v(ss.data(), ss.data() + ss.size());
		secureErase(ss);
		return v;
	} case DocumentPropertyType::DPT_LONGTEXT:
		return Document::convertLongText(value);
	default:
		return Document::convertText(value);
	}
}

bool Document::convertBool(std::string const& v) {
	std::string t = simple_trim(v);
	std::string v0 = simple_lowercase(t); secureErase(t);
	if (v0 == "on" || v0 == "yes" || v0 == "true")
		return true;
	if (v0 == "off" || v0 == "no" || v0 == "false")
		return false;
	bool r = convertNumber(v0) != 0;
	secureErase(v0);
	return r;
}

long long Document::convertNumber(std::string const& v) {
	int i = 0;
	while (i < v.length() && (v[i] == '0' || ::isspace(v[i])))
		i++;
	if (i == v.length())
		return 0;
	char const* p = v.c_str() + i;
	if (v[i] == '-') {
		i++;
		if (i == v.length())
			return 0;
	}
	bool expecting_spaces = false;
	while (i < v.length()) {
		if (::isspace(v[i])) {
			expecting_spaces = true;
		} else if (expecting_spaces || !::isdigit(v[i]))
			throw BadDocumentPropertyValueException("Bad Number or Bool Property value.");
		i++;
	}
	return std::strtoll(p, NULL, 10);
}

struct tm Document::convertTime(std::string const& v) {
	char* p;
	struct tm t;
	t.tm_hour = strtoll(v.c_str(), &p, 10);
	t.tm_min = strtoll(p+1, &p, 10);
	t.tm_sec = strtoll(p+1, &p, 10);
	t.tm_year = -1;
	t.tm_mon = -1;
	t.tm_mday = -1;
	t.tm_wday = -1;
	t.tm_yday = -1;
	t.tm_isdst = -1;
	return t;
}

struct tm Document::convertDate(std::string const& v) {
	char* p;
	struct tm t;
	t.tm_hour = -1;
	t.tm_min = -1;
	t.tm_sec = -1;
	t.tm_year = strtoll(v.c_str(), &p, 10);
	if (t.tm_year != 0)
		t.tm_year -= 1900;
	t.tm_mon = strtoll(p+1, &p, 10);
	if (t.tm_mon != 0)
		t.tm_mon -= 1;
	t.tm_mday = strtoll(p+1, &p, 10);
	t.tm_wday = -1;
	t.tm_yday = -1;
	t.tm_isdst = -1;
	return t;
}

struct tm Document::convertDateTime(std::string const& v) {
	char* p;
	struct tm t;
	t.tm_hour = strtoll(v.c_str(), &p, 10);
	t.tm_min = strtoll(p+1, &p, 10);
	t.tm_sec = strtoll(p+1, &p, 10);
	t.tm_year = strtoll(p+1, &p, 10);
	if (t.tm_year != 0)
		t.tm_year -= 1900;
	t.tm_mon = strtoll(p+1, &p, 10);
	if (t.tm_mon != 0)
		t.tm_mon -= 1;
	t.tm_mday = strtoll(p+1, &p, 10);
	t.tm_wday = -1;
	t.tm_yday = -1;
	t.tm_isdst = -1;
	return t;
}

std::string Document::convertText(std::string const& v) {
	for (char const& c: v)
		if (c == '\n' || c ==  '\r')
			throw BadDocumentPropertyValueException("Text, Password and URI Property values cannot contain line breaks.");
	return v;
}

std::vector<std::string> Document::convertTextArray(std::string const& v) {
	int i = 0;
	int j = 0;
	std::vector<std::string> arr;
	while (j<v.length()) {
		char const& c = v[j];
		if (c == '\n' || c ==  '\r') {
			int d = j - i;
			if (d > 0)
				arr.push_back(v.substr(i, d));
			i = j + 1;
		}
		j++;
	}

	{
		int d = j - i;
		if (d > 0)
			arr.push_back(v.substr(i, d));
	}

	if (arr.size() == 0)
		return arr;

	std::sort(arr.begin(), arr.end(), [](std::string const& s1, std::string const& s2) {
		return simple_compareStrings(s1, s2) <= 0;
	});

	return arr;
}

std::string Document::convertLongText(std::string const& v) {
	return v;
}

std::string Document::convertPassword(std::string const& v) {
	return convertText(v);
}

std::string Document::convertURI(std::string const& v) {
	return convertText(v);
}

std::vector<char> Document::readFileData(std::string xmlFilename) {
	std::vector<char> inData;
	std::ifstream inFile(xmlFilename, std::ios::binary | std::ios::ate);
	if (inFile) {
		std::ifstream::pos_type size = inFile.tellg();
		inData.resize(size);
		inFile.seekg(0, std::ios::beg);
		if (!inFile.read(inData.data(), size))
			inData.clear();
	}
	if (inData.empty())
		throw IOException("Nothing read from file: " + xmlFilename);
	return inData;
}

Document::Document(std::string xmlFilename): Document(readFileData(xmlFilename)) {
	//
}

Document::Document(std::vector<char> const& xmlData, bool ignorePropertyValueValidation): Document() {
	QByteArray in(xmlData.data(), xmlData.size());

	bool parseError = false;
	char* p;
	int ignore = 0;
	{
		QXmlStreamReader x(in);

		enum class MyNodeType {
			None,
			Root,
			PropertyDefinitions,
			PropertyDefinition,
			Nodes,
			Node,
			Properties,
			Property
		};

		MyNodeType t = MyNodeType::None;

		DocumentPropertyDefinition currentPropertyDefinition;
		DocumentNodeId currentNodeId;
		std::string currentPropertyName;
		std::string currentPropertyValue;

		try {
			while (!x.atEnd()) {
				x.readNext();
				if (x.isStartElement()) {
					if (ignore > 0)
						ignore++;
					else {
						QString name = x.name().toString().toLower();
						switch (t) {
						case MyNodeType::None:
							if (name.compare("root") == 0)
								t = MyNodeType::Root;
							else
								ignore++;
							break;
						case MyNodeType::Root:
							if (name.compare("property-definitions") == 0)
								t = MyNodeType::PropertyDefinitions;
							else if (name.compare("nodes") == 0)
								t = MyNodeType::Nodes;
							else
								ignore++;
							break;
						case MyNodeType::PropertyDefinitions:
							if (name.compare("property-definition") == 0) {
								t = MyNodeType::PropertyDefinition;
								currentPropertyDefinition.name = "";
								currentPropertyDefinition.type = DocumentPropertyType::DPT_TEXT;
								currentPropertyDefinition.description = "";
								currentPropertyDefinition.order = 0;
								currentPropertyDefinition.isHidden = false;
							} else
								ignore++;
							break;
						case MyNodeType::PropertyDefinition:
							if (name.compare("name") == 0) {
								currentPropertyDefinition.name = x.readElementText().trimmed().toStdString();
							} else if (name.compare("description") == 0) {
								currentPropertyDefinition.description = x.readElementText().trimmed().toStdString();
							} else if (name.compare("type") == 0) {
								currentPropertyDefinition.type = documentPropertyTypeFromString(x.readElementText().trimmed().toStdString());
							} else if (name.compare("order") == 0) {
								currentPropertyDefinition.order = static_cast<int>(std::strtol(x.readElementText().trimmed().toStdString().c_str(), NULL, 10));
							} else if (name.compare("ishidden") == 0) {
								currentPropertyDefinition.isHidden = static_cast<int>(std::strtol(x.readElementText().trimmed().toStdString().c_str(), NULL, 10)) != 0;
							} else
								ignore++;
							break;
						case MyNodeType::Nodes:
							if (name.compare("node") == 0) {
								t = MyNodeType::Node;
								currentNodeId = this->addNode();
							} else
								ignore++;
							break;
						case MyNodeType::Node:
							if (name.compare("properties") == 0)
								t = MyNodeType::Properties;
							else
								ignore++;
							break;
						case MyNodeType::Properties:
							if (name.compare("property") == 0) {
								t = MyNodeType::Property;
								currentPropertyName = "";
								currentPropertyValue = "";
							} else
								ignore++;
							break;
						case MyNodeType::Property:
							if (name.compare("name") == 0) {
								currentPropertyName = x.readElementText().trimmed().toStdString();
							} else if (name.compare("value") == 0) {
								secureErase(currentPropertyValue);
								QString v = x.readElementText();
								currentPropertyValue = v.toStdString();
								secureErase(v);
							} else
								ignore++;
							break;
						default:
							ignore++;
						}
					}
				} else if (x.isEndElement()) {
					if (ignore > 0)
						ignore--;
					else {
						QString name = x.name().toString().toLower();
						switch (t) {
                        case MyNodeType::None:
						case MyNodeType::Root:
							t = MyNodeType::None;
							break;
						case MyNodeType::PropertyDefinitions:
							t = MyNodeType::Root;
							break;
						case MyNodeType::PropertyDefinition:
							t = MyNodeType::PropertyDefinitions;
							this->addPropertyDefinition(currentPropertyDefinition);
							break;
						case MyNodeType::Nodes:
							t = MyNodeType::Root;
							break;
						case MyNodeType::Node:
							t = MyNodeType::Nodes;
							break;
						case MyNodeType::Properties:
							t = MyNodeType::Node;
							break;
						case MyNodeType::Property:
							t = MyNodeType::Properties;
							if (ignorePropertyValueValidation) {
								try {
									this->setProperty(currentNodeId, currentPropertyName, "");
								} catch (BadDocumentPropertyValueException const& e) {
									//ignore
								}
							} else
								this->setProperty(currentNodeId, currentPropertyName, currentPropertyValue);
							secureErase(currentPropertyValue);
							currentPropertyValue = "";
							break;
						}
					}
				}
			}
		} catch (BadDocumentPropertyValueException const& e) {
			secureErase(currentPropertyValue);
			secureErase(p, sizeof(QXmlStreamReader));
			secureErase(in.begin(), in.end() - in.begin());
			throw DocumentBadDataException("Error while parsing document. Some property values do not match their property type.");
		}

		secureErase(currentPropertyValue);

		parseError = parseError || x.hasError();
		p = reinterpret_cast<char*>(&x);
	}

	secureErase(p, sizeof(QXmlStreamReader));
	secureErase(in.begin(), in.end() - in.begin());

	if (parseError)
		throw DocumentBadDataException("Error while parsing document. It might be corrupt.");
}

std::vector<char> Document::buildXml() const {
	QByteArray out;
	QXmlStreamWriter sw(&out);
	sw.setAutoFormatting(true);
	sw.setAutoFormattingIndent(-1); //one tab

	sw.writeStartDocument();

	sw.writeStartElement("root");
	sw.writeAttribute("version", QString(myStdStringToQString(std::to_string(passcave_VERSION_MAJOR) + "." + std::to_string(passcave_VERSION_MINOR))));

	sw.writeStartElement("property-definitions");
	for (auto const& it: *this->propertyDefinitions) {
		DocumentPropertyDefinition const& propDef = it.second;
		sw.writeStartElement("property-definition");

		sw.writeStartElement("name");
		sw.writeCharacters(myStdStringToQString(propDef.name));
		sw.writeEndElement();

		sw.writeStartElement("type");
		sw.writeCharacters(myStdStringToQString(documentPropertyTypeToString(propDef.type)));
		sw.writeEndElement();

		sw.writeStartElement("description");
		sw.writeCharacters(myStdStringToQString(propDef.description));
		sw.writeEndElement();

		sw.writeStartElement("order");
		sw.writeCharacters(QString::number(propDef.order));
		sw.writeEndElement();

		sw.writeStartElement("ishidden");
		sw.writeCharacters(propDef.isHidden ? "1" : "0");
		sw.writeEndElement();

		sw.writeEndElement();
	}
	sw.writeEndElement(); // </property-definitions>

	sw.writeStartElement("nodes");
	for (DocumentNodeId id: this->getNodeIds()) {
		auto it = this->nodes->find(id);
		if (it == this->nodes->end())
			continue;
		DocumentNode const& node = it->second;
		sw.writeStartElement("node");

		sw.writeStartElement("properties");
		for (auto const& it2: node.properties) {
			DocumentProperty const& prop = it2;
			sw.writeStartElement("property");

			sw.writeStartElement("name");
			sw.writeCharacters(myStdStringToQString(prop.propertyDefinition->name));
			sw.writeEndElement();

			QString qs = myStdStringToQString(prop.value);
			sw.writeStartElement("value");
			sw.writeCharacters(qs);
			sw.writeEndElement();
			secureErase(qs);

			sw.writeEndElement();
		}
		sw.writeEndElement();

		sw.writeEndElement();
	}
	sw.writeEndElement(); // </nodes>

	sw.writeEndElement(); // </root>

	sw.writeEndDocument();

	std::vector<char> r(out.begin(), out.end());
	secureErase(out.begin(), out.end() - out.begin());
	return r;
}

std::string Document::buildXmlString() const {
	std::vector<char> v = buildXml();
	std::string s(v.data(), v.size());
	secureErase(v);
	return s;
}

Document::Document() {
	this->propertyDefinitions = new DocumentPropertyDefinitions();
	this->nodes = new DocumentNodes();
	this->maxNodeId = 0;
	this->sortedNodes = NULL;
}

Document::~Document() {
	clearNodes();

	delete this->nodes;
	this->nodes = NULL;
	delete this->propertyDefinitions;
	this->propertyDefinitions = NULL;
}

void Document::clearSortedNodes() {
	if (this->sortedNodes != NULL) {
		delete this->sortedNodes;
		this->sortedNodes = NULL;
	}
}

void Document::clear() {
	clearNodes();
	delete this->propertyDefinitions;
	this->propertyDefinitions = new DocumentPropertyDefinitions();
}

void Document::clearNodes() {
	clearSortedNodes();

	for (auto& nodeIt: *this->nodes)
		for (auto& propIt: nodeIt.second.properties)
			secureErase(propIt.value);

	delete this->nodes;
	this->nodes = new DocumentNodes();
}

std::string passcave::normalizePropertyName(std::string propertyName) {
	return simple_lowercase(propertyName);
}

bool Document::addPropertyDefinition(std::string name, std::string description, DocumentPropertyType type, int order, bool isHidden) {
	name = normalizePropertyName(name);

	// check for existence
	auto pDefIt = this->propertyDefinitions->find(name);
    if (pDefIt != this->propertyDefinitions->end()) {
        return false;
    }
    // insert
    DocumentPropertyDefinition pDef;
    pDef.name = name;
    pDef.description = description;
    pDef.type = type;
    pDef.order = order;
    pDef.isHidden = isHidden;

    this->propertyDefinitions->insert({name, pDef});
    return true;
}

bool Document::addPropertyDefinition(DocumentPropertyDefinition const& propertyDefinition) {
    return this->addPropertyDefinition(propertyDefinition.name, propertyDefinition.description, propertyDefinition.type, propertyDefinition.order, propertyDefinition.isHidden);
}

bool Document::addDefaultPropertyDefinitions() {
    bool r = false;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::SEQUENCE) | r;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::CREATIONDATE) | r;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::LOGIN) | r;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::PASSWORD) | r;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::PROVIDER) | r;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::OTPAUTH) | r;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::TAGS) | r;
    r = this->addPropertyDefinition(DefaultDocumentPropertyDefinitions::DETAILS) | r;
    return r;
}

void Document::updatePropertyDefinition(std::string name, std::string* newName, std::string* newDescription, DocumentPropertyType* newType,
										int* newOrder, bool* newIsHidden) {
	name = normalizePropertyName(name);

	auto pDefIt = this->propertyDefinitions->find(name);
	if (pDefIt == this->propertyDefinitions->end())
		throw DocumentPropertyDefinitionNotFoundException(name);

	// update
	if (newName != NULL)
		pDefIt->second.name = normalizePropertyName(*newName);
	if (newDescription != NULL)
		pDefIt->second.description = *newDescription;
	if (newType != NULL)
		pDefIt->second.type = *newType;
	if (newOrder != NULL)
		pDefIt->second.order = *newOrder;
	if (newOrder != NULL)
		pDefIt->second.isHidden = *newIsHidden;
}

void Document::removePropertyDefinition(std::string name) {
	name = normalizePropertyName(name);

	auto pDefIt = this->propertyDefinitions->find(name);
	if (pDefIt == this->propertyDefinitions->end())
		throw DocumentPropertyDefinitionNotFoundException(name);

	// remove Node Properties which matches
	for (auto& nodeIt : *this->nodes) {
		for (auto propIt = nodeIt.second.properties.begin(); propIt != nodeIt.second.properties.end(); ++propIt) {
			if (propIt->propertyDefinition == &pDefIt->second) {
				secureErase(propIt->value);
				nodeIt.second.properties.erase(propIt);
				break;
			}
		}
	}

	// remove def
	this->propertyDefinitions->erase(pDefIt);
}

DocumentPropertyDefinition Document::getPropertyDefinition(std::string name) const {
	name = normalizePropertyName(name);
	auto pDefIt = this->propertyDefinitions->find(name);
	if (pDefIt == this->propertyDefinitions->end())
		throw DocumentPropertyDefinitionNotFoundException(name);

	return pDefIt->second;
}

std::vector<DocumentPropertyDefinition> Document::getPropertyDefinitions() const {
	std::vector<DocumentPropertyDefinition> v;
	v.reserve(this->propertyDefinitions->size());

	for (auto const& it: *this->propertyDefinitions) {
		DocumentPropertyDefinition const& propDef = it.second;
		v.push_back(propDef);
	}

	std::sort(v.begin(), v.end(), [](DocumentPropertyDefinition const& p1, DocumentPropertyDefinition const& p2) {
		if (p1.order == p2.order)
			return &p1 < &p2; //just to make sure order is total
		return p1.order < p2.order;
	});

	return v;
}

int Document::getPropertyDefinitionsCount() const {
	return this->propertyDefinitions->size();
}

int Document::addNode() {
	DocumentNode node;
	node.nodeId = this->maxNodeId;
	this->nodes->insert({node.nodeId, node});
	this->maxNodeId++;

	if (this->sortedNodes != NULL)
		this->sortedNodes->push_back(node.nodeId);

	return node.nodeId;
}

SafeDocumentNode Document::getDocumentNode(DocumentNodeId nodeId) const {
	auto nodeIt = this->nodes->find(nodeId);
	if (nodeIt == this->nodes->end())
		throw DocumentNodeNotFoundException(nodeId);

	SafeDocumentNode r;
	r.nodeId = nodeId;
	for (DocumentProperty p: nodeIt->second.properties)
		r.propertyValues[p.propertyDefinition->name] = p.value;

	return r;
}

SafeDocumentNode Document::getEmptyDocumentNode() const {
	SafeDocumentNode r;
	r.nodeId = InvalidDocumentNodeId;
	for (auto it: *this->propertyDefinitions)
		r.propertyValues[it.second.name] = validateDocumentPropertyValue(it.second.type, "");

	return r;
}

bool Document::hasNode(DocumentNodeId nodeId) const {
	return (this->nodes->find(nodeId) != this->nodes->end());
}

void Document::removeNode(DocumentNodeId nodeId) {
	int r = this->nodes->erase(nodeId);
	if (r != 1)
		throw DocumentNodeNotFoundException(nodeId);

	if (this->sortedNodes != NULL)
		this->sortedNodes->erase(std::remove(this->sortedNodes->begin(), this->sortedNodes->end(), nodeId), this->sortedNodes->end());
}

std::vector<DocumentNodeId> Document::getNodeIds() const {
	if (this->sortedNodes != NULL) {
		return *this->sortedNodes;
	} else {
		std::vector<DocumentNodeId> v;
		for (auto const& it : *this->nodes)
			v.push_back(it.first);
		return v;
	}
}

int Document::getNodesCount() const {
	if (this->sortedNodes != NULL)
		return this->sortedNodes->size();
	else
		return this->nodes->size();
}

std::string Document::getProperty(DocumentNodeId nodeId, std::string propertyName) const {
	auto nodeIt = this->nodes->find(nodeId);
	if (nodeIt == this->nodes->end())
		throw DocumentNodeNotFoundException(nodeId);

	propertyName = normalizePropertyName(propertyName);

	for (auto const& documentProperty : nodeIt->second.properties) {
		if (documentProperty.propertyDefinition->name.compare(propertyName) == 0)
			return documentProperty.value;
	}

	return std::string();
}

void Document::setProperty(DocumentNodeId nodeId, std::string propertyName, std::string const& propertyValue) {
	auto nodeIt = this->nodes->find(nodeId);
	if (nodeIt == this->nodes->end())
		throw DocumentNodeNotFoundException(nodeId);

	propertyName = normalizePropertyName(propertyName);

	auto pDefIt = this->propertyDefinitions->find(propertyName);
	if (pDefIt == this->propertyDefinitions->end())
		throw DocumentPropertyDefinitionNotFoundException(propertyName);

	std::string temp = validateDocumentPropertyValue(pDefIt->second.type, propertyValue);

	for (auto& documentProperty : nodeIt->second.properties) {
		if (documentProperty.propertyDefinition->name.compare(propertyName) == 0) {
			documentProperty.value = temp;
			secureErase(temp);
			return;
		}
	}

	DocumentProperty prop;
	prop.propertyDefinition = &pDefIt->second;
	prop.value = temp;
	secureErase(temp);

	nodeIt->second.properties.push_back(prop);
}

void Document::setProperty(DocumentNodeId nodeId, DocumentPropertyDefinition const& propertyDefinition, std::string const& propertyValue) {
	this->setProperty(nodeId, propertyDefinition.name, propertyValue);
}

int Document::comparePropertyValues(std::string const& value1, std::string const& value2, DocumentPropertyType propType, DocumentSortType sortType) {
	int r = 0;
	switch (propType) {
	case DocumentPropertyType::DPT_TIME:
	case DocumentPropertyType::DPT_DATE:
	case DocumentPropertyType::DPT_DATETIME:
		r = simple_compareStringDates(value1, value2);
		break;
	case DocumentPropertyType::DPT_NUMBER:
		r = simple_compareStringNumbers(value1, value2);
		break;
	case DocumentPropertyType::DPT_URI:
		r = simple_compareStringURIs(value1, value2);
		break;
    case DocumentPropertyType::DPT_BOOL:
    case DocumentPropertyType::DPT_PASSWORD:
    case DocumentPropertyType::DPT_OTPAUTH:
	case DocumentPropertyType::DPT_TEXT:
	case DocumentPropertyType::DPT_TEXTARRAY:
	case DocumentPropertyType::DPT_LONGTEXT:
	default:
		r = simple_compareStrings(value1, value2);
	}

	if (r == 0)
		return 0;

	if (sortType == DocumentSortType::DST_ASC)
		return r;
	else
		return -r;
}

void Document::sort(std::string sortFields[], int numFields, DocumentSortType sortTypes[], int numTypes) {
	clearSortedNodes();
	{
		auto v = this->getNodeIds();
		this->sortedNodes = new std::vector<DocumentNodeId>();
		*this->sortedNodes = v;
	}

	if (numFields == 0)
		return; // no fields to sort on

	// transform sortFields to sortPropertyDefinitions
	std::vector<DocumentPropertyDefinition const*> sortPropertyDefinitions;
	for (int i = 0; i< numFields; i++) {
		sortFields[i] = normalizePropertyName(sortFields[i]);

		auto pDefIt = this->propertyDefinitions->find(sortFields[i]);
		if (pDefIt == this->propertyDefinitions->end())
			continue; //ignore it

		sortPropertyDefinitions.push_back(&pDefIt->second);
	}

	std::sort(this->sortedNodes->begin(), this->sortedNodes->end(), [this, &sortPropertyDefinitions, &sortTypes, numTypes](DocumentNodeId const& id1, DocumentNodeId const& id2) {
		int numFields = sortPropertyDefinitions.size();
		auto const& node1It = this->nodes->find(id1);
		auto const& node2It = this->nodes->find(id2);

		if (node1It == this->nodes->end() || node2It == this->nodes->end())
			return id1 < id2; // either node1 or node2 were not found, so use default ordering

		for (int i = 0; i< numFields; i++) {
			DocumentSortType sortType(DocumentSortType::DST_ASC);
			if (i < numTypes)
				sortType = sortTypes[i];
			DocumentPropertyDefinition const* sortPropertyDefinition = sortPropertyDefinitions[i];

			std::string const* propVal1 = NULL;
			for (auto const& documentProperty : node1It->second.properties) {
				if (documentProperty.propertyDefinition == sortPropertyDefinition) {
					propVal1 = &documentProperty.value;
					break;
				}
			}
			if (propVal1 == NULL)
				continue;

			std::string const* propVal2 = NULL;
			for (auto const& documentProperty : node2It->second.properties) {
				if (documentProperty.propertyDefinition == sortPropertyDefinition) {
					propVal2 = &documentProperty.value;
					break;
				}
			}
			if (propVal2 == NULL)
				continue;

			int c = this->comparePropertyValues(*propVal1, *propVal2, sortPropertyDefinition->type, sortType);
			if (c < 0)
				return true; // node1 is before node2
			else if (c > 0)
				return false; // node1 is after node2
		}

		return id1 < id2; // no sort order was set by given fields, so let's use default
	});
}

void Document::sort(std::string sortField, DocumentSortType sortType) {
	std::string a[] = {sortField};
	DocumentSortType b[] = {sortType};
	this->sort(a, 1, b, 1);
}
