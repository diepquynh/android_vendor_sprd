/**
 * INI配置文件管理函数库
 * Ini file parse functions.
 * By Hoverlees http://www.hoverlees.com me[at]hoverlees.com
 */

#include "common.h"
#include "confile.h"

#define MAX_SECTION_SIZE 64
typedef struct _PARSER{
	int status;
	int pos;
	int start1;
	int end1;
	int start2;
	int end2;
	int row;
	int col;
	unsigned char* str;
	int slen;
	INI_CONFIG* config;
	char section_name[MAX_SECTION_SIZE];
}PARSER;
typedef int (*PARSER_JUMP_FUNC)(PARSER* parser);

#define PARSE_STATUS_GET_KEY_OR_SECTION	1
#define PARSE_STATUS_GET_SECTION		2
#define PARSE_STATUS_GET_VALUE			3
#define PARSE_STATUS_COMMENT_LINE		4

CONFIG_BTREE_NODE* config_btree_find_node(CONFIG_BTREE* config,const char* key);

/**
 * 内部使用函数,实现内存段trim,返回第一个非空字符指针及字符串trim后的长度.
 */
char* mem_trim(char* src,int len,int* outlen){
	int start,end;
	if(len==0) return NULL;
	start=0;
	end=len-1;
	while(start<len && (src[start]==' ' || src[start]=='\r' || src[start]=='\n')){
		start++;
	}
	while(end>start && (src[end]==' ' || src[end]=='\r' || src[end]=='\n')){
		end--;
	}
	end=end+1;
	if(start==end) return NULL;
	src+=start;
	*outlen=end-start;
	return src;
}

/**
 * 下面是存储配置专用的二叉树实现.
 */
CONFIG_BTREE_NODE* config_btree_create_node(const char* key,int key_len,void* data,int data_len){
	char* p;
	CONFIG_BTREE_NODE* node;
	if(key_len==0) key_len=strlen(key);
	if(data_len==0) data_len=strlen(data)+1;
	node=(CONFIG_BTREE_NODE*) calloc(sizeof(CONFIG_BTREE_NODE)+key_len+data_len,1);
	if(node!=NULL){
		p=node->mem;
		node->key=p;
		p+=(key_len+1);
		node->data=p;
		memcpy(node->key,key,key_len);
		memcpy(node->data,data,data_len);
		node->left=NULL;
		node->right=NULL;
	}
	return node;
}
int config_btree_free_node(CONFIG_BTREE_NODE* node){
	free(node);
	return 1;
}
CONFIG_BTREE* config_btree_create(){
	CONFIG_BTREE* ret=(CONFIG_BTREE*) calloc(sizeof(CONFIG_BTREE),1);
	return ret;
}
int config_btree_insert_node(CONFIG_BTREE* config,const char* key,int key_len,void* data,int data_len){
	CONFIG_BTREE_NODE *p,**prev;
	CONFIG_BTREE_NODE* node;
	int comp;
	if(config==NULL) return 0;
	if(key_len==0) key_len=strlen(key);
	node=config_btree_create_node(key,key_len,data,data_len);
	if(node==NULL) return 0;
	p=config->root;
	prev=&config->root;
	if(!p){
		config->root=node;
		config->numNodes++;
		return 1;
	}
	while(p){
		comp=memcmp(key,p->key,key_len);
		if(comp>0){
			if(p->right==NULL){
				p->right=node;
				config->numNodes++;
				return 1;
			}
			prev=&p->right;
			p=p->right;
		}
		else if(comp<0){
			if(p->left==NULL){
				p->left=node;
				config->numNodes++;
				return 1;
			}
			prev=&p->left;
			p=p->left;
		}
		else{
			node->left=p->left;
			node->right=p->right;
			*prev=node;
			config_btree_free_node(p);
			return 2; //update
		}
	}
	return 0;
}

int config_btree_insert_section(CONFIG_BTREE* config,const char* section_name){
	CONFIG_BTREE* section=config_btree_create();
	if(section==NULL) return 0;
	if(config_btree_insert_node(config,section_name,0,&section,sizeof(void*))){
		return 1;
	}
	else{
		free(section);
		return 0;
	}
}

CONFIG_BTREE* config_btree_get_section(CONFIG_BTREE* config,const char* section_name){
	CONFIG_BTREE* section;
	CONFIG_BTREE_NODE* node;
	node=config_btree_find_node(config,section_name);
	if(node==NULL) return NULL;
	memcpy(&section,node->data,sizeof(void*));
	return section;
}

