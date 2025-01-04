#include <stdio.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <json-c/json.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include<hiredis/hiredis.h>

#include "xlsxwriter.h"

using namespace std;

const char *hash_unit[] = {"H/s", "kH/s", "MH/s", "GH/s", "TH/s", "PH/s", "EH/s"};
long divisor[7] = {1, 1000, 1000000, 1000000000, 1000000000000, 1000000000000000, 1000000000000000000}; 
lxw_workbook  *workbook;
lxw_worksheet *worksheet;
lxw_workbook  *workbook_pay;
lxw_worksheet *worksheet_pay;
unsigned int data_counter = 1;
int is_run = 1;
long d_networkhashps;
long d_difficulty;
long d_dagSize;
long d_height;
int d_epoch;

static redisContext* db_c;

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

void sig_handler(int signo)
{
    	if (signo == SIGUSR1)
       	printf("received SIGUSR1\n");
    	else if (signo == SIGKILL)
       	printf("received SIGKILL\n");
    	else if (signo == SIGSTOP)
   		printf("received SIGSTOP\n");
	else if (signo == SIGHUP)
        	printf("received SIGHUP\n");
	else if (signo == SIGINT)
        	printf("received SIGINT\n");
	else if (signo == SIGQUIT)
       	printf("received SIGQUIT\n");

	is_run = 0;
}

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) 
    {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}

void DoRediusReadHash(char *key, long hashrate)
{
	char command[256];
	int i;

	if(db_c == NULL)
		return; 

	memset(command,0,256);

	sprintf(command,"ZRANGEBYSCORE %s -inf +inf WITHSCORES",key);

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	time_t timep; 
	struct tm *p;
	int digi_count;
	long tmp_hash;
	
	if(r->type==REDIS_REPLY_ARRAY)
       {
		for(i=0; i<r->elements; i++)
            {
            		if(i % 2 == 0) { //Hashrate
        			digi_count = 0;
				tmp_hash = (long)atol(r->element[i]->str);
			 	do
    				{
        				digi_count++;
        				tmp_hash /= 10;
    				}while(tmp_hash != 0);
				digi_count -= 1;
				
				printf("Hashrate = %.2f %s\n", (float)((float)atol(r->element[i]->str)/divisor[digi_count/3]), hash_unit[digi_count/3]);
    			} else { //Timestamp
        			timep = atoi(r->element[i]->str);
				p=localtime(&timep); 
				printf("%04d-%02d-%02d-%02d-%02d-%02d\n",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec); 
    			}
                //printf(" %d) %s\n",i,r->element[i]->str);
            }
		
	}

	freeReplyObject(r); 

	printf("Succeed to execute DoRediusReadHash command[%s]\n", command); 
}

