/* 
 * tsh - A tiny shell program with job control
 * 
 * idealism-xxm
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

// 包装函数，处理错误情况
pid_t Fork();
void Sigfillset(sigset_t *);
void Sigemptyset(sigset_t *);
void Sigaddset(sigset_t *, int);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldsest);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	        break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	        break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	        break;
	    default:
            usage();
	    }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

        /* Read command line */
        if (emit_prompt) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin)) { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    // 存储本次命令行的命令和参数
    char *argv[MAXARGS];

    // 判断是否是后台作业
    // 框架已经完成了 parseline ，我们直接调用解析即可
    int is_bg = parseline(cmdline, argv);
    // 忽略空行
    if (argv[0] == NULL) {
        return;
    }

    // 执行内置命令，如果是内置命令，则直接返回
    if (builtin_cmd(argv)) {
        return;
    }

    // 先阻塞 SIGCHLD 信号
    sigset_t mask_one, prev_one;
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);
    Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

    // 子进程 PID
    pid_t pid;
    // 执行可执行文件
    if ((pid = Fork()) == 0) {
        // 子进程继承了父进程的被阻塞集合，先解除阻塞
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
        // 子进程重新开一个进程组，方便后续将 `ctrl-c` (`ctrl-z`)
        // 引起的 `SIGINT` (`SIGTSTP`) 信号传递给前台作业及其所有子进程
        if (setpgid(0, 0) < 0) {
           printf("setpgid error\n");
           exit(0);
        }
        // 子进程使用 execve 运行可执行文件
        if (execve(argv[0], argv, environ) < 0) {
            printf("tsh: command not found: %s\n", argv[0]);
            exit(0);
        }
        // execve 只有失败才会返回，所以成功了不会执行后面的语句，
        // 会执行可执行文件的主函数
    }
    // 这里开始必定是父进程才会执行

    // 子进程添加至作业列表中
    int state = is_bg ? BG : FG;
    addjob(jobs, pid, state, cmdline);
    // 恢复原本的被阻塞集合
    Sigprocmask(SIG_SETMASK, &prev_one, NULL);
    if (is_bg) {
        // 后台作业需要输出提示信息
        int jid = pid2jid(pid);
        printf("[%d] (%d) %s", jid, pid, cmdline);
    } else {
        // 前台作业需要阻塞至子进程结束
        waitfg(pid);
    }
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	    buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
        buf++;
        delim = strchr(buf, '\'');
    }
    else {
	    delim = strchr(buf, ' ');
    }

    while (delim) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'') {
            buf++;
            delim = strchr(buf, '\'');
        }
        else {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	    return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	    argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    // 识别到 quiz 内置命令，直接终止 shell
    if (!strcmp(argv[0], "quit")) {
        exit(0);
    }
    // 识别到 jobs 内置命令，执行 listjobs 后返回 1
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    // 识别到 bg <job> 或者 fg <job> 内置命令，执行 do_bgfg 后返回 1
    if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }

    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) {
    // 校验参数
    if (argv[1] == NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    struct job_t *job;
    int id;
    if (sscanf(argv[1], "%%%d", &id) > 0) {
        // 获取 jid 成功，然后获取对应的作业
         job = getjobjid(jobs, id);
         if (job == NULL) {
            printf("%%%d: No such job\n", id);
            return;
         }
    } else if (sscanf(argv[1], "%d", &id) > 0) {
        // 获取 pid 成功，然后获取对应的作业
        job = getjobjid(jobs, id);
        if (job == NULL) {
            printf("(%d): No such process\n", id);
            return;
        }
    } else {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    // 不存在，则直接返回
    if (job == NULL) {
        return;
    }

    // 修改状态，然后给子进程所在的进程组发送 SIGCONT 信号唤醒
    int is_bg = 0;
    if (!strcmp(argv[0], "bg")) {
        is_bg = 1;
    }
    job -> state = is_bg ? BG : FG;
    kill(-(job -> pid), SIGCONT);

    // 标记是否唤醒到后台作业
    if (is_bg) {
        // 后台作业则需要输出相关信息
        printf("[%d] (%d) %s", job -> jid, job -> pid, job -> cmdline);
    } else {
        // 前台作业则需要等到其运行完成
        waitfg(job -> pid);
    }
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    // 当作业列表中的前台作业是 pid ，则先睡 1s
    while(fgpid(jobs) == pid) {
        sleep(1);
        // 书中 P545 提到使用 sigsuspend 是一个更合适的解决方法
    }
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) {
    // 保留原本的错误码，结束调用时恢复
    int olderrno = errno;
    int status;
    pid_t pid;
    // 由于没有其他信号处理程序会使用 jobs ，所以无需阻塞信号

    // 处理所有终止或者停止的子进程，如果没有立即返回
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        // 正常退出，直接从作业列表删除
        if (WIFEXITED(status)) {
            deletejob(jobs, pid);
        }
        // 由于未捕获的信号终止，先打印相关信息，再从作业列表删除
        if (WIFSIGNALED(status)) {
            int jid = pid2jid(pid);
            int signal = WTERMSIG(status);
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, signal);
            deletejob(jobs, pid);
        }
        // 由于信号被停止，先打印相关信息，再将作业状态更改为 ST
        if (WIFSTOPPED(status)) {
            struct job_t *job = getjobpid(jobs, pid);
            int signal = WSTOPSIG(status);
            printf("Job [%d] (%d) stopped by signal %d\n", job -> jid, pid, signal);
            job -> state = ST;
        }
    }
    // 使用 WNOHANG 选项时，如果还有子进程存在，那么就会返回 0
    // 这种情况不算错误
    if (pid != 0 && errno != ECHILD) {
        unix_error("waitpid error");
    }

    // 恢复错误码
    errno = olderrno;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) {
    // 获取前台作业的 PID
    pid_t pid = fgpid(jobs);
    // 对前台作业所在进程组发送 `SIGINT` 信号
    if (pid) {
        kill(-pid, sig);
    }
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) {
    // 获取前台作业的 PID
    pid_t pid = fgpid(jobs);
    // 对前台作业所在进程组发送 SIGTSTP 信号
    if (pid) {
        kill(-pid, sig);
    }
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	    return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if(verbose){
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	    return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs)+1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid != 0) {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state) {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjobs: Internal error: job[%d].state=%d ",
                   i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    Sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	    unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}

pid_t Fork() {
    pid_t pid;
    if ((pid = fork()) < 0) {
        unix_error("fork error");
    }
    return pid;
}

void Sigfillset(sigset_t *set) {
    if (sigfillset(set) < 0) {
        unix_error("sigfillset error");
    }
}

void Sigemptyset(sigset_t *set) {
    if (sigemptyset(set) < 0) {
        unix_error("sigemptyset error");
    }
}

void Sigaddset(sigset_t *set, int num) {
    if (sigaddset(set, num) < 0) {
        unix_error("sigaddset error");
    }
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldsest) {
    if (sigprocmask(how, set, oldsest) < 0) {
        unix_error("sigprocmask error");
    }
}