int config_btree_insert_section_node(CONFIG_BTREE* config,const char* section_name,const char* key,
										int key_len,void* data,int data_len){
	CONFIG_BTREE* section;
	CONFIG_BTREE_NODE* node;
	node=config_btree_find_node(config,section_name);
	if(node==NULL) return 0;
	memcpy(&section,node->data,sizeof(void*));
	return config_btree_insert_node(section,key,key_len,data,data_len);
}

CONFIG_BTREE_NODE* config_btree_find_node(CONFIG_BTREE* config,const char* key){
	CONFIG_BTREE_NODE* p;
	int comp;
	p=config->root;
	while(p){
		comp=strcmp(key,p->key);
		if(comp>0){
			p=p->right;
		}
		else if(comp<0){
			p=p->left;
		}
		else{
			return p;
		}
	}
	return NULL;
}

int config_btree_delete_node(CONFIG_BTREE* config,const char* key){
	CONFIG_BTREE_NODE* p;
	CONFIG_BTREE_NODE* temp;
	CONFIG_BTREE_NODE** prevTmpPos=NULL;
	CONFIG_BTREE_NODE** prevPos=NULL;
	int comp;
	prevPos=&config->root;
	p=config->root;
	while(p){
		comp=strcmp(key,p->key);
		if(comp>0){
			prevPos=&p->right;
			p=p->right;
		}
		else if(comp<0){
			prevPos=&p->left;
			p=p->left;
		}
		else{
			if(p->left){
				temp=p->left;
				while(temp->right){
					prevTmpPos=&temp->right;
					temp=temp->right;
				}
				if(prevTmpPos==NULL){
					*prevPos=temp;
					temp->right=p->right;
				}
				else{
					if(temp->left){
						*prevTmpPos=temp->left;
					}
					else{
						*prevTmpPos=NULL;
					}
					*prevPos=temp;
					temp->left=p->left;
					temp->right=p->right;
				}
				config_btree_free_node(p);
			}
			else if(p->right){
				temp=p->right;
				while(temp->left){
					prevTmpPos=&temp->left;
					temp=temp->left;
				}
				if(prevTmpPos==NULL){
					*prevPos=temp;
					temp->left=p->left;
				}
				else{
					if(temp->right){
						*prevTmpPos=temp->right;
					}
					else{
						*prevTmpPos=NULL;
					}
					*prevPos=temp;
					temp->left=p->left;
					temp->right=p->right;
				}
				config_btree_free_node(p);
			}
			else{
				config_btree_free_node(p);
				*prevPos=NULL;
			}
			config->numNodes--;
			return 1;
		}
	}
	return 0;
}

int config_btree_inorder_traverse(CONFIG_BTREE_NODE* node,CONFIG_BTREE_TRAVERSE_CB callback){
	if(node==NULL) return 1;
	if(!config_btree_inorder_traverse(node->left,callback)) return 0;
	if(!callback(node)) return 0;
	if(!config_btree_inorder_traverse(node->right,callback)) return 0;
	return 1;
}

int config_btree_inorder_save_traverse(CONFIG_BTREE_NODE* node,FILE* fp,CONFIG_BTREE_SAVE_TRAVERSE_CB callback){
	if(node==NULL) return 1;
	if(!config_btree_inorder_save_traverse(node->left,fp,callback)) return 0;
	if(!callback(fp,node)) return 0;
	if(!config_btree_inorder_save_traverse(node->right,fp,callback)) return 0;
	return 1;
}

int config_btree_preorder_traverse(CONFIG_BTREE_NODE* node,CONFIG_BTREE_TRAVERSE_CB callback){
	if(node==NULL) return 1;
	if(!callback(node)) return 0;
	if(!config_btree_preorder_traverse(node->left,callback)) return 0;
	if(!config_btree_preorder_traverse(node->right,callback)) return 0;
	return 1;
}

int config_btree_postorder_traverse(CONFIG_BTREE_NODE* node,CONFIG_BTREE_TRAVERSE_CB callback){
	if(node==NULL) return 1;
	if(!config_btree_postorder_traverse(node->left,callback)) return 0;
	if(!config_btree_postorder_traverse(node->right,callback)) return 0;
	if(!callback(node)) return 0;
	return 1;
}

int config_btree_destroy_section(CONFIG_BTREE_NODE* node){
	CONFIG_BTREE* section;
	if(!node) return 0;
	memcpy(&section,node->data,sizeof(void*));
	config_btree_postorder_traverse(section->root,config_btree_free_node);
	free(section);
	return 1;
}

