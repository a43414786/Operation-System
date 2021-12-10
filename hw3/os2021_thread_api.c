#include "os2021_thread_api.h"

struct itimerval Signaltimer;
ucontext_t dispatch_context;
ucontext_t timer_context;

typedef struct thread_status{
    char name[20];
    char function[20];
    int priority_init;
    int priority_cur;
    int cancelmode;
    int cancelsig;
    int pid;
    int queueing_time;
    int waiting_time;
    char state[10];
    ucontext_t ctx;
    struct thread_status *front;
    struct thread_status *next;
}Thread;

Thread*readyf[3] = {NULL, NULL, NULL};
Thread*readyr[3] = {NULL, NULL, NULL};
Thread*time_waitingf[3] = {NULL, NULL, NULL};
Thread*time_waitingr[3] = {NULL, NULL, NULL};
Thread*event_waitingf[3] = {NULL, NULL, NULL};
Thread*event_waitingr[3] = {NULL, NULL, NULL};
Thread*runnning = NULL;
Thread*terminate = NULL;

int pid_counter = 1;

void enqueue(Thread**front,Thread**rear,Thread*input){
    printf("d");
    Thread *f = *front;
    input->front = input->next = NULL;
    if(!f){
        *front = input;
        *rear = input;
        return;
    }
    input->next = f;
    f->front = input;
    *front = input;
}

Thread *dequeue(Thread**front,Thread**rear){
    Thread *r = *rear;
    if(!r){
        return NULL;
    }
    Thread *temp = r->front;
    r->front = NULL;
    r->next = NULL;
    if(!temp){
        *front = NULL;
        *rear = NULL;
    }else{
        temp->next = NULL;
        *rear = temp;
    }
    return r;
}

void getthreads(){
    FILE*fp=fopen("init_threads.json","r");
    Thread*root = NULL;
    Thread*temp = NULL;
    Thread*thread_root = NULL;
    char input[20];
    char inputs[20][4];
    int input_flag[4] = {0,0,0,0};
    char word;
    int flag = 0;
    int counter = 0;
    int input_counter = 0;
    memset(input,0,sizeof(input));
    memset(inputs,0,sizeof(inputs));
    while((word = getc(fp)) != EOF){
        if(word == '\"'){
            if(input[0]){
                if(!strcmp(input,"name")){
                    input_counter++;
                    input_flag[0] = !input_flag[0];
                }else if(!strcmp(input,"entry function")){
                    input_flag[1] = !input_flag[1];
                    input_counter++;
                }else if(!strcmp(input,"priority")){
                    input_flag[2] = !input_flag[2];
                    input_counter++;
                }else if(!strcmp(input,"cancel mode")){
                    input_flag[3] = !input_flag[3];
                    input_counter++;
                }else{
                    if(input_flag[0]){
                        input_flag[0] = !input_flag[0];
                        strcpy(inputs[0],input);
                    }else if(input_flag[1]){
                        input_flag[1] = !input_flag[1];
                        strcpy(inputs[1],input);
                    }else if(input_flag[2]){
                        input_flag[2] = !input_flag[2];
                        strcpy(inputs[2],input);
                    }else if(input_flag[3]){
                        input_flag[3] = !input_flag[3];
                        strcpy(inputs[3],input);
                    }
                }
            }
            flag = !flag;
            memset(input,0,sizeof(input));
            counter = 0;
            continue;
        }
        if(input_counter == 4){
            input_counter = 0;
            switch(inputs[2][0]){
                case 'H':
                    OS2021_ThreadCreate(inputs[0],inputs[1],2,inputs[3][0] - '0');
                    break;
                case 'M':
                    OS2021_ThreadCreate(inputs[0],inputs[1],1,inputs[3][0] - '0');
                    break;
                case 'L':
                    OS2021_ThreadCreate(inputs[0],inputs[1],0,inputs[3][0] - '0');
                    break;    
            }
            memset(inputs,0,sizeof(inputs));
        }
        if(flag){
            input[counter++] = word;
        }
    }
    return;
}

void pr_info(Thread *temp){
    while(temp){ 
        char b,c;
        switch (temp->priority_cur)
        {
        case 0:
            c = 'L';
            break;
        
        case 1:
            c = 'M';
            break;
        
        case 2:
            c = 'H';
            break;
        default:
            break;
        }
        switch (temp->priority_init)
        {
        case 0:
            b = 'L';
            break;
        
        case 1:
            b = 'M';
            break;
        
        case 2:
            b = 'H';
            break;
        default:
            break;
        }
        printf("*\t%d\t%s\t\t%s\t%c\t\t%c\t\t%d\t%d\t*\n",temp->pid,temp->name,temp->state,b,c,temp->queueing_time,temp->waiting_time);
        temp = temp->next;
    }
}

