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
	enum EXISTS_FLAG{REAL_FILE, SWP1, SWP2};
	xmlNodePtr _xmlGetContentByName(xmlNodePtr rNode, char* name, char* value);
	int createAprNode(xmlNodePtr *node);
	int createEntryNode(xmlNodePtr *node, const char* ts, const char* type);
	int _fileIsExist(EXISTS_FLAG flag);
	void copy_to_xml(const char * file_swp, const char * file);
	char* _findAndReturnBadFile();
	int badfile_process(const char *filename);

private:
	xmlDocPtr m_doc;
	xmlNodePtr m_rootNode;	/* root Node, <aprs></aprs> */
	xmlNodePtr m_aprNode; /* <apr></apr> */
	xmlNodePtr m_exceptionsNode; /* exceptions Node, <exceptions></exceptions> */

	xmlNodePtr m_etNode;
	xmlNodePtr m_CPVersionNode;
	int flag_copy;

	char m_dir[32];
	char m_pathname[48];
	char m_pathname_swp1[56];
	char m_pathname_swp2[56];
	pthread_mutex_t m_mutex;

};

#endif