void DoRediusWriteLong(char *key, long value)
{
	time_t timep; 
	char command[256];

	if(db_c == NULL)
		return; 

	time(&timep); 
	memset(command,0,256);

	sprintf(command,"zadd %s %d %llu",key,timep,value);

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	/*
	if( !(r->type == REDIS_REPLY_INTEGER && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	//REDIS_REPLY_INTEGER
	/*
    	if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	printf("Succeed to execute DoRediusWriteLong command[%s] :: %d\n", command,r->integer); 

	freeReplyObject(r); 	
}

void DoRediusWriteInt(char *key, int value)
{
	time_t timep; 
	char command[256];

	if(db_c == NULL)
		return; 

	time(&timep); 
	memset(command,0,256);

	sprintf(command,"zadd %s %d %d",key,timep,value);

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	/*
	if( !(r->type == REDIS_REPLY_INTEGER && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	//REDIS_REPLY_INTEGER
	/*
    	if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	printf("Succeed to execute DoRediusWriteInt command[%s] :: %d\n", command,r->integer); 

	freeReplyObject(r); 	
}

void DoRediusWritePayouts(char *timestamp, char *value)
{
	time_t timep; 
	char command[256];

	if(db_c == NULL)
		return; 

	time(&timep); 
	memset(command,0,256);

	sprintf(command,"zadd payouts NX %s %s",timestamp,value);

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	/*
	if( !(r->type == REDIS_REPLY_INTEGER && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	//REDIS_REPLY_INTEGER
	/*
    	if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	printf("Succeed to execute DoRediusWritePayouts command[%s] :: %d\n", command,r->integer); 

	freeReplyObject(r); 	
}

void DoRediusWritePayoutsTXid(char *timestamp, char *value)
{
	time_t timep; 
	char command[256];

	if(db_c == NULL)
		return; 

	time(&timep); 
	memset(command,0,256);

	sprintf(command,"zadd txid NX %s %s",timestamp,value);

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	/*
	if( !(r->type == REDIS_REPLY_INTEGER && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	//REDIS_REPLY_INTEGER
	/*
    	if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	printf("Succeed to execute DoRediusWritePayoutsTXid command[%s] :: %d\n", command,r->integer); 

	freeReplyObject(r); 	
}

void DoRediusWriteETCEstimate(char *value)
{
	time_t timep; 
	char command[256];

	if(db_c == NULL)
		return; 

	time(&timep); 
	memset(command,0,256);

	sprintf(command,"zadd etcestimate NX %d %s",timep,value);

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	/*
	if( !(r->type == REDIS_REPLY_INTEGER && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	//REDIS_REPLY_INTEGER
	/*
    	if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	printf("Succeed to execute DoRediusWriteETCEstimate command[%s] :: %d\n", command,r->integer); 

	freeReplyObject(r); 	
}

void DoRediusWriteETCPrice(char *value)
{
	time_t timep; 
	struct tm *p;
	char command[256];

	if(db_c == NULL)
		return; 

	time(&timep); 
	p=localtime(&timep); 

	srand(time(NULL));
	
	memset(command,0,256);

	sprintf(command,"zadd etcprice NX %d %.4f",timep,(float)atof(value)+(float)(rand()%20)/10000);

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	/*
	if( !(r->type == REDIS_REPLY_INTEGER && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	//REDIS_REPLY_INTEGER
	/*
    	if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	printf("Succeed to execute DoRediusWriteETCPrice command[%s] :: %d (%04d-%02d-%02d %02d:%02d:%02d)\n", command,r->integer,
	(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec); 

	freeReplyObject(r); 	
}

void DoRediusSyncRDB(void)
{
	char command[256];

	if(db_c == NULL)
		return; 

	sprintf(command,"save");

	redisReply* r = (redisReply*)redisCommand(db_c, command);

	if( (NULL == r) || (r->type==REDIS_REPLY_ERROR)) 
    	{ 
       	printf("Execut command failure\n"); 
        	redisFree(db_c); 
		db_c = NULL;
        	return; 
    	} 

	/*
	if( !(r->type == REDIS_REPLY_INTEGER && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	//REDIS_REPLY_INTEGER
	/*
    	if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)) 
    	{ 
       	printf("Failed to execute command[%s]\n",command); 
        	freeReplyObject(r); 
        	redisFree(db_c); 
        	return; 
    	}  
	*/

	printf("Succeed to execute DoRediusSyncRDB command[%s] Reply Type : %d Reply String : %s\n", command,r->type,r->str); 

	freeReplyObject(r); 	
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

void DoRediusDisconnect(void)
{
	if(db_c != NULL)
		redisFree(db_c); 
}


int GetETCNetworkStatus(void)
{
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    	if(CURLE_OK != res)
    	{
		printf("curl init failed\n");
       	return 1;
    	}

    	CURL *pCurl = NULL;
    	pCurl = curl_easy_init();

    	if( NULL == pCurl)
    	{
    		printf("Init CURL failed...\n");
        	return -1;
    	}

    	string url = "https://etc.2miners.com/api/stats"; 
    	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 120L);
    	curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 60L); 
    	curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
    	curl_easy_setopt(pCurl, CURLOPT_HEADER, 0L);  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  

    	MemoryStruct oDataChunk;  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &oDataChunk);

    	curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L); 
    	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 0L); // 1L for debug
    	curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str() ); 

    	curl_slist *pList = NULL;
    	pList = curl_slist_append(pList,"accept: application/json"); 
    	curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pList); 

    	res = curl_easy_perform(pCurl); 

    	long res_code=0;
    	res=curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &res_code);

   
    	if(( res == CURLE_OK ) && (res_code == 200 || res_code == 201))
    	{
		struct json_object *root, *etc_nodes, *networkhashps, *worker_online, *worker_offline, *current_hashrate,  *average_hashrate;
		struct json_object *total_payment, *status, *paid;
		root = json_tokener_parse((char *)oDataChunk.memory);

		if (root != NULL) {
 			etc_nodes = json_object_object_get(root, "nodes");
			networkhashps = json_object_object_get(root, "apiVersion");
			printf("networkhashps = %s\n",json_object_get_string(networkhashps));

			json_object_put(root);
		}

		//printf("!!!!!!!! %s\n",oDataChunk.memory);

		
		string filename = "dump.json";
        	FILE *fp=fopen(filename.c_str(),"wb+");
        	if(!fp)
        	{   
			printf("open file failed\n");
        	} else {
        		fwrite(oDataChunk.memory, 1, oDataChunk.size, fp);
        		fclose(fp);
        	}
		
		/*
		json_object *root = json_object_from_file("dump.json");
   		printf("The json file:\n\n%s\n", json_object_to_json_string(root));

   		printf("The json file:\n\n%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
   		json_object_put(root);
		*/
    	}
    	curl_slist_free_all(pList); 
    	curl_easy_cleanup(pCurl);
    	curl_global_cleanup();

	return 0;
}

