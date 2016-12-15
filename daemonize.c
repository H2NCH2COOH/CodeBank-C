
static void daemonize(const char* pid_file)
{
    int i;
    pid_t pid;
    char str[1024];
    
    pid=fork();
    if(pid<0)
    {
        Log(LOG_ERR,"Failed fork() with ret=%d, error=%s\n",pid,strerror(errno));
        exit(1);
    }
    if(pid>0)
    {
        exit(0);
    }

    if(setsid()<0)
    {
        Log(LOG_ERR,"Failed setsid() with error=%s\n",strerror(errno));
        exit(1);
    }

    closelog();
    
    for(i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */

    i=open("/dev/null",O_RDWR); /* open stdin */
    dup(i); /* stdout */
    dup(i); /* stderr */

    open_log();
    
    /* Change the file mode mask */
    umask(0);

    /* Change the current working directory */
    if(chdir("/")<0)
    {
        Log(LOG_ERR,"Failed chdir() with error=%s",strerror(errno));
        exit(1);
    }

    /* Ensure only one copy */
    pid_fd=open(pid_file,O_RDWR|O_CREAT|O_CLOEXEC,0600);

    if(pid_fd==-1)
    {
        /* Couldn't open lock file */
        Log(LOG_ERR,"Could not open PID lock file %s, exiting",pid_file);
        exit(1);
    }

    /* Try to lock file */
    if (lockf(pid_fd,F_TLOCK,0)==-1)
    {
        /* Couldn't get lock on lock file */
        Log(LOG_ERR,"Could not lock PID lock file %s, exiting",pid_file);
        exit(1);
    }

    /* Get and format PID */
    sprintf(str,"%d\n",getpid());

    /* write pid to lockfile */
    write(pid_fd,str,strlen(str));
}
