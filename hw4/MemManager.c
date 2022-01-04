#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define TLB_num 32

FILE*output_file;
    

struct node{
    char name[100];
    int frame;
    struct node*next;
}typedef Node;

struct FFL{
    int frame;
    int page;
    char process[2];
    struct FFL * next;
}typedef FFL;

struct TLBE{
    int VPN;
    int PFN;
    int valid;
    struct TLBE * next;
}typedef TLBE;

struct PTE{
    int frame;
    int dbi;
    int valid;
    int reference;
    int present;
}typedef PTE;

Node*cnode(char*name,int frame){
    Node*temp = malloc(sizeof(Node));
    memset(temp,0,sizeof(Node));
    strcpy(temp->name,name);
    temp->frame = frame;
    temp->next = NULL;
    return temp;
}

void addnode(Node**root,char*name,int frame){
    Node*temp = *root;
    if(!temp){
        *root = cnode(name,frame);
    }else{
        while(temp->next){
            temp = temp->next;
        }
        temp->next = cnode(name,frame);
    }
}

void pr_info(Node*root){
    while(root){
        printf("%s %d\n",root->name,root->frame);
        root = root->next;
    }
}

Node* get_trace(){
    FILE*trace = fopen("trace.txt", "r");
    char word;
    char name[10];
    char frame[10];
    int namecount = 0;
    int framecount = 0;
    int flag1 = 0;
    int flag2 = 0;
    Node*root = NULL;
    memset(name, 0, sizeof(name));
    while(word = getc(trace)){
        if(word == '\n' || word == EOF){
            addnode(&root,name,atoi(frame));
            namecount = 0;
            framecount = 0;
            memset(name, 0, sizeof(name));
            memset(frame, 0, sizeof(frame));
            if(word == EOF){
                break;
            }
            continue;
        }
        if(word == '('){
            flag1 = !flag1;
            continue;
        }
        if(word == ','){
            flag1 = !flag1;
            flag2 = !flag2;
            continue;
        }
        if(word == ')'){
            flag2 = !flag2;
            continue;
        }
        if(flag1){
            name[namecount++] = word;
        }
        if(flag2){
            frame[framecount++] = word;
        }
    }
    return root;
}

void get_sys_config(char*TLB_policy,char*page_policy,char*frame_policy,int*process_num,int*vir_num,int*phy_num){
    FILE*sys_config = fopen("sys_config.txt", "r");
    char word;
    int flag = 0;
    int counter = 0;
    int counter2 = 0;
    char input[10];
    memset(input, 0, sizeof(input));
    
    while(word = getc(sys_config)){
        if(word == ':'){
            flag = !flag;
            continue;
        }
        if(word == '\n' || word == EOF){
            flag = !flag;

            switch(counter2){
                case 0:
                    strcpy(TLB_policy,input);
                    counter2++;
                    break;
                case 1:
                    strcpy(page_policy,input);
                    counter2++;
                    break;
                case 2:
                    strcpy(frame_policy,input);
                    counter2++;
                    break;
                case 3:
                    *process_num = atoi(input);
                    counter2++;
                    break;
                case 4:
                    *vir_num = atoi(input);
                    counter2++;
                    break;
                case 5:
                    *phy_num = atoi(input);
                    counter2++;
                    break;
                default:
                    break;
            }
            
            counter = 0;
            memset(input,0,sizeof(input));
            if(word == EOF){
                return;
            }
            continue;
        }
        if(flag){ 
            
            if(word == ' '){
                continue;
            }
            input[counter++] = word;

        }
        
    }
}



void pr_TLB(TLBE * TLB){
    while(TLB){

        printf("%d,%d,%d\n",TLB->valid,TLB->VPN,TLB->PFN);
        TLB = TLB->next;
    }

}

void enqueue(FFL**root,FFL*new){

    FFL*temp = *root;
    if(!temp){
        *root = new;
    }else{
        while(temp->next){
            temp = temp->next;
        }
        temp->next = new;
    }
    new->next = NULL;

}

FFL*dequeue(FFL**root){
    FFL*temp = *root;
    if(temp){
        *root = temp->next;
        temp->next = NULL;
        return temp;
    }
    return NULL;
}

TLBE*make_TLB(){

    TLBE*root,*temp;
    root = temp = malloc(sizeof(TLBE));
    memset(temp, 0, sizeof(TLBE));
    for(int i = 1 ; i < 32 ; i++){

        temp->next = malloc(sizeof(TLBE));
        temp = temp->next;
        memset(temp, 0, sizeof(TLBE));
            
    }
    return root;

}

