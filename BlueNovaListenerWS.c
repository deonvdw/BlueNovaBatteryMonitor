// gcc -g -l pgeasy -l pq -lpthread -lwebsockets -L/usr/local/lib -I/usr/local/include -o bluenovalistener BlueNovaListenerWS.c
// Requires libwebsockets web socket library to be installled - https://libwebsockets.org/

#define	UPDATEFREQ	10		/* update every 10 seconds */

#include <libwebsockets.h>

#ifdef WIN32
#define	_CRT_SECURE_NO_WARNINGS
#define _CRT_NO_POSIX_ERROR_CODES
#include <windows.h>

#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>	
#endif

#ifndef WIN32
#define	POSTGRES
#endif

#ifdef POSTGRES
#include <libpq-fe.h>
#include <libpgeasy.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

int		retry_times[]= {1,1,2,2,3,3,5,5,10,10,30,60};
int		retry_time;
int		retry_index;

FILE	*LogFile=	NULL;
char	*LogFilename= "-";
int		LogSize= 0;
int		LogMaxSize= 1024*1024*50;	// 50 MB
int		LogMaxFiles= 5;
int		FlushLogFlag= 0;
int		lograw= 0;

unsigned char gethexvalue(char ch)
{
 return ((ch>='0')&&(ch<='9'))?ch-'0':((ch>='A')&&(ch<='F'))?ch-'A'+10:((ch>='a')&&(ch<='f'))?ch-'a'+10:0;
}

void hextobin (char *hexstr, unsigned char *outbuf, int bufsize, int* outlen)
{
 unsigned char	value;
 int    	dummy;
 
 if (!outlen)
  outlen= &dummy;

 *outlen= 0;
 while ((bufsize--)>0)
 {
  while (*hexstr==' ')
   hexstr++;
  if (*hexstr==0)
   break;
  value= gethexvalue(*(hexstr++))<<4;

  while (*hexstr==' ')
   hexstr++;
  if (*hexstr==0)
   break;
  value|= gethexvalue(*(hexstr++));
  *(outbuf++)= value;
  (*outlen)++;
 }
}

#ifdef WIN32

int gettimeofday (struct timeval *tv, void *Dummy)
{
 ULARGE_INTEGER	ft;	// 64-bit value, number of 100-nanosecond intervals since January 1, 1601 (UTC). 

 GetSystemTimeAsFileTime((FILETIME*)&ft);
 ft.QuadPart= (ft.QuadPart-116444736000000000i64)/10;
 tv->tv_sec= (long) (ft.QuadPart/1000000);
 tv->tv_usec=  (long) (ft.QuadPart%1000000);
 return 0;
}

#endif

