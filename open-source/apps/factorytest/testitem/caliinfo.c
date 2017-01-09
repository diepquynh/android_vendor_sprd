#include "testitem.h"

extern int text_rows;
extern int cheight;
extern char s_cali_info1[1024];
extern char s_cali_info2[1024];
int version_change_page = 0;
extern char* test_modem_get_caliinfo(void);

/********************************************************************
*  Function: my_strstr()
*********************************************************************/
char * my_strstr(char * ps,char *pd)
{
    char *pt = pd;
    int c = 0;
    while(*ps != '\0')
    {
        if(*ps == *pd)
        {
            while(*ps == *pd && *pd!='\0')
            {
                ps++;
                pd++;
                c++;
            }
        }else
        {
            ps++;
        }
        if(*pd == '\0')
        {
            return (ps - c);
        }
        c = 0;
        pd = pt;
    }
    return 0;
}


/********************************************************************
*  Function:str_replace()
*********************************************************************/
int str_replace(char *p_result,char* p_source,char* p_seach,char *p_repstr)
{
    int c = 0;
    int repstr_leng = 0;
    int searchstr_leng = 0;
    char *p1;
    char *presult = p_result;
    char *psource = p_source;
    char *prep = p_repstr;
    char *pseach = p_seach;
    int nLen = 0;
    repstr_leng = strlen(prep);
    searchstr_leng = strlen(pseach);
    do{
        p1 = my_strstr(psource,p_seach);
        if (p1 == 0)
        {
            strcpy(presult,psource);
            return c;
        }
        c++;
        nLen = p1 - psource;
        memcpy(presult, psource, nLen);
        memcpy(presult + nLen,p_repstr,repstr_leng);
        psource = p1 + searchstr_leng;
        presult = presult + nLen + repstr_leng;
    }while(p1);
    return c;
}