FFL* make_free_memory_list(int phy_num){

    FFL*root,*temp;
    root = temp = malloc(sizeof(FFL));
    temp->frame = 0;
    temp->next = NULL;
    for(int i = 1 ; i < phy_num ; i++){

        temp->next = malloc(sizeof(FFL));
        temp = temp->next;
        temp->frame = i;
        temp->next = NULL;
            
    }
    return root;
}

void flush_TLB(TLBE**TLB){
    TLBE*root = *TLB;
    while(root){
        root->valid = 0;
        root = root->next;
    }

}

int search_TLB(TLBE**TLB,int page,char*policy){
    TLBE*pre,*post;
    pre = post= *TLB;

    while(post){
        
        if(post->valid == 1){

            if(post->VPN == page){
                
                    
                if(strcmp(policy,"LRU") == 0){
                    if(post != pre){
                        pre->next = post->next;
                        post->next = *TLB;
                        *TLB = post;
                    }
                }
                
                return post->PFN;

            }

        }
        pre = post;
        post = post->next;
    }

    return -1;

}

int update_TLB(TLBE**TLB,int page,int frame,char * policy){
    
    TLBE*pre,*post;
    pre = post= *TLB;
    int TLB_full = 1;
    int vpn = 0;
    int x = 0;
    while(post){
        
        if(post->valid == 0){     
            
            post->valid = 1;
            post->VPN = page;
            post->PFN = frame;
            
            if(strcmp(policy,"LRU") == 0){
                if(post != pre){
                    pre->next = post->next;
                    post->next = *TLB;
                    *TLB = post;
                }
            }
            TLB_full = 0;
            break;

        }
        pre = post;
        post = post->next;
    }
    if(TLB_full == 1){
        if(strcmp(policy,"LRU") == 0){
            post = pre = *TLB;
            if(post){
                while(post->next){
                    pre = post;
                    post = post->next;
                }
            }
            pre->next = NULL;
            post->next = *TLB;
            *TLB = post;
            vpn = post->VPN;
            post->PFN = frame;
            post->VPN = page;
        }
        else if(strcmp(policy,"RANDOM") == 0){
            srand(time(NULL));
            x = rand() % 32;
            post = *TLB;
            while(x){
                post = post->next;
                x--;
            }
            vpn = post->VPN;
            post->VPN = page;
            post->PFN = frame;            
        }
        return vpn;


    }else{

        return -1;

    }
}

void invalid_TLB(TLBE**TLB,int page){
    
    TLBE*post= *TLB;

    while(post){
        
        if(post->valid == 1){

            if(post->VPN == page){
                    
                post->valid=0;

            }

        }
        post = post->next;
    }

}

void pr_FFL(FFL*root){

    while(root){
        fprintf(output_file,"%d,%c\n", root->frame,root->process[0]);
        
        printf("%d,%c\n", root->frame,root->process[0]);
        root = root->next;
    }
    
}

void page_out(){

}