int GetWorkertStatus(char *walletID)
{
	printf("=======GetWorkertStatus========\n");
	unsigned int eight_miner_cnt, two_miner_cnt, one_miner_cnt;

	workbook  = workbook_new("worker_status.xlsx");
    	worksheet = workbook_add_worksheet(workbook, NULL);
	/* Change the column width for clarity. */
	worksheet_set_column(worksheet, 0, 0, 20, NULL);
    	worksheet_set_column(worksheet, 0, 1, 20, NULL);
	worksheet_set_column(worksheet, 0, 2, 20, NULL);
	worksheet_set_column(worksheet, 0, 3, 20, NULL);
	worksheet_set_column(worksheet, 0, 4, 20, NULL);
	worksheet_set_column(worksheet, 0, 5, 20, NULL);
	worksheet_set_column(worksheet, 0, 6, 20, NULL);


	worksheet_write_string(worksheet, 0, 0, "Miner Model", NULL);
	worksheet_write_string(worksheet, 1, 0, "Total Miner", NULL);
	worksheet_write_string(worksheet, 2, 0, "Total W1", NULL);
	worksheet_write_string(worksheet, 3, 0, "Total Hashrate", NULL);
	worksheet_write_string(worksheet, 4, 0, "Avg Hashrate", NULL);


	worksheet_write_string(worksheet, 0, 1, "W1x8", NULL);
	worksheet_write_string(worksheet, 0, 2, "W1x2", NULL);
	worksheet_write_string(worksheet, 0, 3, "W1x1", NULL);
	worksheet_write_string(worksheet, 0, 4, "SUM", NULL);
	worksheet_write_string(worksheet, 0, 5, "Target", NULL);
	worksheet_write_string(worksheet, 0, 6, "Short of", NULL);


	eight_miner_cnt = two_miner_cnt = one_miner_cnt = 0;
	
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    	if(CURLE_OK != res)
    	{
		printf("curl init failed\n");
       	return 1;
    	}

    	CURL *pCurl = NULL;
    	pCurl = curl_easy_init();

    	if( NULL == pCurl)
    	{
    		printf("Init CURL failed...\n");
        	return -1;
    	}

    	string url = "https://etc.2miners.com/api/accounts/0x61a186b076dd7a16b3829f12b9823e29e64cb5b9"; 
    	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 120L);
    	curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 60L); 
    	curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
    	curl_easy_setopt(pCurl, CURLOPT_HEADER, 0L);  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  

    	MemoryStruct oDataChunk;  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &oDataChunk);

    	curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L); 
    	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 0L); // 1L for debug
    	curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str() ); 

    	curl_slist *pList = NULL;
    	pList = curl_slist_append(pList,"accept: application/json"); 
    	curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pList); 

    	res = curl_easy_perform(pCurl); 

    	long res_code=0;
    	res=curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &res_code);

   
    	if(( res == CURLE_OK ) && (res_code == 200 || res_code == 201))
    	{
    		int objlen, i;
		enum json_type type;
		struct json_object *root, *worker_total, *workers, *workers_array_obj, *workers_array_lastBeat;
		struct json_object *worker_hr;
		root = json_tokener_parse((char *)oDataChunk.memory);

		if (root != NULL) {
			worker_total = json_object_object_get(root, "workersTotal");
			workers = json_object_object_get(root, "workers");
			objlen = json_object_object_length(workers);
			printf("Total=%d / %d\n", json_object_get_int(worker_total), objlen);
			if(workers != NULL) {
				struct json_object *new_obj;
				struct json_object_iterator it;
				struct json_object_iterator itEnd;
				float avg_hr;
				float  total_8_hr, total_2_hr, total_1_hr;
				float target_total_hr = 3150*180*1000000;
				float short_total_hr = 0;

 				total_8_hr = total_2_hr = total_1_hr = 0;

				it = json_object_iter_init_default();
				//new_obj = json_tokener_parse(input);
				it = json_object_iter_begin(workers);
				itEnd = json_object_iter_end(workers);
				i = 0;
				while (!json_object_iter_equal(&it, &itEnd))
				{
					//printf("worker name : %s\n", json_object_iter_peek_name(&it));
					//printf("%s\n", json_object_to_json_string(json_object_iter_peek_value(&it)));
					workers_array_obj = json_object_iter_peek_value(&it);
					worker_hr = json_object_object_get(workers_array_obj, "hr2");
					avg_hr = json_object_get_int64(worker_hr);
					//printf("hr2 = %s\n",json_object_get_string(worker_hr));

					if(avg_hr > 800000000) {
						eight_miner_cnt++;
						total_8_hr += avg_hr;
						worksheet_write_number(worksheet, eight_miner_cnt+4, 1, avg_hr, NULL);
					} else if(avg_hr > 200000000) {
						two_miner_cnt++;
						total_2_hr += avg_hr;
						worksheet_write_number(worksheet, two_miner_cnt+4, 2, avg_hr, NULL);
					} else {
						one_miner_cnt++;
						total_1_hr += avg_hr;
						worksheet_write_number(worksheet, one_miner_cnt+4, 3, avg_hr, NULL);
					}
					
					json_object_iter_next(&it);
					i++;
				}
				printf("=========Total : %d=========\n",i);
				printf("8 : %d\n",eight_miner_cnt);
				printf("2 : %d\n",two_miner_cnt);
				printf("1 : %d\n",one_miner_cnt);
				worksheet_write_number(worksheet, 1, 1, eight_miner_cnt, NULL);
				worksheet_write_number(worksheet, 1, 2, two_miner_cnt, NULL);
				worksheet_write_number(worksheet, 1, 3, one_miner_cnt, NULL);
				worksheet_write_number(worksheet, 1, 4, eight_miner_cnt+two_miner_cnt+one_miner_cnt, NULL);

				worksheet_write_number(worksheet, 2, 1, eight_miner_cnt*8, NULL);
				worksheet_write_number(worksheet, 2, 2, two_miner_cnt*2, NULL);
				worksheet_write_number(worksheet, 2, 3, one_miner_cnt, NULL);
				worksheet_write_number(worksheet, 2, 4, eight_miner_cnt*8+two_miner_cnt*2+one_miner_cnt, NULL);
				worksheet_write_number(worksheet, 2, 5, 3150, NULL);
				worksheet_write_number(worksheet, 2, 6, 3150-(eight_miner_cnt*8+two_miner_cnt*2+one_miner_cnt), NULL);

				worksheet_write_number(worksheet, 3, 1, total_8_hr, NULL);
				worksheet_write_number(worksheet, 3, 2, total_2_hr, NULL);
				worksheet_write_number(worksheet, 3, 3, total_1_hr, NULL);
				worksheet_write_number(worksheet, 3, 4, total_8_hr+total_2_hr+total_1_hr, NULL);
				worksheet_write_number(worksheet, 3, 5, target_total_hr, NULL);
				short_total_hr = target_total_hr - (total_8_hr+total_2_hr+total_1_hr);
				worksheet_write_number(worksheet, 3, 6, short_total_hr, NULL);

				worksheet_write_number(worksheet, 4, 1, total_8_hr/eight_miner_cnt, NULL);
				worksheet_write_number(worksheet, 4, 2, total_2_hr/two_miner_cnt, NULL);
				worksheet_write_number(worksheet, 4, 3, total_1_hr/one_miner_cnt, NULL);

				//json_object_put(new_obj);
				#if 0
				objlen = json_object_object_length(workers);
				printf("!!!!!!!!!!!!! %d\n",objlen);
				
				//for (i = 0; i < objlen; i++) {
					workers_array_obj = json_object_get(workers);
					printf("***************** %s\n",json_object_get_string(workers_array_obj));
					//workers_array_obj = json_object_array_get_idx(workers, i);
					/*
					type = json_object_get_type(workers_array_obj);
    					if (type == json_type_array) {
      						printf("*****************\n");
    					}
					*/
					//workers_array_lastBeat = json_object_object_get(workers_array_obj, "lastBeat");
					//printf("~~~~~~~ %s\n",json_object_get_string(workers_array_lastBeat));
				//}
				#endif
			}
			json_object_put(root);
		}
    	}

	curl_slist_free_all(pList); 
    	curl_easy_cleanup(pCurl);
    	curl_global_cleanup();

	workbook_close(workbook);

	return 0;
}

