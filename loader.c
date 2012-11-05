#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>

#include <string.h>

int Game_Main(const char  *path, const char *udn __attribute__ ((unused)))
{
	char szlogfile[256];
	FILE *msg;

	strcpy( szlogfile, path );
	strcat( szlogfile, "time_limit.log" );

	msg = fopen( szlogfile, "a+" );

	unsigned *handle=NULL;
	if ( (handle = dlopen(NULL,RTLD_LAZY | RTLD_GLOBAL)) == NULL )
	{
		fputs("Cannot open self!\n",msg);
		fputs (dlerror(), msg);
		fclose(msg);
		return 1;
	}

	unsigned *pPowerOff  = dlsym(handle,"_Z10OSPowerOffv");
	unsigned *pEpochTime = dlsym(handle,"_ZN6PCTime9EpochTimeEv");
  
	if(pPowerOff == NULL)
	{
		fputs("Cannot find _Z10OSPowerOffv function!\n",msg);
		fputs(dlerror(), msg);
		fputs("\n",msg);
		fclose(msg);
		return 1;
	}

	if(pEpochTime == NULL)
	{
		fputs("Cannot find _ZN6PCTime9EpochTimeEv functions\n",msg);
		fputs(dlerror(), msg);
		fputs("\n",msg);
		fclose(msg);
		return 1;
	}

	dlclose(handle);

	/* Load time_limit library */
	char filename[64];
	strcpy(filename, path);
	strcat(filename,"time_limit.so");
	if ( (handle = dlopen( filename, RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE)) == NULL )
	{
		fputs("Cannot open time_limit.so!\n",msg);
		fputs (dlerror(), msg);
		fputs("\n",msg);
		fclose(msg);
		return 1;
	}

	unsigned *time_limit_thread = dlsym(handle,"time_limit_thread");
	
	dlclose(handle);

        time_t current_time = ((time_t(*)())pEpochTime)();
//        struct tm * ptm = localtime(&current_time);
//        char buffer[32];
//        strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);
//        fprintf( msg, "Current time: %s\n", buffer);
//        printf( "Current time: %s\n", buffer);

	if ( time_limit_thread ) {
		pthread_t	helper;
		pthread_create( &helper, NULL, (void *)time_limit_thread, (void*)current_time );
                fclose( msg );
		return 0;
	} else {
		fprintf(msg, "time_limit_thread NOT found..\n");
	}

	fclose( msg );

	return 1;
}
