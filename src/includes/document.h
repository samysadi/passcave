#pragma once

#include "exceptions.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace passcave {
	enum class DocumentSortType {
		DST_ASC = 0,
		DST_DSC
	};

	enum class DocumentPropertyType {
		DPT_BOOL = 0,
		DPT_NUMBER,
		DPT_TIME,
		DPT_DATE,
		DPT_DATETIME,
		DPT_TEXT,
		DPT_TEXTARRAY,
		DPT_LONGTEXT,
		DPT_PASSWORD,
        DPT_URI,
        DPT_OTPAUTH
	};

	std::string documentPropertyTypeToString(DocumentPropertyType type);

	DocumentPropertyType documentPropertyTypeFromString(std::string s);

	std::string validateDocumentPropertyValue(DocumentPropertyType type, std::string const& value);

	std::string normalizePropertyName(std::string propertyName);

	struct DocumentPropertyDefinition {
		std::string name;
		std::string description;
		DocumentPropertyType type;
		int order;
		bool isHidden;
	};

	namespace DefaultDocumentPropertyDefinitions {
		DocumentPropertyDefinition const SEQUENCE{"#", "Sequence number.", DocumentPropertyType::DPT_NUMBER, 0, false};
		DocumentPropertyDefinition const LOGIN{"login", "Authentication username.", DocumentPropertyType::DPT_TEXT, 10, false};
		DocumentPropertyDefinition const PASSWORD{"password", "Authentication password.", DocumentPropertyType::DPT_PASSWORD, 20, false};
		DocumentPropertyDefinition const PROVIDER{"provider", "URL where login and password are applicable.", DocumentPropertyType::DPT_URI, 30, false};
        DocumentPropertyDefinition const OTPAUTH{"otpauth", "Otp auth.", DocumentPropertyType::DPT_OTPAUTH, 35, false};
		DocumentPropertyDefinition const TAGS{"tags", "A set of tags describing each node.", DocumentPropertyType::DPT_TEXTARRAY, 40, false};
        DocumentPropertyDefinition const CREATIONDATE{"creation date", "Creation date.", DocumentPropertyType::DPT_DATE, 50, false};
        DocumentPropertyDefinition const DETAILS{"details", "Extra information.", DocumentPropertyType::DPT_LONGTEXT, 60, false};
	}

	typedef std::unordered_map<std::string, DocumentPropertyDefinition> DocumentPropertyDefinitions;

	struct DocumentProperty {
		DocumentPropertyDefinition* propertyDefinition;
		std::string value;
	};

	typedef int DocumentNodeId;
	DocumentNodeId const InvalidDocumentNodeId = -1;

	struct DocumentNode {
		DocumentNodeId nodeId;
		std::vector<DocumentProperty> properties;
	};

	struct SafeDocumentNode {
		DocumentNodeId nodeId;
		std::unordered_map<std::string, std::string> propertyValues;
	};

	typedef std::unordered_map<DocumentNodeId, DocumentNode> DocumentNodes;

	class Document;

	class DocumentException;
	class DocumentBadDataException;
	class DocumentPropertyDefinitionNotFoundException;
	class DocumentPropertyDefinitionAlreadyExistsException;
	class DocumentNodeNotFoundException;
	class BadDocumentPropertyValueException;
}

using namespace passcave;

class passcave::Document {
private:
	DocumentPropertyDefinitions* propertyDefinitions;
	DocumentNodes* nodes;
	std::vector<DocumentNodeId>* sortedNodes;
	DocumentNodeId maxNodeId;
	void clearSortedNodes();
	int comparePropertyValues(std::string const& value1, std::string const& value2, DocumentPropertyType propType, DocumentSortType sortType);
	static std::vector<char> readFileData(std::string xmlFilename);
	void updatePropertyDefinition(std::string name, std::string newName, std::string* newDescription, DocumentPropertyType* newType);
public:
	Document();
	Document(std::string xmlFilename);
	Document(std::vector<char> const& xmlData, bool ignorePropertyValueValidation = false);
	~Document();

	static bool convertBool(std::string const& v);
	static long long convertNumber(std::string const& v);
	static struct tm convertTime(std::string const& v);
	static struct tm convertDate(std::string const& v);
	static struct tm convertDateTime(std::string const& v);
	static std::string convertText(std::string const& v);
	static std::vector<std::string> convertTextArray(std::string const& v);
	static std::string convertLongText(std::string const& v);
	static std::string convertPassword(std::string const& v);
	static std::string convertURI(std::string const& v);

