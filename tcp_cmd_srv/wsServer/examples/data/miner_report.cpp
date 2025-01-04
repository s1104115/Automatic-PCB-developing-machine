#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ws.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <curl/easy.h>
#include <json-c/json.h>
#include <hiredis/hiredis.h> 

#include "xlsxwriter.h"

using namespace std;


struct MemoryStruct 
{
    char *memory;
    size_t size;
    MemoryStruct()
    {
        memory = (char *)malloc(1);
        size = 0;
    }
    ~MemoryStruct()
    {
        free(memory);
        memory = NULL;
        size = 0;
    }
};


const char *hash_unit[] = {"H/s", "kH/s", "MH/s", "GH/s", "TH/s", "PH/s", "EH/s"};
long divisor[7] = {1, 1000, 1000000, 1000000000, 1000000000000, 1000000000000000, 1000000000000000000}; 
unsigned int data_counter = 1;
int is_run = 0;
long d_networkhashps;
long d_currentHashrate;
long d_difficulty;
long d_dagSize;
long d_height;
int d_epoch;

long s_networkhashps;
long s_rnhashrate;

char buffer_nethash[32];
char buffer_rnhash[32];

static int hu = 20;
static int tem = 20;
static char cmd_path[128];

static redisContext* db_c;

lxw_workbook  *workbook;
lxw_worksheet *worksheet;

time_t TimeConvert(char *ascii_time)
{
	time_t result = 0;
   	int year = 0, month = 0, day = 0, hour = 0, min = 0;

	if (sscanf(ascii_time, "%4d/%2d/%2d %2d:%2d", &year, &month, &day, &hour, &min) == 5) {
       	struct tm breakdown = {0};
       	breakdown.tm_year = year - 1900; /* years since 1900 */
      	 	breakdown.tm_mon = month - 1;
       	breakdown.tm_mday = day;
       	breakdown.tm_hour = hour;
       	breakdown.tm_min = min;
    
       	if ((result = mktime(&breakdown)) == (time_t)-1) {
          		printf("Could not convert time input to time_t\n");
          		return -1;
       		}
       	//printf("TimeConvert : %d\n",result);
       	return result;
   	}
	return -1;
}