int config_btree_destroy(CONFIG_BTREE* config){
	if(!config) return 0;
	config_btree_postorder_traverse(config->root,config_btree_destroy_section);
	free(config);
	return 1;
}

/**
 * ini文件解析函数跳转表,此方式在大型解析的实现中非常高效.
 */

int parser_default_action(PARSER* parser){
	parser->pos++;
	parser->col++;
	return 1;
}
int parse_default_gbk_action(PARSER* parser){
	parser->pos+=2;
	parser->col+=2;
	return 1;
}
int parser_on_section_start(PARSER* parser){
	if(parser->status==PARSE_STATUS_COMMENT_LINE){}
	else if(parser->status==PARSE_STATUS_GET_KEY_OR_SECTION){
		parser->start1=parser->pos+1;
		parser->status=PARSE_STATUS_GET_SECTION;
	}
	parser->pos++;
	parser->col++;
	return 1;
}
int parser_on_section_end(PARSER* parser){
	char* p;
	int len,r;
	if(parser->status==PARSE_STATUS_COMMENT_LINE){}
	else if(parser->status==PARSE_STATUS_GET_SECTION){
		memset(parser->section_name,0,MAX_SECTION_SIZE);
		p=mem_trim(parser->str+parser->start1,parser->pos-parser->start1,&len);
		if(p==NULL){//section段名不能为空
			return 0;
		}
		memcpy(parser->section_name,p,len);
		r=config_btree_insert_section(parser->config,parser->section_name);
		if(r==0) return 0;//添加section失败
		parser->status=PARSE_STATUS_GET_KEY_OR_SECTION;
		parser->start1=parser->pos+1;
	}
	parser->pos++;
	parser->col++;
	return 1;
}
int parser_on_value_start(PARSER* parser){
	char* p;
	int len,r;
	if(parser->status==PARSE_STATUS_GET_KEY_OR_SECTION){
		parser->status=PARSE_STATUS_GET_VALUE;
		parser->end1=parser->pos;
		parser->start2=parser->pos+1;
	}
	parser->pos++;
	parser->col++;
	return 1;
}
int parser_on_new_line(PARSER* parser){
	char *k,*v;
	int klen,vlen;
	switch(parser->status){
		case PARSE_STATUS_COMMENT_LINE:
			break;
		case PARSE_STATUS_GET_VALUE:
			k=mem_trim(parser->str+parser->start1,parser->end1-parser->start1,&klen);
			v=mem_trim(parser->str+parser->start2,parser->pos-parser->start2,&vlen);
			if(k==NULL) return 0;
			if(v==NULL){
				v="";
				vlen=0;
			}
			if(!config_btree_insert_section_node(parser->config,parser->section_name,k,klen,v,vlen)) return 0;
			break;
		case PARSE_STATUS_GET_KEY_OR_SECTION:
			break;
		default:
			return 0;
	}
	parser->start1=parser->pos+1;
	parser->status=PARSE_STATUS_GET_KEY_OR_SECTION;
	parser->pos++;
	parser->col=1;
	parser->row++;
	return 1;
}
int parser_on_comment(PARSER* parser){
	if(parser->col==1){
		parser->status=PARSE_STATUS_COMMENT_LINE;
	}
	parser->col++;
	parser->pos++;
	return 1;
}
/**
 * 接下来是ini配置管理的上层函数.
 */

INI_CONFIG* ini_config_create_from_string(unsigned char* str,int slen,int isGBK){
	int r;
	PARSER parser;
	PARSER_JUMP_FUNC funcs[256];
	INI_CONFIG* config=config_btree_create();
	if(slen==0) slen=strlen(str);
	strcpy(parser.section_name,"default");
	parser.pos=0;
	parser.status=PARSE_STATUS_GET_KEY_OR_SECTION;
	parser.start1=0;
	parser.str=str;
	parser.slen=slen;
	parser.row=1;
	parser.col=1;
	parser.config=config;
	//初始化解析跳转表
	if(isGBK){
		for(r=0;r<127;r++){
			funcs[r]=parser_default_action;
		}
		for(r=128;r<256;r++){
			funcs[r]=parse_default_gbk_action;
		}
	}
	else{
		for(r=0;r<256;r++){
			funcs[r]=parser_default_action;
		}
	}
	funcs['[']=parser_on_section_start;
	funcs[']']=parser_on_section_end;
	funcs['=']=parser_on_value_start;
	funcs['\n']=parser_on_new_line;
	funcs[';']=parser_on_comment;

	if(config!=NULL){
		r=config_btree_insert_section(config,parser.section_name);
		if(!r){
			config_btree_destroy(config);
			return NULL;
		}
	}
	while(parser.pos<slen){
		r=funcs[str[parser.pos]](&parser);
		if(!r){ //解析错误,本代码不做任何提示,直接返回ＮＵＬＬ,但可以从parser里在这里从parser里取得当前解析的文件位置和当前状态.
			config_btree_destroy(config);
			return NULL;
		}
	}
	r=parser_on_new_line(&parser);
	if(!r){
		config_btree_destroy(config);
		return NULL;
	}
	return config;
}

