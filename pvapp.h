//***************************************************************************
//                          pvapp.h  -  description
//                             -------------------
//  begin            : ter mai 7 07:25:59 2024
//  generated by     : pvdevelop (C) Lehrig Software Engineering
//  email            : lehrig@t-online.de
//***************************************************************************
#ifndef _PVAPP_H_
#define _PVAPP_H_

#ifdef USE_INETD
static int trace=0; // todo: set trace=0 if you do not want printf() within event loop
#else
static int trace=1; // todo: set trace=0 if you do not want printf() within event loop
#endif

#include "processviewserver.h"
// todo: comment me out
#include "rlmodbusclient.h"
//#include "rlsiemenstcpclient.h"
//#include "rlppiclient.h"
#include "modbusdaemon.h"             // this is generated. Change for name.h -> "name.mkmodbus"
//#include "siemensdaemon.h"            // this is generated
//#include "ppidaemon.h"                // this is generated

int show_mask1(PARAM *p);

//***************************************************************************
//                                 mainloop.h
//                                 ----------
//  O código a seguir foi incluído diretamente em "pvapp.h" por esta ser uma 
//  aplicação pequena.
//  Em projetos maiores, será mais adequado que seja criado um arquivo a parte
//  com o nome "mainloop.h".
//  O objetivo de mainloop é criar uma thread que permita executar lógicas 
//  independentes das masks usando o estilo dos programas para Arduino com
//  setup() e loop().
// 
#ifndef _MAINLOOP_
#define _MAINLOOP_

// GLOBAL

#include "stdlib.h"
#include "rlthread.h"
#include "rltime.h"
#include "qtdatabase.h"

/////////////////////////////////////////////
#ifndef _MAIN_  // Código exclusivo das Masks
/////////////////////////////////////////////
extern rlMutex    dbmutex;
extern qtDatabase db;


/////////////////////////////////////////////////////////////////
#else // Aqui começa o código que vai ser compilado na seção Main
/////////////////////////////////////////////////////////////////

rlModbusClient     modbus(modbusdaemon_MAILBOX,modbusdaemon_SHARED_MEMORY,modbusdaemon_SHARED_MEMORY_SIZE);

#include <QCoreApplication>

rlThread   usrThread;
rlMutex    dbmutex;
qtDatabase db;
static const char *tableName="REMOTA";
static char buf[16384];

static void task01()
{
	static int i = 0, day0 = 0;
	int j;
	rlTime HMS;

	//~ printf("Task01 : %d\n",i);
	HMS.getLocalTime();
	i=sprintf(buf,"insert %s values (now(3)",tableName);
	for(j=0; j<8; j++)
		i+=sprintf(&buf[i],",%g", modbus.readShort(modbusdaemon_CYCLE3_BASE,j)*20.0/20000);
	i+=sprintf(&buf[i],")"); buf[i]=0;
	dbmutex.lock();
	db.dbQuery(buf);
	if(HMS.day!=day0) 
	{
		day0=HMS.day; 
		sprintf(buf,"delete from %s where t < (now() - interval 3 week)",tableName); 
		db.dbQuery(buf);
	}
	dbmutex.unlock();

	i++;
};

static void task02()
{
 static int i = 0;
 i++;
};

static void setup()
{
	int i,j,n;

	dbmutex.lock();
	db.open("QMYSQL","localhost","pvdb","","");

	i=sprintf(buf,"create table if not exists %s (t timestamp(3)",tableName);
	for (j=0; j<8; j++) i+=sprintf(&buf[i],",AI%d float", j+1);
	i+=sprintf(&buf[i],");"); buf[i]=0;
	db.dbQuery(buf);
	i=sprintf(buf,"create index if not exists idx_%s on %s(t);",tableName, tableName);
        i=sprintf(buf,"select * from %s limit 1;",tableName);
        db.dbQuery(buf);
        dbmutex.unlock();
        n=db.result->record().count()-1;
        printf("records:%d\n",n);
    
};

#define TZ 100

static void loop()
{
	static int i=0;
	
	if(!(i%10)) task01(); // Chamar task01 a cada 1 segundo
	if(!(i%30)) task02(); // Chamar task02 a cada 3 segundos

	i++;
	pvSleep(TZ); // TZ = 100 ms
};

static void *usrMain(void *arg)
{
 setup();
 while(1) loop();
 return arg;
};

////////////////////////////////////////////////////////////////////////

#endif

#endif

// FIM DE _MAINLOOP_
//
//***************************************************************************


#endif