void DoRediusExportAVGHashrate(int  start_t, int end_t)
{
	char command[256];
	float tmp_amount;
	char tmp_amount_str[32];
	char tmp_date_str[32];
	int i, data_cnt;
	char file_path[256];

	memset(file_path,0,256);
	sprintf(file_path,"hashrate.xlsx"); 
	

	if(db_c == NULL)
		return; 

	workbook  = workbook_new(file_path);
    	worksheet = workbook_add_worksheet(workbook, NULL);

	worksheet_set_column(worksheet, 0, 0, 20, NULL);
	worksheet_set_column(worksheet, 0, 1, 20, NULL);

	worksheet_write_string(worksheet, 0, 0, "Time", NULL);
	worksheet_write_string(worksheet, 0, 1, "AVG Hashrate", NULL);

	data_cnt = 0;

	memset(command,0,256);

	//sprintf(command,"ZRANGEBYSCORE %s -inf +inf WITHSCORES",key);
	if(start_t < 0) //ALL
		sprintf(command,"ZRANGE avghashrate 0 -1 withscores");
	else //Range
		sprintf(command,"ZRANGE avghashrate %d %d BYSCORE withscores",start_t,end_t);
	
	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure : %s\n",command); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	time_t timep; 
	struct tm *p;

	if(r->type==REDIS_REPLY_ARRAY)
       {
		for(i=0; i<r->elements; i++)
            {
            		if(i % 2 == 0) { //Amount				
				tmp_amount = (float)((float)atol(r->element[i]->str)/1000000000);
				//printf("Hashrate = %.2f %s\n", (float)((float)atol(r->element[i]->str)/divisor[digi_count/3]), hash_unit[digi_count/3]);
    			} else { //Timestamp
    				data_cnt++;
        			timep = atoi(r->element[i]->str);
				p=localtime(&timep); 
				memset(tmp_date_str,0,32);
				memset(tmp_amount_str,0,32);
				sprintf(tmp_date_str,"%04d/%02d/%02d %02d:%02d:%02d",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
				sprintf(tmp_amount_str,"%.4f",tmp_amount);
				//printf("%04d/%02d/%02d %02d:%02d:%02d %.2f\n",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec,tmp_amount); 
				worksheet_write_string(worksheet, data_cnt, 0, tmp_date_str, NULL);
				worksheet_write_string(worksheet, data_cnt, 1, tmp_amount_str, NULL);
			}
                //printf(" %d) %s\n",i,r->element[i]->str);
            }
		
	}

	freeReplyObject(r); 
	workbook_close(workbook);
	#if KT_DEBUG
	printf("Succeed to execute DoRediusExportAVGHashrate command[%s]\n", command); 
	#endif
}

void DoRediusExportPayouts(int  start_t, int end_t)
{
	char command[256];
	float tmp_amount;
	char tmp_amount_str[32];
	char tmp_date_str[32];
	int i, data_cnt;
	char file_path[256];

	memset(file_path,0,256);
	sprintf(file_path,"deposit.xlsx"); 
	

	if(db_c == NULL)
		return; 

	workbook  = workbook_new(file_path);
    	worksheet = workbook_add_worksheet(workbook, NULL);

	worksheet_set_column(worksheet, 0, 0, 20, NULL);
	worksheet_set_column(worksheet, 0, 1, 20, NULL);

	worksheet_write_string(worksheet, 0, 0, "Time", NULL);
	worksheet_write_string(worksheet, 0, 1, "Amount", NULL);

	data_cnt = 0;

	memset(command,0,256);

	//sprintf(command,"ZRANGEBYSCORE %s -inf +inf WITHSCORES",key);
	if(start_t < 0) //ALL
		sprintf(command,"ZRANGE payouts 0 -1 withscores");
	else //Range
		sprintf(command,"ZRANGE payouts %d %d BYSCORE withscores",start_t,end_t);
	
	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure : %s\n",command); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	time_t timep; 
	struct tm *p;

	if(r->type==REDIS_REPLY_ARRAY)
       {
		for(i=0; i<r->elements; i++)
            {
            		if(i % 2 == 0) { //Amount				
				tmp_amount = (float)((float)atol(r->element[i]->str)/1000000000);
				//printf("Hashrate = %.2f %s\n", (float)((float)atol(r->element[i]->str)/divisor[digi_count/3]), hash_unit[digi_count/3]);
    			} else { //Timestamp
    				data_cnt++;
        			timep = atoi(r->element[i]->str);
				p=localtime(&timep); 
				memset(tmp_date_str,0,32);
				memset(tmp_amount_str,0,32);
				sprintf(tmp_date_str,"%04d/%02d/%02d %02d:%02d:%02d",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
				sprintf(tmp_amount_str,"%.4f",tmp_amount);
				//printf("%04d/%02d/%02d %02d:%02d:%02d %.2f\n",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec,tmp_amount); 
				worksheet_write_string(worksheet, data_cnt, 0, tmp_date_str, NULL);
				worksheet_write_string(worksheet, data_cnt, 1, tmp_amount_str, NULL);
			}
                //printf(" %d) %s\n",i,r->element[i]->str);
            }
		
	}

	freeReplyObject(r); 
	workbook_close(workbook);
	#if KT_DEBUG
	printf("Succeed to execute DoRediusReadAmount command[%s]\n", command); 
	#endif
}

void DoRediusConnect(void)
{
	db_c = redisConnect("127.0.0.1", 6379); 
    	if ( db_c->err) 
    	{ 
        	redisFree(db_c); 
		db_c = NULL;
        	printf("Connect to redisServer faile\n"); 
        	return ; 
    	} 
    	printf("Connect to redisServer Success\n"); 

}

void DoSendDone(ws_cli_conn_t *client)
{
	char ws_buf[128];

	memset(ws_buf,0,128);
	sprintf(ws_buf,"done,0,0");
	ws_sendframe_txt(client, ws_buf);
}


void DoRediusDisconnect(void)
{
	if(db_c != NULL)
		redisFree(db_c); 
}

int main(int argc, const char* argv[])
{
	DoRediusConnect();

  	DoRediusExportAVGHashrate(-1,-1);

	DoRediusDisconnect();

    return (0);
}