int main(){
    char TLB_policy[10];
    char page_policy[10];
    char frame_policy[10];
    int process_num = 0;
    int vir_num = 0;
    int phy_num = 0;
    char cur_process[2] = " ";
    int time_counter = 0;
    int block_counter = 0;
    int frame_counter = 0;
    output_file = fopen("trace_output.txt","w");
    
    get_sys_config(TLB_policy,page_policy,frame_policy,&process_num,&vir_num,&phy_num);
    
    TLBE*TLB = make_TLB();
    PTE vir[process_num][vir_num];
    FFL* free_memory_list = make_free_memory_list(phy_num);
    FFL* global_victim_page = NULL;
    FFL* local_victim_page[process_num];
    int hit_num[process_num];
    int pagefault_num[process_num];
    memset(vir,0,sizeof(vir));
    memset(hit_num,0,sizeof(hit_num));
    memset(pagefault_num,0,sizeof(pagefault_num));
    
    for(int i = 0 ; i < process_num ; i++) {

        local_victim_page[i] = NULL;
        for(int j = 0 ; j < vir_num ; j++) {

            vir[i][j].dbi = -1;
        
        }

    }   

    Node*root = get_trace();
    /*
    pr_info(root);
    
    printf("%s\n%s\n%s\n%d\n%d\n%d\n",
    TLB_policy,
    page_policy,
    frame_policy,
    process_num,
    vir_num,
    phy_num);
    */
    //strcpy(TLB_policy,"RANDOM");
    flush_TLB(&TLB);

    while(root){

        PTE*page_table = vir[root->name[0] - 'A'];
        FFL*temp = NULL;
        int page = root->frame;
        int frame = 10;
        int evict_page;
        int dest;
        char evict_process;
        time_counter++;
        if(strcmp(cur_process,root->name) != 0){
            
            flush_TLB(&TLB);
            
        }

        strcpy(cur_process,root->name);
        //TLB miss
        if((frame = search_TLB(&TLB,page,TLB_policy)) == -1){
            //page Hit
            if(page_table[page].valid == 1 && page_table[page].present == 1){
                
                frame = page_table[page].frame;
                page_table[page].reference = 1;
                fprintf(output_file,"Process %c, TLB Miss, Page Hit, %d=>%d\n",cur_process[0],page,frame);
                printf("Process %c, TLB Miss, Page Hit, %d=>%d\n",cur_process[0],page,frame);

            }
            //page fault casue by invalid
            else if(page_table[page].valid == 0){
                pagefault_num[root->name[0] - 'A']++;
                if(free_memory_list){
                    
                    temp = dequeue(&free_memory_list);
                    //global victim page
                    temp->next = NULL;
                    if(strcmp(frame_policy,"GLOBAL") == 0){
                        enqueue(&global_victim_page,temp);
                    }else if(strcmp(frame_policy,"LOCAL") == 0){
                        enqueue(&(local_victim_page[root->name[0] - 'A']),temp);
                    }
                    //local victim page
                    frame = temp->frame;
                    temp->page = page;
                    temp->process[0] = root->name[0]; 

                    page_table[page].valid = 1;
                    page_table[page].reference = 1;
                    page_table[page].present = 1;
                    page_table[page].frame = frame;
                    fprintf(output_file,"Process %c, TLB Miss, Page Fault, %d, Evict -1 of Process %c to -1, %d<<-1\n",cur_process[0],frame,cur_process[0],page);
                    
                    printf("Process %c, TLB Miss, Page Fault, %d, Evict -1 of Process %c to -1, %d<<-1\n",cur_process[0],frame,cur_process[0],page);

                    /*
                    int frame;
                    int dbi;
                    int valid;
                    int reference;
                    int present;
                    */
                }else{
                    
                    if(strcmp(page_policy,"FIFO") == 0){
                        if(strcmp(frame_policy,"GLOBAL") == 0){
                            temp = dequeue(&global_victim_page);
                            temp->next = NULL;
                            enqueue(&global_victim_page,temp);
                            
                            frame = temp->frame;
                            vir[temp->process[0] - 'A'][temp->page].present = 0;
                            if(vir[temp->process[0] - 'A'][temp->page].dbi == -1){
                                vir[temp->process[0] - 'A'][temp->page].dbi = block_counter++;
                            }
                            
                            evict_page = temp->page;
                            evict_process = temp->process[0];
                            dest = vir[temp->process[0] - 'A'][temp->page].dbi;

                            if(temp->process[0] == cur_process[0]){

                                invalid_TLB(&TLB,page);

                            }

                            temp->process[0] = root->name[0];
                            temp->page = page;
                            
                            page_table[page].valid = 1;
                            page_table[page].reference = 1;
                            page_table[page].present = 1;
                            page_table[page].frame = frame;
                            fprintf(output_file,"Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<-1\n",cur_process[0],frame,evict_page,evict_process,dest,page);
                            printf("Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<-1\n",cur_process[0],frame,evict_page,evict_process,dest,page);
                    

                        }
                        else if(strcmp(frame_policy,"LOCAL") == 0){
                            
                            temp = dequeue(&(local_victim_page[root->name[0] - 'A']));
                            temp->next = NULL;
                            enqueue(&(local_victim_page[root->name[0] - 'A']),temp);
                        
                            frame = temp->frame;
                            vir[temp->process[0] - 'A'][temp->page].present = 0;
                            if(vir[temp->process[0] - 'A'][temp->page].dbi == -1){
                                vir[temp->process[0] - 'A'][temp->page].dbi = block_counter++;
                            }
                            
                            evict_page = temp->page;
                            evict_process = temp->process[0];
                            dest = vir[temp->process[0] - 'A'][temp->page].dbi;

                            

                            invalid_TLB(&TLB,page);

                            temp->process[0] = root->name[0];
                            temp->page = page;
                            
                            page_table[page].valid = 1;
                            page_table[page].reference = 1;
                            page_table[page].present = 1;
                            page_table[page].frame = frame;
                            fprintf(output_file,"Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<-1\n",cur_process[0],frame,evict_page,evict_process,dest,page);
                            printf("Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<-1\n",cur_process[0],frame,evict_page,evict_process,dest,page);
                    
                        }
                    }
                    else if(strcmp(page_policy,"CLOCK") == 0){

                        if(strcmp(frame_policy,"GLOBAL") == 0){

                        }
                        else if(strcmp(frame_policy,"LOCAL") == 0){

                        }

                    }
                    
                }
            }
            //page fault casue by page in disk
            else if(page_table[page].present == 0){
                pagefault_num[root->name[0] - 'A']++;
                if(strcmp(page_policy,"FIFO") == 0){
                    if(strcmp(frame_policy,"GLOBAL") == 0){
                        temp = dequeue(&global_victim_page);
                        temp->next = NULL;
                        enqueue(&global_victim_page,temp);
                        
                        frame = temp->frame;
                        vir[temp->process[0] - 'A'][temp->page].present = 0;
                        if(vir[temp->process[0] - 'A'][temp->page].dbi == -1){
                            vir[temp->process[0] - 'A'][temp->page].dbi = block_counter++;
                        }
                        
                        evict_page = temp->page;
                        evict_process = temp->process[0];
                        dest = vir[temp->process[0] - 'A'][temp->page].dbi;

                        if(temp->process[0] == cur_process[0]){

                            invalid_TLB(&TLB,page);

                        }

                        temp->process[0] = root->name[0];
                        temp->page = page;
                        
                        page_table[page].valid = 1;
                        page_table[page].reference = 1;
                        page_table[page].present = 1;
                        page_table[page].frame = frame;
                        fprintf(output_file,"Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<%d\n",cur_process[0],frame,evict_page,evict_process,dest,page,page_table[page].dbi);
                        printf("Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<%d\n",cur_process[0],frame,evict_page,evict_process,dest,page,page_table[page].dbi);
                        

                    }
                    else if(strcmp(frame_policy,"LOCAL") == 0){
                        temp = dequeue(&(local_victim_page[root->name[0] - 'A']));
                        temp->next = NULL;
                        enqueue(&(local_victim_page[root->name[0] - 'A']),temp);
                        
                        frame = temp->frame;
                        vir[temp->process[0] - 'A'][temp->page].present = 0;
                        if(vir[temp->process[0] - 'A'][temp->page].dbi == -1){
                            vir[temp->process[0] - 'A'][temp->page].dbi = block_counter++;
                        }
                        
                        evict_page = temp->page;
                        evict_process = temp->process[0];
                        dest = vir[temp->process[0] - 'A'][temp->page].dbi;

                        
                        invalid_TLB(&TLB,page);

                        
                        
                        temp->process[0] = root->name[0];
                        temp->page = page;
                        
                        page_table[page].valid = 1;
                        page_table[page].reference = 1;
                        page_table[page].present = 1;
                        page_table[page].frame = frame;
                        fprintf(output_file,"Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<%d\n",cur_process[0],frame,evict_page,evict_process,dest,page,page_table[page].dbi);
                        printf("Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<%d\n",cur_process[0],frame,evict_page,evict_process,dest,page,page_table[page].dbi);
                        
                    }
                }
                else if(strcmp(page_policy,"CLOCK") == 0){

                    if(strcmp(frame_policy,"GLOBAL") == 0){

                    }
                    else if(strcmp(frame_policy,"LOCAL") == 0){

                    }

                }
                    
            }
            update_TLB(&TLB,page,frame,TLB_policy);
            

        }
        //TLB hit
        else{
            hit_num[root->name[0] - 'A']++;
            fprintf(output_file,"Process %c,TLB Hit,%d=>%d\n",root->name[0],page,frame);
            printf("Process %c,TLB Hit,%d=>%d\n",root->name[0],page,frame);
            page_table[page].reference = 1;
            root = root->next;
        }

        

    }

    
    FILE *fp = fopen("analysis.txt","w");

    for(int i = 0 ; i < process_num ; i++){
        
        double hit_rate = ((double)hit_num[i])/((double)time_counter);
        double pagefault_rate = ((double)pagefault_num[i])/((double)time_counter);


        fprintf(fp,"Process %c, Effective Access Time = %.3f\n",i+'A',(hit_rate*120 + (1-hit_rate)*220));
        fprintf(fp,"Process %c, Page Fault Rate = %.3f\n",i+'A',pagefault_rate);
        

    }

    pr_TLB(TLB);
    /*
    while(root){
        //printf("%s",root->name);
        time_counter++;
        PTE*page_table = vir[root->name[0] - 'A'];
        FFL*temp = NULL;
        short TLB_hit = 0;
        short page_hit = 0;
        int page = root->frame;
        int frame = -1;
        int min = 2000000000;
        int min_idx = 0;
        int TLB_full = 1;
        //Flush TLB
        if(strcmp(cur_process,root->name) != 0){
            
            memset(TLB,0,sizeof(TLB)); 

        }

        strcpy(cur_process,root->name);
            
        for(int i = 0 ; i < TLB_num ; i++){

            
            if(TLB[i].valid){
                
                if(TLB[i].VPN == page){
                    
                    frame = TLB[i].PFN;
                    TLB[i].time = time_counter;
                    TLB_hit = 1;
                    
                    break;

                }

            }

        }
        //TLB hit
        if(TLB_hit){
            printf("Process %c,TLB Hit,%d=>%d\n",root->name[0],page,frame);
            root = root->next;
        }
        //TLB miss
        else{
            //page hit
            if(page_table[page].valid && page_table[page].present){
                
                page_hit = 1;
                frame = page_table[page].frame;
                page_table[page].reference = 1;
                page_table[page].present = 1;
                
                printf("Process %c TLB miss,Page hit %d => %d\n",root->name[0],page,frame);
            }
            //page fault
            else{
                //page fault cause by invalid
                if(!page_table[page].valid){
                    //free memory list is not empty
                    if(free_memory_list){
                        temp = dequeue(&free_memory_list);
                        frame = temp->frame;
                        page_table[page].frame = temp->frame;
                        strcpy(temp->process,cur_process);
                        temp->page = page;
                        enqueue(&global_victim_page,temp);
                        page_table[page].valid = 1;
                        page_table[page].time = time_counter;
                        page_table[page].reference = 1;
                        page_table[page].present = 1;
                        printf("Process %c, TLB Miss, Page Fault, %d, Evict -1 of Process %c to -1, %d<<-1\n",
                        root->name[0],
                        root->frame,
                        root->name[0],
                        page
                        );
                    //free memory list is empty
                    }else{
                        
                        //FIFO
                        if(strcmp(page_policy,"FIFO") == 0){
                            
                            temp = dequeue(&global_victim_page);
                            vir[temp->process[0] - 'A'][temp->page].present = 0;
                            vir[temp->process[0] - 'A'][temp->page].reference = 0;
                            temp->process[0] = cur_process[0];
                            temp->page = page;
                            enqueue(&global_victim_page,temp);
                            frame = temp->frame;
                            page_table[page].frame = temp->frame;
                            page_table[page].valid = 1;
                            page_table[page].time = time_counter;
                            page_table[page].reference = 1;
                            page_table[page].present = 1;
                            
                        }
                        //Clock
                        else{

                        }
                        printf("Process %c, TLB Miss, Page Fault, %d, Evict %d of Process %c to %d, %d<<-1\n",
                        root->name[0],

                        root->frame,

                        root->name[0],
                        page
                        );
                    }
                //page fault cause by page in disk
                }else{
                    temp = dequeue(&global_victim_page);
                    if(vir[temp->process[0] - 'A'][temp->page].dbi == -1){
                        vir[temp->process[0] - 'A'][temp->page].dbi = block_counter;
                        block_counter++;
                    }
                    vir[temp->process[0] - 'A'][temp->page].present = 0;
                    vir[temp->process[0] - 'A'][temp->page].reference = 0;
                    temp->process[0] = cur_process[0];
                    temp->page = page;
                    enqueue(&global_victim_page,temp);
                    frame = temp->frame;
                    page_table[page].frame = temp->frame;
                    page_table[page].valid = 1;
                    page_table[page].time = time_counter;
                    page_table[page].reference = 1;
                    page_table[page].present = 1;
                        
                            
                }
                    
            }
            

            // Update TLB
            for(int i = 0 ; i < TLB_num ; i++){
                            
                if(!TLB[i].valid){
                    TLB[i].valid = 1;
                    TLB[i].VPN = page;
                    TLB[i].PFN = frame;
                    TLB[i].time = time_counter;
                    TLB_full = 0;
                    break;
                }

            }
            if(TLB_full){
                //LRU
                if(strcmp(TLB_policy,"LRU") == 0){
                    for(int i = 0 ; i < TLB_num ; i++){
                    
                        if(min > TLB[i].time){
                            
                            min = TLB[i].time;
                            min_idx = i;

                        }

                    }
                    TLB[min_idx].VPN = page;
                    TLB[min_idx].PFN = frame;
                    TLB[min_idx].valid = 1;
                    TLB[min_idx].time = time_counter;
                
                }
                //random
                else{

                    int x = rand() % 32;
                    TLB[x].VPN = page;
                    TLB[x].PFN = frame;
                    TLB[x].valid = 1;
                    TLB[x].time = time_counter;
                    
                }
            
            }
            
        }   
        //root = root->next;
            
    }
    //pr_TLB(TLB);
    
    */
    return 0;
    
}