int GetAllStatus(void)
{
	time_t timep; 
	struct tm *p;

	printf("=======GetAllStatus========\n");

	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    	if(CURLE_OK != res)
    	{
		printf("curl init failed\n");
       	return 1;
    	}

    	CURL *pCurl = NULL;
    	pCurl = curl_easy_init();

    	if( NULL == pCurl)
    	{
    		printf("Init CURL failed...\n");
        	return -1;
    	}

    	string url = "https://etc.2miners.com/api/stats"; 
    	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 120L);
    	curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 60L); 
    	curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
    	curl_easy_setopt(pCurl, CURLOPT_HEADER, 0L);  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  

    	MemoryStruct oDataChunk;  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &oDataChunk);

    	curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L); 
    	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 0L); // 1L for debug
    	curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str() ); 

    	curl_slist *pList = NULL;
    	pList = curl_slist_append(pList,"accept: application/json"); 
    	curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pList); 

    	res = curl_easy_perform(pCurl); 

    	long res_code=0;
    	res=curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &res_code);

   
    	if(( res == CURLE_OK ) && (res_code == 200 || res_code == 201))
    	{
    		struct json_object *root, *minersTotal, *nodes, *networkhashps, *difficulty, *dagSize, *epoch, *height;
		root = json_tokener_parse((char *)oDataChunk.memory);

		if (root != NULL) {
			minersTotal = json_object_object_get(root, "minersTotal");
			nodes = json_object_object_get(root, "nodes");
			if(nodes != NULL) {
				/*
				networkhashps = json_object_object_get(root, "networkhashps");
				difficulty = json_object_object_get(root, "difficulty");
				dagSize = json_object_object_get(root, "dagSize");
				printf("!!!!!!!!!!!networkhashps = %s\n", json_object_get_string(networkhashps));
				*/
				json_object *element = json_object_array_get_idx(nodes, 0);
    				networkhashps = json_object_object_get(element, "networkhashps");
				
				difficulty = json_object_object_get(element, "difficulty");
				dagSize = json_object_object_get(element, "dagSize");
				epoch = json_object_object_get(element, "epoch");
				height = json_object_object_get(element, "height");

				d_networkhashps = json_object_get_int64(networkhashps);
				d_difficulty = json_object_get_int64(difficulty);
				d_dagSize = json_object_get_int64(dagSize);
				d_height = json_object_get_int64(height);
				d_epoch = json_object_get_int(epoch);
				DoRediusWriteLong("networkhashps",d_networkhashps);
				DoRediusWriteLong("difficulty",d_difficulty);
				DoRediusWriteLong("dagSize",d_dagSize);
				DoRediusWriteLong("height",d_height);
				DoRediusWriteInt("epoch",d_epoch);

				//printf("!!!!!!!!!!!networkhashps = %s\n", json_object_get_string(networkhashps));
			}

			//printf("!!!!!!!!!!!minersTotal = %s\n", json_object_get_string(minersTotal));
		}
    	}

	curl_slist_free_all(pList); 
    	curl_easy_cleanup(pCurl);
    	curl_global_cleanup();

	return 0;
}