void WriteLog (char *Format,...)
{
 va_list    args;

 char		buffer[4096];
 int		nwritten;
 int		i;

 char		old_name[256];
 char		new_name[256];
 
 struct tm		*tm;
 struct timeval	tv;
 time_t			now;
 
 if ((LogSize>=LogMaxSize)&&LogMaxSize)
 {
  if (LogFile)
   fclose (LogFile);
  LogFile= NULL;

  snprintf (old_name,sizeof(old_name),LogMaxFiles>1?"%s.%d":"%s",LogFilename,LogMaxFiles-2);
  unlink (old_name);

  for (i=LogMaxFiles-2;i>0;)
  {
   snprintf (new_name,sizeof(new_name),"%s.%d",LogFilename,i);
   snprintf (old_name,sizeof(old_name),"%s.%d",LogFilename,--i);
   rename (old_name,new_name);
  }
  if (LogMaxFiles>1)
   rename (LogFilename,old_name);
  LogSize= 0;
 }
 
 if (!LogFile)
 {
  LogFile= fopen (LogFilename,"w");
  if (!LogFile)
   return;
 }
 
 if (gettimeofday(&tv,NULL)==-1)
  memset (&tv,0,sizeof(tv));   /* Error! */
 now= tv.tv_sec;
 tm= localtime (&now);

 snprintf (buffer,sizeof(buffer),"%04d/%02d/%02d %02d:%02d:%02d ",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
 va_start (args,Format);
 i= strlen(buffer); 
 vsnprintf (buffer+i,sizeof(buffer)-1,Format,args);
 va_end(args);

 nwritten= fprintf(LogFile,"%s",buffer);
 if (nwritten>=0)
  LogSize+= nwritten;

 if (FlushLogFlag)
   fflush (LogFile);
}

/*
*create table battery_pack
*(
* INSERT_TIME_UTC   TIMESTAMP,
* MCU_ID            BIGINT,
* N_UNITS           SMALLINT,
* UNIT_RANK         SMALLINT,
* CELL_COUNT        SMALLINT,
* CELL_01_V         REAL,
* CELL_02_V         REAL,
* CELL_03_V         REAL,
* CELL_04_V         REAL,
* CELL_05_V         REAL,
* CELL_06_V         REAL,
* CELL_07_V         REAL,
* CELL_08_V         REAL,
* CELL_09_V         REAL,
* CELL_10_V         REAL,
* CELL_11_V         REAL,
* CELL_12_V         REAL,
* CELL_13_V         REAL,
* CELL_14_V         REAL,
* CELL_15_V         REAL,
* CELL_16_V         REAL,
* CELL_01_T         SMALLINT,
* CELL_02_T         SMALLINT,
* CELL_03_T         SMALLINT,
* CELL_04_T         SMALLINT,
* CELL_05_T         SMALLINT,
* CELL_06_T         SMALLINT,
* CELL_07_T         SMALLINT,
* CELL_08_T         SMALLINT,
* CELL_09_T         SMALLINT,
* CELL_10_T         SMALLINT,
* CELL_11_T         SMALLINT,
* CELL_12_T         SMALLINT,
* CELL_13_T         SMALLINT,
* CELL_14_T         SMALLINT,
* CELL_15_T         SMALLINT,
* CELL_16_T         SMALLINT,
* V_PACK            REAL,
* I_PACK            REAL,
* TEMP              SMALLINT,
* V_CELL_MIN        REAL,
* V_CELL_MAX        REAL,
* SOC               SMALLINT,
* HEALTH            REAL,
* ERRORS            VARCHAR(32),
* SYSTEM_STATE      VARCHAR(32),
* BREAKER           VARCHAR(32),
* CHARGE_STATE      VARCHAR(16),
* ENERGY_IN         REAL,
* ENERGY_OUT        REAL,
* ADC_VP            REAL,
* ADC_0             REAL,
* ADC_1             REAL,
* ADC_2             REAL,
* ADC_3             REAL,
* ADC_4             REAL,
* BAD_CRC           SMALLINT,
* PARSE_FAIL        SMALLINT,
* BAD_VCELL         SMALLINT
*);
*create index battery_pack_time on battery_pack(insert_time_utc);
*/

struct
{
 unsigned int	mcu_id;
 int			num_units;	
 int			unit_rank;
 int			cell_count;
 double			cell_01_v;
 double			cell_02_v;
 double			cell_03_v;
 double			cell_04_v;
 double			cell_05_v;
 double			cell_06_v;
 double			cell_07_v;
 double			cell_08_v;
 double			cell_09_v;
 double			cell_10_v;
 double			cell_11_v;
 double			cell_12_v;
 double			cell_13_v;
 double			cell_14_v;
 double			cell_15_v;
 double			cell_16_v;
 int			cell_01_t;
 int			cell_02_t;
 int			cell_03_t;
 int			cell_04_t;
 int			cell_05_t;
 int			cell_06_t;
 int			cell_07_t;
 int			cell_08_t;
 int			cell_09_t;
 int			cell_10_t;
 int			cell_11_t;
 int			cell_12_t;
 int			cell_13_t;
 int			cell_14_t;
 int			cell_15_t;
 int			cell_16_t;
 double			v_pack;
 double			i_pack;
 int			temp;
 double			v_cell_min;
 double			v_cell_max;
 int			soc;
 double			health;
 char			errors[33];
 char			system_state[33];
 char			breaker[33];
 char			charge_state[17];
 double			adc_vp;
 double			adc0;
 double			adc1;
 double			adc2;
 double			adc3;
 double			adc4;
}	db;

struct
{
 int			bad_crc;
 int			parse_fail;
 int			bad_vcell;
}	db2;

struct
{
 double			energy_in;
 double			energy_out;
}	db3;

#ifdef POSTGRES
 PGconn    *pgconn= NULL;
#endif
 char    *DatabaseName = "homeauto";
 char    *DatabaseUser = "homeauto";

void WriteRecordToDB (void)
{
 time_t	timestamp;
#ifdef POSTGRES
 char    pgoptions[2048];
 char    statement[4096];
 int	len;
 PGresult	*pgres;
#endif

 timestamp= time(NULL);
		   
 WriteLog (" == Write record: Time: %u, MCU ID %08X, NumUnit %d, MyRank %d, CellCount %d, Vpack %.3lf, Ipack %.1lf, Temp %d, Vmin_cell %.3lf,"
		   " Vmax_cell %.3lf, SOC %d, Health %.3lf, Errors '%s', State '%s', Breaker '%s', Charge '%s'\n",
		   timestamp,db.mcu_id,db.num_units,db.unit_rank,db.cell_count,db.v_pack,db.i_pack,db.temp,db.v_cell_min,db.v_cell_max,db.soc,
		   db.health,db.errors,db.system_state,db.breaker,db.charge_state);
 WriteLog (" -- ADCvp %.3lf, ADC0 %.3lf, ADC1 %.3lf, ADC2 %.3lf, ADC3 %.3lf, ADC4 %.3lf, Bad CRC %d, ParseFail %d, Bad Vcell %d, EnergyIn %.3lf, EnergyOut %.3lf\n",
		   db.adc_vp,db.adc0,db.adc1,db.adc2,db.adc3,db.adc4,db2.bad_crc,db2.parse_fail,db2.bad_vcell,db3.energy_in,db3.energy_out);
		   
 WriteLog (" -- Volt: %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf %.3lf\n",
			db.cell_01_v,db.cell_02_v,db.cell_03_v,db.cell_04_v,db.cell_05_v,db.cell_06_v,db.cell_07_v,db.cell_08_v,
			db.cell_09_v,db.cell_10_v,db.cell_11_v,db.cell_12_v,db.cell_13_v,db.cell_14_v,db.cell_15_v,db.cell_16_v);
 WriteLog (" -- Temp: %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d %-5d\n",
			db.cell_01_t,db.cell_02_t,db.cell_03_t,db.cell_04_t,db.cell_05_t,db.cell_06_t,db.cell_07_t,db.cell_08_t,
			db.cell_09_t,db.cell_10_t,db.cell_11_t,db.cell_12_t,db.cell_13_t,db.cell_14_t,db.cell_15_t,db.cell_16_t);

 if (db.mcu_id==0xFFFFFFFF)
  return;

 if (!pgconn)
 {
  on_error_continue();
  snprintf (pgoptions,sizeof(pgoptions),"dbname=%s user=%s",DatabaseName,DatabaseUser);
  pgconn= connectdb(pgoptions);
 
  if (PQstatus(pgconn) != CONNECTION_OK)
  {
   WriteLog ("Error connecting to postgresql database: %s\n", PQerrorMessage(pgconn));
   disconnectdb();
   pgconn= NULL;
   return;
  }
  else
  {  
   WriteLog ("PostgreSQL database connection OK\n");
   pgres= doquery("SET TIME ZONE 'UTC';");
   if (PQresultStatus(pgres) != PGRES_COMMAND_OK)
   {
    WriteLog ("PostgreSQL error setting timezone to UTC: %s\n",PQresultErrorMessage(pgres));
    disconnectdb();
    pgconn= NULL;
   }
  }
 }

 len= snprintf(statement,sizeof(statement),"INSERT into battery_pack(INSERT_TIME_UTC,MCU_ID,N_UNITS,UNIT_RANK,CELL_COUNT,CELL_01_V,CELL_02_V,"
			"CELL_03_V,CELL_04_V,CELL_05_V,CELL_06_V,CELL_07_V,CELL_08_V,CELL_09_V,CELL_10_V,CELL_11_V,CELL_12_V,CELL_13_V,CELL_14_V,"
			"CELL_15_V,CELL_16_V,CELL_01_T,CELL_02_T,CELL_03_T,CELL_04_T,CELL_05_T,CELL_06_T,CELL_07_T,CELL_08_T,CELL_09_T,CELL_10_T,"
			"CELL_11_T,CELL_12_T,CELL_13_T,CELL_14_T,CELL_15_T,CELL_16_T,V_PACK,I_PACK,TEMP,V_CELL_MIN,V_CELL_MAX,SOC,HEALTH,ERRORS,"
			"SYSTEM_STATE,BREAKER,CHARGE_STATE,ENERGY_IN,ENERGY_OUT,ADC_VP,ADC_0,ADC_1,ADC_2,ADC_3,ADC_4,BAD_CRC,PARSE_FAIL,BAD_VCELL)"
			" VALUES(to_timestamp(%u)",timestamp);

 len+= snprintf(statement+len,sizeof(statement)-len,",%u",db.mcu_id);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.num_units?",NULL":",%u",db.num_units);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.unit_rank?",NULL":",%u",db.unit_rank);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_count?",NULL":",%u",db.cell_count);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_01_v)?",NULL":",%.3lf",db.cell_01_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_02_v)?",NULL":",%.3lf",db.cell_02_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_03_v)?",NULL":",%.3lf",db.cell_03_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_04_v)?",NULL":",%.3lf",db.cell_04_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_05_v)?",NULL":",%.3lf",db.cell_05_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_06_v)?",NULL":",%.3lf",db.cell_06_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_07_v)?",NULL":",%.3lf",db.cell_07_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_08_v)?",NULL":",%.3lf",db.cell_08_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_09_v)?",NULL":",%.3lf",db.cell_09_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_10_v)?",NULL":",%.3lf",db.cell_10_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_11_v)?",NULL":",%.3lf",db.cell_11_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_12_v)?",NULL":",%.3lf",db.cell_12_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_13_v)?",NULL":",%.3lf",db.cell_13_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_14_v)?",NULL":",%.3lf",db.cell_14_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_15_v)?",NULL":",%.3lf",db.cell_15_v);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.cell_16_v)?",NULL":",%.3lf",db.cell_16_v);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_01_t?",NULL":",%u",db.cell_01_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_02_t?",NULL":",%u",db.cell_02_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_03_t?",NULL":",%u",db.cell_03_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_04_t?",NULL":",%u",db.cell_04_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_05_t?",NULL":",%u",db.cell_05_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_06_t?",NULL":",%u",db.cell_06_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_07_t?",NULL":",%u",db.cell_07_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_08_t?",NULL":",%u",db.cell_08_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_09_t?",NULL":",%u",db.cell_09_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_10_t?",NULL":",%u",db.cell_10_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_11_t?",NULL":",%u",db.cell_11_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_12_t?",NULL":",%u",db.cell_12_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_13_t?",NULL":",%u",db.cell_13_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_14_t?",NULL":",%u",db.cell_14_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_15_t?",NULL":",%u",db.cell_15_t);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.cell_16_t?",NULL":",%u",db.cell_16_t);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.v_pack)?",NULL":",%.3lf",db.v_pack);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.i_pack)?",NULL":",%.2lf",db.i_pack);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.temp?",NULL":",%u",db.temp);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.v_cell_min)?",NULL":",%.3lf",db.v_cell_min);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.v_cell_max)?",NULL":",%.3lf",db.v_cell_max);
 len+= snprintf(statement+len,sizeof(statement)-len,!~db.soc?",NULL":",%u",db.soc);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.health)?",NULL":",%.3lf",db.health);
 len+= snprintf(statement+len,sizeof(statement)-len,!~*db.errors?",NULL":",'%s'",db.errors);
 len+= snprintf(statement+len,sizeof(statement)-len,!~*db.system_state?",NULL":",'%s'",db.system_state);
 len+= snprintf(statement+len,sizeof(statement)-len,!~*db.breaker?",NULL":",'%s'",db.breaker);
 len+= snprintf(statement+len,sizeof(statement)-len,!~*db.charge_state?",NULL":",'%s'",db.charge_state);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db3.energy_in)?",NULL":",%.3lf",db3.energy_in);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db3.energy_out)?",NULL":",%.3lf",db3.energy_out);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.adc_vp)?",NULL":",%.3lf",db.adc_vp);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.adc0)?",NULL":",%.4lf",db.adc0);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.adc1)?",NULL":",%.4lf",db.adc1);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.adc2)?",NULL":",%.4lf",db.adc2);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.adc3)?",NULL":",%.4lf",db.adc3);
 len+= snprintf(statement+len,sizeof(statement)-len,isnan(db.adc4)?",NULL":",%.4lf",db.adc4);
 len+= snprintf(statement+len,sizeof(statement)-len,",%u,%u,%u",db2.bad_crc,db2.parse_fail,db2.bad_vcell);
 snprintf(statement+len,sizeof(statement)-len,");");

 pgres= doquery(statement);
 if (PQresultStatus(pgres) != PGRES_COMMAND_OK)
 {
  WriteLog ("PostgreSQL INSERT error %s\n",PQresultErrorMessage(pgres));
  disconnectdb();
  pgconn= NULL;
 }
}

