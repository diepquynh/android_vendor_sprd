
#include "common.h"
#include "XmlStorage.h"
#include "AprData.h"
#include <string>

#define MY_ENCODING "utf-8"

XmlStorage::XmlStorage(Observable *o, char* pdir, char* pfname):Observer(o)
{
	APR_LOGD("XmlStorage::XmlStorage()\n");
	strcpy(m_dir, pdir);
	strcpy(m_pathname, pdir);
	strcat(m_pathname, "/");
	strcat(m_pathname, pfname);

	m_doc = NULL;
	m_rootNode = NULL;
	m_aprNode = NULL;
	m_exceptionsNode = NULL;
	m_etNode = NULL;

	pthread_mutex_init(&m_mutex, NULL);
	this->init();
}

XmlStorage::~XmlStorage()
{
	APR_LOGD("XmlStorage::~XmlStorage()\n");
	m_dir[0] = '\0';
	m_pathname[0] = '\0';

	xmlFreeDoc(m_doc);
	pthread_mutex_destroy(&m_mutex);
}

void XmlStorage::init()
{
	APR_LOGD("XmlStorage::init()\n");
	int ret;
	AprData* aprData = static_cast<AprData *>(m_observable);
	char *default_value = (char*)"unknown";
	string v;
	char* pbadfile;

	// file exist
	if (_fileIsExist())
	{
		xmlKeepBlanksDefault(0); // libxml2 global variable.
		xmlIndentTreeOutput = 1; // indent .with \n
		m_doc = xmlParseFile(m_pathname);//, MY_ENCODING, XML_PARSE_RECOVER);
		if (NULL == m_doc) {
			APR_LOGE("%s not load successfully.\n", m_pathname);
			pbadfile = _findAndReturnBadFile();

			if (pbadfile) {
				if(rename(m_pathname, pbadfile)) {
					APR_LOGE("we didn't rename %s to %s, so exit.\n", m_pathname, pbadfile);
					exit(1);
				}
				free(pbadfile);

				goto next;
			} else {
				APR_LOGE("we didn't find an available bad file, so exit.\n");
				exit(1);
			}
		}
		m_rootNode = xmlDocGetRootElement(m_doc);
		// sysdump ??
		v = aprData->getBootMode();
		if (strcmp(v.c_str(), default_value))
		{
			xmlNodePtr aprNode = _xmlGetContentByName(m_rootNode, NULL, NULL);
			if (aprNode != NULL)
			{
				xmlNodePtr excepNode = _xmlGetContentByName(aprNode, (char*)"exceptions", NULL);
				if (NULL == excepNode) {
					excepNode = xmlNewNode(NULL, BAD_CAST"exceptions");
					xmlAddChild(aprNode, excepNode);
				}
				char strbuf[PROPERTY_VALUE_MAX];
				_xmlGetContentByName(aprNode, (char*)"endTime", strbuf);
				// add <entry>...<entry> to m_exceptionsNode
				xmlNodePtr entryNode;
				createEntryNode(&entryNode, strbuf, v.c_str());
				xmlAddChild(excepNode, entryNode);
			}
		}

	} else
	{
next:
		// document pointer
		m_doc = xmlNewDoc(BAD_CAST"1.0");
		// root node pointer
		m_rootNode = xmlNewNode(NULL, BAD_CAST"aprs");
		xmlDocSetRootElement(m_doc, m_rootNode);
	}

	// add <apr>...</apr> to m_rNode
	createAprNode(&m_aprNode);
	xmlAddChild(m_rootNode, m_aprNode);

	ret = xmlSaveFormatFileEnc(m_pathname, m_doc, MY_ENCODING, 1);
	/*
	 * If you have to long press the power button to restart the phone,
	 * Occasionally it's happened to empty the contents of the apr.xml file
	 * being written by 'collect_apr' process. sync() will reduce the
	 * probability.
	 */
	/*
	 * It's write-back to disk in global, so remove this sync() syscall
	 * to avoid influence on other modules.
	 */
	/* sync(); */
	if (ret != -1) {
		APR_LOGD("%s file is created\n", m_pathname);
	} else {
		APR_LOGE("xmlSaveFormatFile failed\n");
		exit(1);
	}
}

int XmlStorage::createAprNode(xmlNodePtr *node)
{
	string v;
	char value[PROPERTY_VALUE_MAX];
	AprData* aprData = static_cast<AprData *>(m_observable);

	// <apr>
	xmlNodePtr aprNode = xmlNewNode(NULL,BAD_CAST"apr");
	//     <hardwareVersion> </hardwareVersion>
	v = aprData->getHardwareVersion();
	xmlNewTextChild(aprNode, NULL, BAD_CAST "hardwareVersion", BAD_CAST v.c_str());
	//     <SN> </SN>
	v = aprData->getSN();
	xmlNewTextChild(aprNode, NULL, BAD_CAST "SN", BAD_CAST v.c_str());
	//     <buildNumber> </buildNumber>
	v = aprData->getBuildNumber();
	xmlNewTextChild(aprNode, NULL, BAD_CAST "buildNumber", BAD_CAST v.c_str());
	//     <CPVersion> </CPVersion>
	v = aprData->getCPVersion();
	xmlNewTextChild(aprNode, NULL, BAD_CAST "CPVersion", BAD_CAST v.c_str());
	//     <extraInfo> </extraInfo>
	v = aprData->getExtraInfo();
	xmlNewTextChild(aprNode, NULL, BAD_CAST "extraInfo", BAD_CAST v.c_str());
	//     <startTime> </startTime>
	//     <endTime> </endTime>
	aprData->getStartTime(value);
	xmlNewTextChild(aprNode, NULL, BAD_CAST "startTime", BAD_CAST value);
	aprData->getUpTime(value);
	m_etNode = xmlNewTextChild(aprNode, NULL, BAD_CAST "endTime", BAD_CAST value);
	//     <exceptions> </exceptions>
	m_exceptionsNode = NULL;
	// </apr>
	*node = aprNode;

	return 0;
}