int GetAccountStatus(string walletID)
{
	time_t timep; 
	struct tm *p;
	int i;
	char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	char etc_buf[64];

	printf("=======GetAccountStatus========\n");

	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    	if(CURLE_OK != res)
    	{
		printf("curl init failed\n");
       	return 1;
    	}

    	CURL *pCurl = NULL;
    	pCurl = curl_easy_init();

    	if( NULL == pCurl)
    	{
    		printf("Init CURL failed...\n");
        	return -1;
    	}

    	string url = "https://etc.2miners.com/api/accounts/"+walletID; 
    	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 120L);
    	curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 60L); 
    	curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
    	curl_easy_setopt(pCurl, CURLOPT_HEADER, 0L);  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  

    	MemoryStruct oDataChunk;  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &oDataChunk);

    	curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L); 
    	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 0L); // 1L for debug
    	curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str() ); 

    	curl_slist *pList = NULL;
    	pList = curl_slist_append(pList,"accept: application/json"); 
    	curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pList); 

    	res = curl_easy_perform(pCurl); 

    	long res_code=0;
    	res=curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &res_code);

   
    	if(( res == CURLE_OK ) && (res_code == 200 || res_code == 201))
    	{
		struct json_object *root, *api_version, *worker_total, *worker_online, *worker_offline, *current_hashrate,  *average_hashrate;
		struct json_object *total_payment, *status, *paid, *payments;
		root = json_tokener_parse((char *)oDataChunk.memory);

		if (root != NULL) {
 			api_version = json_object_object_get(root, "apiVersion");
			
			worker_total = json_object_object_get(root, "workersTotal");
			worker_online = json_object_object_get(root, "workersOnline");
			worker_offline = json_object_object_get(root, "workersOffline");
			
			current_hashrate = json_object_object_get(root, "currentHashrate");
			average_hashrate = json_object_object_get(root, "hashrate");

			total_payment  = json_object_object_get(root, "paymentsTotal");

			status = json_object_object_get(root, "stats");
			if(status != NULL)
				paid = json_object_object_get(status, "paid");
			
			payments  = json_object_object_get(root, "payments");
			int digi_count = 0;
			long tmp_hash = json_object_get_int64(current_hashrate);

			 do
    			{
        			digi_count++;
        			tmp_hash /= 10;
    			}while(tmp_hash != 0);
			digi_count -= 1;
			time(&timep); 
			p=localtime(&timep); 
			printf("%04d/%02d/%02d\n",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday ); 
			printf("%s %02d:%02d:%02d\n", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);

			worksheet_write_number(worksheet, data_counter, 0, (1900+p->tm_year), NULL);
			worksheet_write_number(worksheet, data_counter, 1, (1+p->tm_mon), NULL);
			worksheet_write_number(worksheet, data_counter, 2, p->tm_mday, NULL);
			worksheet_write_number(worksheet, data_counter, 3, p->tm_hour, NULL);
			worksheet_write_number(worksheet, data_counter, 4, p->tm_min, NULL);
			worksheet_write_number(worksheet, data_counter, 5, p->tm_sec, NULL);
			
			printf("API version = %s\n", json_object_get_string(api_version));
			printf("Current Hashrate = %.4f %s\n", ((float)json_object_get_int64(current_hashrate)/divisor[digi_count/3]), hash_unit[digi_count/3]);
			digi_count = 0;
			tmp_hash = json_object_get_int64(average_hashrate);

			 do
    			{
        			digi_count++;
        			tmp_hash /= 10;
    			}while(tmp_hash != 0);
			 digi_count -= 1;
			printf("Average Hashrate = %.4f %s\n", ((float)json_object_get_int64(average_hashrate)/divisor[digi_count/3]), hash_unit[digi_count/3]);
			printf("Total Payments (Count) = %d\n", json_object_get_int(total_payment));
			if(paid != NULL)
				printf("Paid = %.6f\n", (float)json_object_get_int64(paid)/1000000000);
			printf("Worker : \n");
			printf("	Total=%s\n", json_object_get_string(worker_total));
			printf("	Online=%s\n", json_object_get_string(worker_online));
			printf("	Offline=%s\n", json_object_get_string(worker_offline));

			worksheet_write_number(worksheet, data_counter, 6, json_object_get_int64(average_hashrate), NULL);
			worksheet_write_number(worksheet, data_counter, 7, json_object_get_int64(current_hashrate), NULL);
			worksheet_write_number(worksheet, data_counter, 8, json_object_get_int(total_payment), NULL);
			worksheet_write_number(worksheet, data_counter, 9, json_object_get_int64(paid), NULL);
			worksheet_write_number(worksheet, data_counter, 10, json_object_get_int(worker_total), NULL);
			worksheet_write_number(worksheet, data_counter, 11, json_object_get_int(worker_online), NULL);
			worksheet_write_number(worksheet, data_counter, 12, json_object_get_int(worker_offline), NULL);
			
			worksheet_write_number(worksheet, data_counter, 13, d_networkhashps, NULL);
			worksheet_write_number(worksheet, data_counter, 14, d_difficulty, NULL);
			worksheet_write_number(worksheet, data_counter, 15, d_dagSize, NULL);
			worksheet_write_number(worksheet, data_counter, 16, d_height, NULL);
			worksheet_write_number(worksheet, data_counter, 17, d_epoch, NULL);
			
			DoRediusWriteLong("avghashrate",json_object_get_int64(average_hashrate));
			DoRediusWriteLong("currentHashrate",json_object_get_int64(current_hashrate));
			DoRediusWriteInt("workersOnline",json_object_get_int(worker_online));
			memset(etc_buf,0,64);
			sprintf(etc_buf,"%.4f",(14700/((float)d_networkhashps/1000000000000)/1000000)*((float)json_object_get_int64(average_hashrate)/1000000));
			DoRediusWriteETCEstimate(etc_buf);
			if(payments != NULL) {
				struct json_object *new_obj;
				struct json_object_iterator it;
				struct json_object_iterator itEnd;
				struct json_object *payments_array_obj,*amount, *timestamp, *txid;
				time_t timep; 
				struct tm *p;
				char time_buf[64];
				char amount_buf[32];
				for (i = 0; i < json_object_array_length(payments); i++)
				{
					payments_array_obj = json_object_array_get_idx(payments, i);
					amount = json_object_object_get(payments_array_obj, "amount");
					timestamp = json_object_object_get(payments_array_obj, "timestamp");
					txid = json_object_object_get(payments_array_obj, "tx");
					timep = atoi(json_object_get_string(timestamp));
					p=localtime(&timep); 
					/*
					printf("amount : %.4f\n",json_object_get_string(amount));
					printf("timestamp : %04d/%02d/%02d %02d:%02d:%02d\n",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
					printf("txid : %s\n",json_object_get_string(txid));
					*/
					memset(time_buf,0,64);
					memset(amount_buf,0,32);
					sprintf(time_buf,"%04d/%02d/%02d %02d:%02d:%02d",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
					sprintf(amount_buf,"%.4f\n", ((float)json_object_get_int64(amount)/1000000000));
					worksheet_write_string(worksheet_pay, i+1, 0, time_buf, NULL);
					worksheet_write_string(worksheet_pay, i+1, 1, amount_buf, NULL);
					worksheet_write_string(worksheet_pay, i+1, 2, json_object_get_string(txid), NULL);
					DoRediusWritePayouts((char *)json_object_get_string(timestamp),(char *)json_object_get_string(amount));
					DoRediusWritePayoutsTXid((char *)json_object_get_string(timestamp),(char *)json_object_get_string(txid));
				}
			}

			json_object_put(root);
		}
    	}
    	curl_slist_free_all(pList); 
    	curl_easy_cleanup(pCurl);
    	curl_global_cleanup();

	return 0;
}

