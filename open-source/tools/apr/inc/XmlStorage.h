


#ifndef XMLSTORAGE_H
#define XMLSTORAGE_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pthread.h>
#include "Observable.h"
#include "Observer.h"

class XmlStorage: public Observer
{
public:
	XmlStorage(Observable *o, char* pdir, char* pfname);
	~XmlStorage();
	virtual void handleEvent();
	virtual void handleEvent(void* arg);
	void UpdateEndTime();
	void init();

protected:
	xmlNodePtr _xmlGetContentByName(xmlNodePtr rNode, char* name, char* value);
	int createAprNode(xmlNodePtr *node);
	int createEntryNode(xmlNodePtr *node, const char* ts, const char* type);
	int _fileIsExist();
	char* _findAndReturnBadFile();

private:
	xmlDocPtr m_doc;
	xmlNodePtr m_rootNode;	/* root Node, <aprs></aprs> */
	xmlNodePtr m_aprNode; /* <apr></apr> */
	xmlNodePtr m_exceptionsNode; /* exceptions Node, <exceptions></exceptions> */

	xmlNodePtr m_etNode;

	char m_dir[32];
	char m_pathname[48];
	pthread_mutex_t m_mutex;

};

#endif