void show_info(){
    puts("\n****************************************************************************************");
    puts("*\tTID\tName\t\tState\tB_Priority\tC_Priority\tQ_Time\tW_Time\t*");
    pr_info(runnning);
    pr_info(readyf[0]);
    pr_info(readyf[1]);
    pr_info(readyf[2]);   
    pr_info(time_waitingf[0]);
    pr_info(time_waitingf[1]);
    pr_info(time_waitingf[2]);
    pr_info(event_waitingf[0]);
    pr_info(event_waitingf[1]);
    pr_info(event_waitingf[2]);   
    puts("****************************************************************************************");
    
}

void handler(){
    if(!runnning) return;
    Thread *temp = runnning;
    runnning = NULL;
    enqueue(&(readyf[2]),&(readyr[2]),temp);
    swapcontext(&(temp->ctx),&dispatch_context);
}

void fu1(){
    while(1){
        printf("fun1");
    }
}
void fu2(){
    while(1){
        printf("fun2");
    }
}
void fu3(){
    while(1){
        printf("fun3");
    }
}

int OS2021_ThreadCreate(char *job_name, char *p_function, int priority, int cancel_mode)
{   
    printf("a");
    if(strcmp(p_function,"Function1") && strcmp(p_function,"Function2") && strcmp(p_function,"Function3") && strcmp(p_function,"Function4") && strcmp(p_function,"Function5") && strcmp(p_function,"ResourceReclaim"))
    {
        return -1;
    }
    Thread *temp = malloc(sizeof(Thread));
    memset(temp,0,sizeof(Thread));
    strcpy(temp->name,job_name);
    strcpy(temp->function,p_function);
    temp->priority_init = priority;
    temp->priority_cur = priority;
    temp->cancelmode = cancel_mode;
    temp->cancelsig = 0;
    temp->front = NULL;
    temp->next = NULL;
    temp->pid = pid_counter++;
    temp->queueing_time = 0;
    temp->waiting_time = 0;
    strcpy(temp->state,"READY");
    
    enqueue(&(readyf[priority]),&(readyr[priority]),temp);
    
    if(pid_counter%3 == 0){
        CreateContext(&(temp->ctx),&dispatch_context,&fu1);
    }
    else if(pid_counter%3 == 1){
        CreateContext(&(temp->ctx),&dispatch_context,&fu2);
    }
    else if(pid_counter%3 == 2){
        CreateContext(&(temp->ctx),&dispatch_context,&fu3);
    }
    return pid_counter;
}

void OS2021_ThreadCancel(char *job_name)
{
    
}

void OS2021_ThreadWaitEvent(int event_id)
{

}

void OS2021_ThreadSetEvent(int event_id)
{

}

void OS2021_ThreadWaitTime(int msec)
{

}

void OS2021_DeallocateThreadResource()
{

}

void OS2021_TestCancel()
{
    Thread *temp = NULL;
    temp = runnning;
    if(temp->cancelsig){
        runnning = NULL;
        free(temp);
    }
}

void CreateContext(ucontext_t *context, ucontext_t *next_context, void *func)
{
    getcontext(context);
    context->uc_stack.ss_sp = malloc(STACK_SIZE);
    context->uc_stack.ss_size = STACK_SIZE;
    context->uc_stack.ss_flags = 0;
    context->uc_link = next_context;
    makecontext(context,(void (*)(void))func,0);
}

void ResetTimer()
{
    Signaltimer.it_value.tv_sec = 0;
    Signaltimer.it_value.tv_usec = 10000;
    if(setitimer(ITIMER_REAL, &Signaltimer, NULL) < 0)
    {
        printf("ERROR SETTING TIME SIGALRM!\n");
    }
}

void Dispatcher()
{
    getcontext(&dispatch_context);

    Signaltimer.it_interval.tv_usec = 10000;
    Signaltimer.it_interval.tv_sec = 0;
    ResetTimer();
    
    Thread*temp = NULL;

    OS2021_ThreadCreate("reclaimer","ResourceReclaim",0,1);

    getthreads();

    while(1){
        temp = dequeue(&(readyf[2]),&(readyr[2]));
        runnning = temp;
        swapcontext(&dispatch_context,&(temp->ctx));
    }
}

void StartSchedulingSimulation()
{
    /*Set Timer*/
    Signaltimer.it_interval.tv_usec = 10000;
    Signaltimer.it_interval.tv_sec = 0;
    ResetTimer();
    /*Create Context*/
    CreateContext(&dispatch_context, &timer_context, &Dispatcher);
    setcontext(&dispatch_context);
}