INI_CONFIG* ini_config_create_from_file(const char* filename,int isGBK){
	FILE* file;
	INI_CONFIG* config = NULL;
	char* buf;
	struct stat s;
	int ret;
	if(stat(filename,&s)) return NULL;
	buf=malloc(s.st_size);
	if(buf==NULL) return NULL;
	file=fopen(filename,"r");
	if(file==NULL){
		free(buf);
		return NULL;
	}
	ret = fread(buf,s.st_size,1,file);
	if (ret<1) goto error;
	config=ini_config_create_from_string(buf,s.st_size,isGBK);
error:
	free(buf);
	fclose(file);
	return config;
}

void ini_config_destroy(INI_CONFIG* config){
	config_btree_destroy(config);
}

int ini_config_get_int(INI_CONFIG* config,const char* section,const char* key,int default_int){
	CONFIG_BTREE_NODE* node;
	INI_CONFIG* sec;
	if(section==NULL) section="default";
	sec=config_btree_get_section(config,section);
	if(sec==NULL) return default_int;
	node=config_btree_find_node(sec,key);
	if(node==NULL) return default_int;
	return atoi(node->data);
}

char* ini_config_get_string(INI_CONFIG* config,const char* section,const char* key,char* default_string){
	CONFIG_BTREE_NODE* node;
	INI_CONFIG* sec;
	if(section==NULL) section="default";
	sec=config_btree_get_section(config,section);
	if(sec==NULL) return default_string;
	node=config_btree_find_node(sec,key);
	if(node==NULL) return default_string;
	return (char*)node->data;
}

int ini_config_get_bool(INI_CONFIG* config,const char* section,const char* key, int default_bool){
	CONFIG_BTREE_NODE* node;
	INI_CONFIG* sec;
	if(section==NULL) section="default";
	sec=config_btree_get_section(config,section);
	if(sec==NULL) return default_bool;
	node=config_btree_find_node(sec,key);
	if(node==NULL) return default_bool;
	switch (((char*)node->data)[0])
	{
		case 't':
		case 'T':
		case 'y':
		case 'Y':
		case '1':
			return 1;
		case 'f':
		case 'F':
		case 'n':
		case 'N':
		case '0':
			return 0;
		default:
			return default_bool;
	}
}

int ini_config_set_string(INI_CONFIG* config,const char* section,const char* key,int key_len,const char* value,int value_len){
	CONFIG_BTREE* sect;
	CONFIG_BTREE* node;
	int r;
	if(section==NULL) section="default";
	sect=config_btree_get_section(config,section);
	if(sect==NULL){
		r=config_btree_insert_section(config,section);
		if(!r) return 0;
		sect=config_btree_get_section(config,section);
	}
	return config_btree_insert_node(sect,key,key_len,(void*)value,value_len);
}

int ini_config_set_int(INI_CONFIG* config,const char* section,const char* key,int key_len,int value){
	char number[32];
	int len=sprintf(number,"%d",value);
	return ini_config_set_string(config,section,key,key_len,number,len);
}

int ini_config_save_traverse_value(FILE* fp,CONFIG_BTREE_NODE* node){
	fprintf(fp,"%s=%s\n",node->key,(char*)node->data);
	return 1;
}
int ini_config_save_traverse_section(FILE* fp,CONFIG_BTREE_NODE* node){
	CONFIG_BTREE* section;
	memcpy(&section,node->data,sizeof(void*));
	fprintf(fp,"[%s]\n",node->key);
	config_btree_inorder_save_traverse(section->root,fp,ini_config_save_traverse_value);
	return 1;
}
int ini_config_save(INI_CONFIG* config,const char* filename){
	FILE* fp=fopen(filename,"w");
	if(fp==NULL) return 0;
	config_btree_inorder_save_traverse(config->root,fp,ini_config_save_traverse_section);
	fclose(fp);
	return 1;
}
int ini_config_print(INI_CONFIG* config,FILE* fp){
	if(fp==NULL) return 0;
	config_btree_inorder_save_traverse(config->root,fp,ini_config_save_traverse_section);
	return 1;
}