time_t	lastupdate= 0;
int		updatefreq= UPDATEFREQ;
int		energy_sm= 0;

#define	FIND_END_PREFIX(m,s)	(strncmp(m,s,sizeof(s)-1)?NULL:m+sizeof(s)-1)
#define	STRCOPY_NT(d,s)			{ strncpy(d,s,sizeof(d)-1); (d)[sizeof(d)-1]=0; }

void handle_message (const char *rxmessage)
{
 const char *s;
 char		*s2;
 
 if (lograw) WriteLog ("RX: %s\n",rxmessage);

 if ((s=FIND_END_PREFIX(rxmessage,"MCU ID: "))) 							{ db.mcu_id= strtoul(s,NULL,16); }
 if ((s=FIND_END_PREFIX(rxmessage,"--- PO PARAMS --- (i=")))				{ db.num_units= atoi(s); }
 if ((s=FIND_END_PREFIX(rxmessage,"after assignRank() --> own rank = "))) 	{ db.unit_rank= atoi(s); }
 if ((s=FIND_END_PREFIX(rxmessage,"CellCount: ")))							{ db.cell_count= atoi(s); }

 if ((s=FIND_END_PREFIX(rxmessage,"C01) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_01_t,&db.cell_01_v); db.cell_01_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C02) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_02_t,&db.cell_02_v); db.cell_02_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C03) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_03_t,&db.cell_03_v); db.cell_03_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C04) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_04_t,&db.cell_04_v); db.cell_04_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C05) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_05_t,&db.cell_05_v); db.cell_05_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C06) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_06_t,&db.cell_06_v); db.cell_06_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C07) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_07_t,&db.cell_07_v); db.cell_07_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C08) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_08_t,&db.cell_08_v); db.cell_08_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C09) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_09_t,&db.cell_09_v); db.cell_09_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C10) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_10_t,&db.cell_10_v); db.cell_10_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C11) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_11_t,&db.cell_11_v); db.cell_11_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C12) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_12_t,&db.cell_12_v); db.cell_12_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C13) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_13_t,&db.cell_13_v); db.cell_13_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C14) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_14_t,&db.cell_14_v); db.cell_14_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C15) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_15_t,&db.cell_15_v); db.cell_15_v/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"C16) T: "))) { sscanf (s,"%d, V: %lf",&db.cell_16_t,&db.cell_16_v); db.cell_16_v/= 1000; }

 if ((s=FIND_END_PREFIX(rxmessage,"Vpack: ")))								{ db.v_pack= atoi(s)/1000.0; }
 if ((s=FIND_END_PREFIX(rxmessage,"Current: ")))							{ sscanf(s,"%lf",&db.i_pack); }
 if ((s=FIND_END_PREFIX(rxmessage,"Temp: ")))								{ db.temp= atoi(s); }
 if ((s=FIND_END_PREFIX(rxmessage,"Vc_max: ")))								{ db.v_cell_max= atoi(s)/1000.0; }
 if ((s=FIND_END_PREFIX(rxmessage,"Vc_min: ")))								{ db.v_cell_min= atoi(s)/1000.0; }
 if ((s=FIND_END_PREFIX(rxmessage,"SOC: ")))								{ db.soc= atoi(s); }
 if ((s=FIND_END_PREFIX(rxmessage,"SOH: ")))								{ sscanf(s,"%lf",&db.health); }

 if ((s=FIND_END_PREFIX(rxmessage,"Errors: \"")))							{ STRCOPY_NT(db.errors,s); if ((s2=strchr(db.errors,'"'))) *s2= 0; }
 if ((s=FIND_END_PREFIX(rxmessage,"System_State: ")))						{ STRCOPY_NT(db.system_state,s); }
 if ((s=FIND_END_PREFIX(rxmessage,"Breaker: ")))							{ STRCOPY_NT(db.breaker,s); }
 if ((s=FIND_END_PREFIX(rxmessage,"Charge State: ")))						{ STRCOPY_NT(db.charge_state,s); if ((s2=strchr(db.charge_state,' '))) *s2= 0; }

 if ((s=FIND_END_PREFIX(rxmessage,"Vp_adc: ")))								{ db.adc_vp= atoi(s)/1000.0; }
 if ((s=FIND_END_PREFIX(rxmessage,"DEBUG: ADC[0]: ")))						{ sscanf(s,"%*d = %lf",&db.adc0); db.adc0/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"DEBUG: ADC[1]: ")))						{ sscanf(s,"%*d = %lf",&db.adc1); db.adc1/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"DEBUG: ADC[2]: ")))						{ sscanf(s,"%*d = %lf",&db.adc2); db.adc2/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"DEBUG: ADC[3]: ")))						{ sscanf(s,"%*d = %lf",&db.adc3); db.adc3/= 1000; }
 if ((s=FIND_END_PREFIX(rxmessage,"DEBUG: ADC[4]: ")))						{ sscanf(s,"%*d = %lf",&db.adc4); db.adc4/= 1000; }

 if (FIND_END_PREFIX(rxmessage,"INFO: BAD CRC"))							{ db2.bad_crc++; }
 if (FIND_END_PREFIX(rxmessage,"INFO: cellcomms parse FAIL"))				{ db2.parse_fail++; }
 if (FIND_END_PREFIX(rxmessage,"DEBUG: REJECT BAD "))						{ db2.bad_vcell++; }

 if (FIND_END_PREFIX(rxmessage,"DEBUG: CALC PACK_CHARGE, STORE IN FLASH"))	{ energy_sm= 1; }
 if ((s=FIND_END_PREFIX(rxmessage,"Write EnergyOut: ")))					{ if (energy_sm==1) sscanf(s,"%lf",&db3.energy_out); if ((energy_sm++)==2) sscanf(s,"%lf",&db3.energy_in); } 

 if ((time(NULL)-lastupdate)>updatefreq)
 {
  lastupdate= (time(NULL)/updatefreq)*updatefreq;
  WriteRecordToDB();
  memset (&db,0xFF,sizeof(db));
  memset (&db2,0x00,sizeof(db2));
 }
}