int XmlStorage::createEntryNode(xmlNodePtr *node, const char* ts, const char* type)
{
	// <entry>
	xmlNodePtr entryNode = xmlNewNode(NULL, BAD_CAST "entry");
	//     <timestamp> </timestamp>
	xmlNewTextChild(entryNode, NULL, BAD_CAST "timestamp", BAD_CAST ts);
	//     <type> </type>
	xmlNewTextChild(entryNode, NULL, BAD_CAST "type", BAD_CAST type);
	// </entry>
	*node = entryNode;
	return 0;
}

void XmlStorage::handleEvent()
{

}

void XmlStorage::handleEvent(void* arg)
{
	APR_LOGD("XmlStorage::handleEvent()\n");
	int ret;
	char value[PROPERTY_VALUE_MAX];
	AprData* aprData = static_cast<AprData *>(m_observable);

	// lock
	pthread_mutex_lock(&m_mutex);

	if (!_fileIsExist()) {
		APR_LOGD("%s isn't exist!\n", m_pathname);
		xmlFreeDoc(m_doc);
		m_doc = NULL;
		m_rootNode = NULL;
		m_aprNode = NULL;
		m_exceptionsNode = NULL;

		// document pointer
		m_doc = xmlNewDoc(BAD_CAST"1.0");
		// root node pointer
		m_rootNode = xmlNewNode(NULL, BAD_CAST"aprs");
		xmlDocSetRootElement(m_doc, m_rootNode);

		// add <apr>...</apr> to m_rNode
		createAprNode(&m_aprNode);
		xmlAddChild(m_rootNode, m_aprNode);
	}

	//     <endTime> </endTime>
	aprData->getUpTime(value);
	xmlNodeSetContent(m_etNode, (const xmlChar*)value);

	if (arg) {
		if (NULL == m_exceptionsNode ) {
			m_exceptionsNode = xmlNewNode(NULL, BAD_CAST"exceptions");
			xmlAddChild(m_aprNode, m_exceptionsNode);
		}
		// add <entry>...<entry> to m_exceptionsNode
		xmlNodePtr entryNode;
		struct e_info *p_ei = (struct e_info*)arg;
		// get wall clock time as timestamp
		aprData->getWallClockTime(value, sizeof(value));
		createEntryNode(&entryNode, value, get_et_name(p_ei->et));
		if ( E_MODEM_ASSERT == p_ei->et || E_WCN_ASSERT == p_ei->et ) {
			xmlNewTextChild(entryNode, NULL, BAD_CAST "brief", BAD_CAST (char*)(p_ei->private_data));
		}
		xmlAddChild(m_exceptionsNode, entryNode);
	}

	ret = xmlSaveFormatFileEnc(m_pathname, m_doc, MY_ENCODING, 1);
	/*
	 * If you have to long press the power button to restart the phone,
	 * Occasionally it's happened to empty the contents of the apr.xml file
	 * being written by 'collect_apr' process. sync() will reduce the
	 * probability.
	 */
	/*
	 * It's write-back to disk in global, so remove this sync() syscall
	 * to avoid influence on other modules.
	 */
	/* sync(); */
	if (ret != -1) {
		APR_LOGD("%s file is created\n", m_pathname);
	} else {
		APR_LOGE("xmlSaveFormatFile failed\n");
		exit(1);
	}

	// unlock
	pthread_mutex_unlock(&m_mutex);
}

int XmlStorage::_fileIsExist()
{
	int retval;
	/* clear the umask */
	umask(0);
	// If directory is not exist, make dir
	if (access(m_dir, F_OK) < 0) {
		retval = mkdir(m_dir, S_IRWXU | S_IRWXG | S_IRWXO);
		if (retval < 0) {
			APR_LOGE("mkdir %s fail, error:%s\n", m_dir, strerror(errno));
			exit(1);
		}
	}

	// file not exist
	if (access(m_pathname, F_OK) < 0) {
		return false;
	} else {
		return true;
	}
}

char* XmlStorage::_findAndReturnBadFile()
{
	struct stat sb;
	char path[128];

	for (int i = 0; i < 10; i++) {
		snprintf(path, sizeof(path), "%s/apr_bad_%02d.xml", m_dir, i);

		if (!stat(path, &sb))
			continue;
		if (errno != ENOENT)
			continue;

		return strdup(path);
	}

	return NULL;
}

xmlNodePtr XmlStorage::_xmlGetContentByName(xmlNodePtr rNode, char* name, char* value)
{
	xmlChar *szKey;
	xmlNodePtr nextNode = rNode->xmlChildrenNode;
	xmlNodePtr curNode = NULL;
	while (nextNode != NULL) {
		curNode = nextNode;
		nextNode = nextNode->next;
		if (name != NULL) {
			if (!xmlStrcmp(curNode->name, (const xmlChar*)name)) {
				if (value != NULL) {
					szKey = xmlNodeGetContent(curNode);
					strcpy(value, (char*)szKey);
					xmlFree(szKey);
				}
				break;
			}
			curNode = nextNode;
		}
	}

	return curNode;
}