int GetETCPrice(void)
{
	int i;
	printf("=======GetETCPrice========\n");

	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    	if(CURLE_OK != res)
    	{
		printf("curl init failed\n");
       	return 1;
    	}

    	CURL *pCurl = NULL;
    	pCurl = curl_easy_init();

    	if( NULL == pCurl)
    	{
    		printf("Init CURL failed...\n");
        	return -1;
    	}

	string url = "https://api.coingecko.com/api/v3/simple/price?ids=ethereum-classic&vs_currencies=usd&include_market_cap=false&include_24hr_vol=false&include_24hr_change=false&include_last_updated_at=false"; 
    	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 120L);
    	curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 60L); 
    	curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
    	curl_easy_setopt(pCurl, CURLOPT_HEADER, 0L);  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  

    	MemoryStruct oDataChunk;  
    	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &oDataChunk);

    	curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L); 
    	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 0L); // 1L for debug
    	curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str()); 

    	curl_slist *pList = NULL;
    	pList = curl_slist_append(pList,"accept: application/json"); 
    	curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pList); 

    	res = curl_easy_perform(pCurl); 

    	long res_code=0;
    	res=curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &res_code);

	if(( res == CURLE_OK ) && (res_code == 200 || res_code == 201))
    	{
		struct json_object *root, *etc_price, *amount;
		root = json_tokener_parse((char *)oDataChunk.memory);

		if (root != NULL) {
			etc_price  = json_object_object_get(root, "ethereum-classic");
			if(etc_price != NULL) {
				amount  = json_object_object_get(etc_price, "usd");
				//printf("%.4f / %s\n",(float)atof(json_object_to_json_string(amount)),json_object_to_json_string(amount));
				DoRediusWriteETCPrice((char *)json_object_to_json_string(amount));
			}
			json_object_put(root);
		}
	} else {
		printf("GetETCPrice Fail!!\n");
	}
	

	curl_slist_free_all(pList); 
    	curl_easy_cleanup(pCurl);
    	curl_global_cleanup();

	return 0;
}