static struct lws *client_wsi;

static int wscallback (struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
 switch (reason)
 {
  /* because we are protocols[0] ... */
  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			WriteLog ("CLIENT_CONNECTION_ERROR: %s\n",in?(char *)in:"(null)");
			client_wsi= NULL;
			break;

  case LWS_CALLBACK_CLIENT_ESTABLISHED:
			WriteLog ("Websocket connection established\n");
			retry_index= 0;
			break;

  case LWS_CALLBACK_CLIENT_RECEIVE:
			if (in)
			 handle_message((const char *)in);
			break;

  case LWS_CALLBACK_CLIENT_CLOSED:
			client_wsi= NULL;
			break;

  default:	break;
 }
 return lws_callback_http_dummy(wsi,reason,user,in,len);
}

static const struct lws_protocols wsprotocols[]=
{
 {"",wscallback,0,0},
 {NULL,NULL,0,0}
};

int main(int argc, char **argv)
{
 char	*BatteryIP= "10.0.0.2";
 int    Count;
 
 struct lws_context_creation_info	ccinfo;
 struct lws_client_connect_info		conninfo;
 struct lws_context					*context;

 struct stat        statbuf;

 int	rc;
	
#ifndef WIN32
 int	daemon = 0;
 pid_t	pid;
 pid_t	sid;
#endif

 for (Count = 1; Count < argc;)
 {
  if (!strcmp(argv[Count],"-b"))
  {
   BatteryIP= ((++Count) < argc) ? argv[Count++] : "";
   if (!*BatteryIP)
   {
	printf("No Battery hostname specified\n");
    return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count],"-d"))
  {
   DatabaseName= ((++Count) < argc) ? argv[Count++] : "";
   if (!*DatabaseName)
   {
	printf("No database name specified\n");
    return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count],"-u"))
  {
   DatabaseUser= ((++Count) < argc) ? argv[Count++] : "";
   if (!*DatabaseUser)
   {
	printf("No database user name specified\n");
    return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count],"-l"))
  {
   LogFilename= ((++Count) < argc) ? argv[Count++] : "";
   if (!*LogFilename)
   {
	printf("No logfile name specified\n");
    return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count],"-n"))
  {
   LogMaxFiles= ((++Count) < argc) ? atoi(argv[Count++]) : 0;
   if (LogMaxFiles <= 0)
   {
    printf("Invalid maximum number of log files %d\n", LogMaxFiles);
	return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count],"-m"))
  {
   LogMaxSize= ((++Count) < argc) ? atoi(argv[Count++]) : 0;
   if (LogMaxSize < 0)
   {
    printf("Invalid maximum file size %d\n", LogMaxSize);
	return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count],"-f"))
  {
   FlushLogFlag= ((++Count) < argc) ? atoi(argv[Count++]) : 0;
   if ((FlushLogFlag < 0) || (FlushLogFlag > 1))
   {
    printf("Log flush flag %d, flag must be 0 or 1\n",FlushLogFlag);
	return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count],"-i"))
  {
   updatefreq= ((++Count) < argc) ? atoi(argv[Count++]) : 0;
   if (updatefreq < 10)
   {
    printf("Update interval must be 10 seconds or longer\n",updatefreq);
	return 1;
   }
   continue;
  }

  if (!strcmp(argv[Count], "-r"))
  {
   lograw= 1;
   Count++;
   continue;
  }