	/**
	 * @brief adds a property definition
	 * @param name
	 * @param description
	 * @param type
     * @return true if the property was added, false otherwise (if the property already exists)
	 */
    bool addPropertyDefinition(std::string name, std::string description, DocumentPropertyType type = DocumentPropertyType::DPT_TEXT, int order = 0, bool isHidden = false);

    bool addPropertyDefinition(DocumentPropertyDefinition const& propertyDefinition);

    bool addDefaultPropertyDefinitions();

	void updatePropertyDefinition(std::string name, std::string* newName, std::string* newDescription, DocumentPropertyType* newType,
								  int* newOrder, bool* newIsHidden);

	void removePropertyDefinition(std::string name);

	DocumentPropertyDefinition getPropertyDefinition(std::string name) const;
	std::vector<DocumentPropertyDefinition> getPropertyDefinitions() const;
	int getPropertyDefinitionsCount() const;

	/**
	 * @brief addNode
	 * @return the nodeId corresponding to the added node
	 */
	int addNode();

	SafeDocumentNode getDocumentNode(DocumentNodeId nodeId) const;
	SafeDocumentNode getEmptyDocumentNode() const;

	bool hasNode(DocumentNodeId nodeId) const;

	void removeNode(DocumentNodeId nodeId);

	std::vector<DocumentNodeId> getNodeIds() const;

	int getNodesCount() const;

	std::string getProperty(DocumentNodeId nodeId, std::string propertyName) const;

	/**
	 * Sets or replaces the property name / value for the given node.
	 * This might throw a {@link BadDocumentPropertyValueException} if the value is
	 * not of the right type.
	 * Also a {@link DocumentNodeNotFoundException} might be thrown if the nodeId is incorrect,
	 * and a {@link DocumentPropertyDefinitionNotFoundException} might be thrown if the propertyName does not match any
	 * property definition.
	 *
	 * @param nodeId
	 * @param propertyName
	 * @param propertyValue
	 */
	void setProperty(DocumentNodeId nodeId, std::string propertyName, std::string const& propertyValue);

	void setProperty(DocumentNodeId nodeId, DocumentPropertyDefinition const& propertyDefinition, std::string const& propertyValue);

	/**
	 * @brief clears everything (propertyDefinitions + nodes)
	 */
	void clear();

	/**
	 * @brief clears nodes
	 */
	void clearNodes();

	/**
	 * @brief sort nodes using the given sortFields (aka. property names) and the given sortTypes.
	 * @param sortFields array of property names to use for sorting
	 * @param numFields number of elements in the sortFields array
	 * @param sortTypes optional array of sort types, if not given then the default is to use ascending sorting
	 * @param numTypes number of elements in the sortTypes array, if it does not match numFields then the default is to use ascending sorting
	 */
	void sort(std::string sortFields[], int numFields, DocumentSortType sortTypes[] = {}, int numTypes = 0);

	void sort(std::string sortField, DocumentSortType sortType = DocumentSortType::DST_ASC);

	std::vector<char> buildXml() const;
	std::string buildXmlString() const;
};


class passcave::DocumentException: public std::runtime_error {
public:
	DocumentException(std::string const& __arg): std::runtime_error(__arg) {}
};

class passcave::DocumentBadDataException: public DocumentException {
public:
	DocumentBadDataException(std::string const& __arg): DocumentException(__arg) {}
};

class passcave::DocumentPropertyDefinitionNotFoundException: public DocumentException {
public:
	DocumentPropertyDefinitionNotFoundException(std::string const& property_name): DocumentException("Given property name: " + property_name + " does not match any existing property definition.") {}
};

class passcave::DocumentPropertyDefinitionAlreadyExistsException: public DocumentException {
public:
	DocumentPropertyDefinitionAlreadyExistsException(std::string const& property_name): DocumentException("Given property name: " + property_name + " already matches an existing property definition.") {}
};

class passcave::DocumentNodeNotFoundException: public DocumentException {
public:
	DocumentNodeNotFoundException(DocumentNodeId nodeId): DocumentException("There is no node matching the given node id: " + std::to_string(nodeId) + ".") {}
};

class passcave::BadDocumentPropertyValueException: public DocumentException {
public:
	BadDocumentPropertyValueException(): BadDocumentPropertyValueException("") {}
	BadDocumentPropertyValueException(std::string const& __arg): DocumentException(__arg) {}
};