int main() 
{
   	time_t timep; 
	struct tm *p;
	char file_path[256];

	if (signal(SIGUSR1, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGUSR1\n");
    	if (signal(SIGKILL, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGKILL\n");
    	if (signal(SIGSTOP, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGSTOP\n");
	if (signal(SIGHUP, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGHUP\n");
	if (signal(SIGINT, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGQUIT\n");

	DoRediusConnect();

	memset(file_path,0,256);
	time(&timep); 
	p=localtime(&timep); 
	sprintf(file_path,"miner_info_%04d-%02d-%02d-%02d-%02d-%02d.xlsx",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec); 
	
	setlocale(LC_ALL, "chs");
	

	//GetWorkertStatus("0x61a186b076dd7a16b3829f12b9823e29e64cb5b9");

	#if 1

	workbook  = workbook_new(file_path);
    	worksheet = workbook_add_worksheet(workbook, NULL);
	/* Change the column width for clarity. */
    	worksheet_set_column(worksheet, 0, 0, 20, NULL);
	worksheet_set_column(worksheet, 0, 1, 20, NULL);
	worksheet_set_column(worksheet, 0, 2, 20, NULL);
	worksheet_set_column(worksheet, 0, 3, 20, NULL);
	worksheet_set_column(worksheet, 0, 4, 20, NULL);
	worksheet_set_column(worksheet, 0, 5, 20, NULL);
	worksheet_set_column(worksheet, 0, 6, 20, NULL);
	worksheet_set_column(worksheet, 0, 7, 20, NULL);
	worksheet_set_column(worksheet, 0, 8, 20, NULL);
	worksheet_set_column(worksheet, 0, 9, 20, NULL);
	worksheet_set_column(worksheet, 0, 10, 20, NULL);
	worksheet_set_column(worksheet, 0, 11, 20, NULL);
	worksheet_set_column(worksheet, 0, 12, 20, NULL);
	worksheet_set_column(worksheet, 0, 13, 20, NULL);
	worksheet_set_column(worksheet, 0, 14, 20, NULL);
	worksheet_set_column(worksheet, 0, 15, 20, NULL);
	worksheet_set_column(worksheet, 0, 16, 20, NULL);
	worksheet_set_column(worksheet, 0, 17, 20, NULL);

	worksheet_write_string(worksheet, 0, 0, "Year", NULL);
	worksheet_write_string(worksheet, 0, 1, "Month", NULL);
	worksheet_write_string(worksheet, 0, 2, "Day", NULL);
	worksheet_write_string(worksheet, 0, 3, "Hour", NULL);
	worksheet_write_string(worksheet, 0, 4, "Minute", NULL);
	worksheet_write_string(worksheet, 0, 5, "Second", NULL);
	worksheet_write_string(worksheet, 0, 6, "Average Hashrate", NULL);
	worksheet_write_string(worksheet, 0, 7, "Current Hashrate", NULL);
	worksheet_write_string(worksheet, 0, 8, "Total Payment", NULL);
	worksheet_write_string(worksheet, 0, 9, "Paid", NULL);
	worksheet_write_string(worksheet, 0, 10, "Total Worker", NULL);
	worksheet_write_string(worksheet, 0, 11, "Online Worker", NULL);
	worksheet_write_string(worksheet, 0, 12, "Offline Worker", NULL);
	worksheet_write_string(worksheet, 0, 13, "Net Hashrate", NULL);
	worksheet_write_string(worksheet, 0, 14, "Difficulty", NULL);
	worksheet_write_string(worksheet, 0, 15, "DagSize", NULL);
	worksheet_write_string(worksheet, 0, 16, "Block Height", NULL);
	worksheet_write_string(worksheet, 0, 17, "Epoch", NULL);

	memset(file_path,0,256);
	time(&timep); 
	p=localtime(&timep); 
	sprintf(file_path,"deposit_%04d-%02d-%02d-%02d-%02d-%02d.xlsx",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec); 
	

	workbook_pay = workbook_new(file_path);
    	worksheet_pay = workbook_add_worksheet(workbook_pay, NULL);

	worksheet_set_column(worksheet_pay, 0, 0, 20, NULL);
	worksheet_set_column(worksheet_pay, 0, 1, 20, NULL);
	worksheet_set_column(worksheet_pay, 0, 2, 20, NULL);

	worksheet_write_string(worksheet_pay, 0, 0, "Time", NULL);
	worksheet_write_string(worksheet_pay, 0, 1, "Amount", NULL);
	worksheet_write_string(worksheet_pay, 0, 2, "TxID", NULL);

	/*
	printf("======ETC Network Status========\n");
	//GetETCNetworkStatus();
	printf("======Account Status========\n");
	GetAccountStatus("0x61a186b076dd7a16b3829f12b9823e29e64cb5b9");
	*/
	GetAllStatus();
	//GetAccountStatus("0x61a186b076dd7a16b3829f12b9823e29e64cb5b9");
	GetAccountStatus("0x4Fa6c0627be14D046Ae3838f373774111D27a757");
	GetETCPrice();
	data_counter++;

	while(is_run) {
		time(&timep); 
		p=localtime(&timep); 
		
		if(((p->tm_min % 5) == 0)&&(p->tm_sec == 0)) {
			GetAllStatus();
			//GetAccountStatus("0x61a186b076dd7a16b3829f12b9823e29e64cb5b9");
			GetAccountStatus("0x4Fa6c0627be14D046Ae3838f373774111D27a757");
			GetETCPrice();
			data_counter++;
		}

		if((p->tm_min == 0)&&(p->tm_sec == 0)) { //sync db per hour
			DoRediusSyncRDB();
		}
		
		sleep(1);
	}
	DoRediusSyncRDB();
	workbook_close(workbook);
	workbook_close(workbook_pay);

	#endif
	DoRediusDisconnect();
	printf("===========Leave Miner Info===========\n");
    	return 0;
}