#ifndef WIN32
  if (!strcmp(argv[Count], "-daemon"))
  {
   daemon= 1;
   Count++;
   continue;
  }
#endif

  printf("Unknown option '%s'\n", argv[Count++]);
 }

 printf("Connecting to Battery %s using database %s and logfile %s\n", BatteryIP,DatabaseName,LogFilename);

#ifndef WIN32
 if (daemon)
 {
  /* Fork off the parent process */
  pid = fork();
  if (pid < 0)
  {
   printf ("fork() error %d\n",errno);
   return 3;
  }
  /* If we got a good PID, then we can exit the parent process. */
  if (pid > 0)
   return 0;

  umask(0);
  chdir("/");
  }
#endif

 if (!strcmp(LogFilename, "-"))
 {
  LogFile= stdout;
  LogMaxSize= 0;
 }
 else
 {
  LogSize= LogMaxSize+1;
  if (!stat(LogFilename,&statbuf))
   LogSize= statbuf.st_size;
 }
 WriteLog ("BMSListnerMQTT v0.25 successfully started, using Battery IP %s and database name %s\n",BatteryIP,DatabaseName);

#ifndef WIN32
 if (daemon)
 {
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);
  stdin= fopen("/dev/null","rw");
  stdout= fopen("/dev/null","rw");
  stderr= fopen("/dev/null","rw");

  /* Create a new SID for the child process */
  sid= setsid();
  if (sid<0)
  {
   WriteLog ("setsid() error %d\n",errno);
   return 3;
  }
 }