int test_cali_info(void)
{
	int ret = 0;
	int i,tp_row;
	int row = 2;
	char tmp[64][64],gsm_cali[64][64],wcdma_cali[64][64], lte_cali[64][64];
	char property[PROPERTY_VALUE_MAX];
	char* pcur,*pos1,*pos2;
	int len, testlen,has_show = 0,remain_len = 0;
	int gsm_num=0, wcdma_num=0, lte_num=0;
	unsigned char chang_page=0,change=0;

	version_change_page = 1;
	tp_row = ((CHAR_HEIGHT>35)?(gr_fb_height()-(gr_fb_height()>>3)-CHAR_HEIGHT):(gr_fb_height()-2*CHAR_HEIGHT))/cheight;

	ui_fill_locked();
	ui_show_title(MENU_CALI_INFO);
	pcur = test_modem_get_caliinfo();
	len = strlen(pcur);
	ui_set_color(CL_WHITE);

	/*delete the "BIT",and replace the calibrated with cali */
	memset(tmp,0,sizeof(tmp));
	memset(gsm_cali,0,sizeof(gsm_cali));
	memset(wcdma_cali,0,sizeof(wcdma_cali));
	memset(lte_cali,0,sizeof(lte_cali));
	while(len > 0) {
		pos1 = strchr(pcur, ':');
		if(pos1 == NULL) break;
		pos1++;
		pos2 = strstr(pos1, "BIT");
		if(pos2 == NULL) {
			strcpy(tmp[gsm_num], pos1);
			len = 0;
		} else {
			memcpy(tmp[gsm_num], pos1, (pos2-pos1));
			tmp[gsm_num][pos2-pos1] = '\0';
			len -= (pos2 - pcur);
			pcur = pos2;
		}
		testlen=str_replace(gsm_cali[gsm_num],tmp[gsm_num],"calibrated","cali");
		LOGD("mmitest test=%s,gsm_num = %d",gsm_cali[gsm_num],gsm_num);
		gsm_num++;
	}
	memset(tmp,0,sizeof(tmp));
	property_get(PROP_MODEM_LTE_ENABLE, property, "not_find");
	if(!strcmp(property, "1")){
		pcur = s_cali_info2;
		len = strlen(pcur);
		while(len > 0) {
			pos1 = strchr(pcur, ':');
			if(pos1 == NULL) break;
			pos1++;
			pos2 = strstr(pos1, "BIT");
			if(pos2 == NULL) {
				strcpy(tmp[lte_num], pos1);
				len = 0;
			} else {
				memcpy(tmp[lte_num], pos1, (pos2-pos1));
				tmp[lte_num][pos2-pos1] = '\0';
				len -= (pos2 - pcur);
				pcur = pos2;
			}
			testlen=str_replace(lte_cali[lte_num],tmp[lte_num],"BAND","WCDMA BAND");
			LOGD("mmitest test=%s,lte_num = %d",lte_cali[lte_num],lte_num);
			lte_num++;
			//row = ui_show_text(row, 0, tmp2);
		}
	}

	memset(tmp,0,sizeof(tmp));
	pcur = s_cali_info1;
	len = strlen(pcur);
	while(len > 0) {
		pos1 = strchr(pcur, ':');
		if(pos1 == NULL) break;
		pos1++;
		pos2 = strstr(pos1, "BIT");
		if(pos2 == NULL) {
			strcpy(tmp[wcdma_num], pos1);
			len = 0;
		} else {
			memcpy(tmp[wcdma_num], pos1, (pos2-pos1));
			tmp[wcdma_num][pos2-pos1] = '\0';
			len -= (pos2 - pcur);
			pcur = pos2;
		}
		testlen=str_replace(wcdma_cali[wcdma_num],tmp[wcdma_num],"BAND","WCDMA BAND");
		LOGD("mmitest wcdma_cali[%d]=%s,wcdma_num = %d",wcdma_num,wcdma_cali[wcdma_num],wcdma_num);
		wcdma_num++;
		//row = ui_show_text(row, 0, tmp2);
	}
start:
	do{
        if( 0 == change%3){
            row=2;
            ui_set_color(CL_SCREEN_BG);
            gr_fill(0, 0, gr_fb_width(), gr_fb_height());
            ui_fill_locked();
            ui_show_title(MENU_CALI_INFO);
	      if(has_show == 0)
	          remain_len = gsm_num;
	      for(i=0;i<remain_len;i++){
                if(strstr(gsm_cali[i+has_show],"Pass")!=NULL)
                    ui_set_color(CL_GREEN);
                if(strstr(gsm_cali[i+has_show],"Not")!=NULL)
                    ui_set_color(CL_RED);
                if(strstr(gsm_cali[i+has_show],"MMI Test")!=NULL){
			 if(1 == remain_len){
			    change++;
			    has_show = 0;
			    goto start;
			 }else{
			    has_show += 1;
			    goto gsm_label;
			 }
                }
                LOGD("mmitest row=%d,tp_row=%d,gsm_cali[%d] = %s,gsm_num = %d",row,tp_row,(i+has_show),gsm_cali[i+has_show],gsm_num);
                row = ui_show_text(row, 0, gsm_cali[i+has_show]);
		   if(row-1 > tp_row){
			 change = 0;
			 remain_len -= i+1;
			 has_show += i+1;
			 i = 0;
			 gr_flip();
			 break;
		   }
	      }
gsm_label:
	      if((has_show+i) == gsm_num){
			change++;
			has_show = 0;
	      }
	      gr_flip();
        }else if( 1 == change%3){
            row=2;
            ui_set_color(CL_SCREEN_BG);
            gr_fill(0, 0, gr_fb_width(), gr_fb_height());
            ui_fill_locked();
            ui_show_title(MENU_CALI_INFO);
	      if(has_show == 0)
	          remain_len = wcdma_num;
	      for(i = 0; i <remain_len; i++){
                if(strstr(wcdma_cali[i+has_show],"Pass")!=NULL)
                    ui_set_color(CL_GREEN);
                if(strstr(wcdma_cali[i+has_show],"Not")!=NULL)
                    ui_set_color(CL_RED);
                if(strstr(wcdma_cali[i+has_show],"MMI Test")!=NULL){
			 if(1 == remain_len){
			    change++;
			    has_show = 0;
			    goto start;
			 }else{
			    has_show += 1;
			    goto wcdma_label;
			 }
		   }
                LOGD("mmitest row=%d,wcdma_cali[%d] = %s,wcdma_num = %d",row,(i+has_show),wcdma_cali[i+has_show],wcdma_num);
                row = ui_show_text(row, 0, wcdma_cali[i+has_show]);
		   if(row-1 > tp_row){
			 change = 1;
			 remain_len -= i+1;
			 has_show += i+1;
			 i = 0;
			 gr_flip();
			 break;
		   }
	      }
wcdma_label:
	      if ((has_show+i)  == wcdma_num){
			change++;
			has_show = 0;
	      }
	      gr_flip();
        } else if( 2 == change%3){
            row=2;
            ui_set_color(CL_SCREEN_BG);
            gr_fill(0, 0, gr_fb_width(), gr_fb_height());
            ui_fill_locked();
            ui_show_title(MENU_CALI_INFO);
	      if(has_show == 0)
	          remain_len = lte_num;
	      for(i=0;i<remain_len;i++){
                if(strstr(lte_cali[i+has_show],"Pass")!=NULL)
                    ui_set_color(CL_GREEN);
                if(strstr(lte_cali[i+has_show],"Not")!=NULL)
                    ui_set_color(CL_RED);
                if(strstr(lte_cali[i+has_show],"MMI Test")!=NULL){
			 if(1 == remain_len){
			    change++;
			    has_show = 0;
			    goto start;
			 }else{
			    has_show += 1;
			    goto lte_label;
			 }
		   }
                LOGD("mmitest row=%d,lte_cali[%d] = %s,lte_num = %d",row,(i+has_show),lte_cali[i+has_show],lte_num);
                row = ui_show_text(row, 0, lte_cali[i+has_show]);
		   if(row-1 > tp_row){
			 change = 2;
			 remain_len -= i+1;
			 has_show += i+1;
			 i = 0;
			 gr_flip();
			 break;
		   }
	      }
lte_label:
	      if((has_show+i)  == lte_num){
			change++;
			has_show = 0;
	      }
	      gr_flip();
        }
	  chang_page=ui_handle_button(NULL,TEXT_NEXT_PAGE,TEXT_GOBACK);
	}while(chang_page == RL_PASS || chang_page == RL_NEXT_PAGE);

	version_change_page = 0;
	return 0;
}