#endif

 // signal(SIGINT, sigint_handler);
 // Main websocket loop
 while (1)
 {
  // Init LWS
  memset(&ccinfo,0,sizeof(ccinfo));
  ccinfo.options= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  ccinfo.port= CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
  ccinfo.protocols= wsprotocols;
  ccinfo.fd_limit_per_thread = 15;	/* was 3, add extra for postgres? */
  context= lws_create_context(&ccinfo);
  if (!context)
  {
   WriteLog ("lws init failed\n");
   return 5;
  }
  // Connect to web socket
  client_wsi= NULL;
  memset(&conninfo,0,sizeof(conninfo));
  conninfo.context= context;
  conninfo.port= 9000;
  conninfo.address= BatteryIP;
  conninfo.path = "/ws/";
  conninfo.host = conninfo.address;
  conninfo.origin = conninfo.address;
  conninfo.ssl_connection = 0;
  conninfo.protocol = wsprotocols[0].name;
  conninfo.pwsi= &client_wsi;
  lws_client_connect_via_info(&conninfo);
  // Read loop
  memset (&db,0xFF,sizeof(db));
  memset (&db2,0x00,sizeof(db2));
  memset (&db3,0xFF,sizeof(db3));
  lastupdate= (time(NULL)/updatefreq+1)*updatefreq;
  while (client_wsi)
  {
   rc= lws_service(context,0);
   if (rc<0)
   {
    WriteLog ("lws_service error %d\n",rc);
    break;
   }
  }
  lws_context_destroy(context);
  // Get retry sleep time
  retry_time= retry_times[retry_index];
  if (retry_index<((sizeof(retry_times)/sizeof(*retry_times))-1))
   retry_index++;
  WriteLog ("Websocket connection broken, attempting to reconnect in %d seconds\n",retry_time);
  sleep (retry_time);
 }

#ifdef POSTGRES
 disconnectdb();
#endif
 
 if (LogFile!=stdout)
  fclose (LogFile);
 return 0;
}
