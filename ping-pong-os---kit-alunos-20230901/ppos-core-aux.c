#define _XOPEN_SOURCE 700 //habilitar o struct sigaction
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos_data.h"


// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções

#include <signal.h>
#include <sys/time.h>


#define QUANTUM 20; //cada tarefa de usuario tem um quantum de 20ms 


// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;

// estrutura de inicialização to timer
struct itimerval timer;


/*
Esta função ajusta aprioridade com base no tempo de execução total estimado para cada tarefa. 
Caso task seja nulo, ajusta a prioridade da tarefa atual. Quando a tarefa já está em execução,
essa função deve sobrescrever tanto o valor estimado do tempo de execução como também 
o valor do tempo que ainda resta para a tarefa terminar sua execução.
*/
void task_set_eet (task_t *task, int et){
    //se task for nulo, então taskExec tem seu tempo estimado e restante modificados
    if(task == NULL){
        taskExec->tempoEstimado = et;
        taskExec->tempoRestante = et - taskExec->running_time;
    }
    else {
        task->tempoEstimado = et;
        //o tempo restante é o tempo estimado menos o tempo que a tarefa já rodou na cpu
        task->tempoRestante = et - task->running_time;
    }
}


/*
Esta função devolve o valor do tempo
restante para terminar a execução da tarefa task (ou da tarefa corrente, se task for nulo).*/
int task_get_ret(task_t *task){
    if(task == NULL){
        return taskExec->tempoRestante;
    }
    else{
        return task->tempoRestante;
    }
}


//Esta função devolve o valor do tempo
//estimado de execução da tarefa task (ou da tarefa corrente, se task for nulo).
int task_get_eet(task_t *task){
    if(task == NULL)
        return taskExec->tempoEstimado;
    else
        return task->tempoEstimado;
}


/*
Analisa a fila de tarefas prontas, devolvendo um ponteiro para a
próxima tarefa a receber o processador*/
task_t * scheduler() {
    
    if (readyQueue == NULL) {
        return NULL;
    }
    
    //tarefa auxiliar para iterar a fila circular
    task_t *tarefaAux = readyQueue;

    //ponteiro para a proxima tarefa a executar
    task_t *proximaTarefa = readyQueue;

    int menorTempoRestante;
    int tempoRestanteAtual;
    
    //o menor tempo restante, primeiramente, é o tempo 
    //restante da primeira tarefa da fila
    menorTempoRestante = task_get_ret(proximaTarefa);

    

    while(tarefaAux->next != readyQueue){

        tarefaAux = tarefaAux->next;

        tempoRestanteAtual = task_get_ret(tarefaAux);

        if(tempoRestanteAtual < menorTempoRestante){
            menorTempoRestante = tempoRestanteAtual;
            proximaTarefa = tarefaAux;
        }

        //se a tarefa a ser mandada para o processador é crítica (dispacher)
        //então, ela não poderá ser preemptada no tratador
        if(proximaTarefa == taskDisp){
            proximaTarefa->tarefaCritica = 1;
        }
    }

    //a tarefa vai receber o processador
    proximaTarefa->ativacoes++;

    return proximaTarefa;
}

/*
Incrementa o tempo de espera para todas as tarefas da fila de prontas
ou seja, todas as tarefas que não estão executando na CPU
*/
void incrementaTempoDeEspera(){

    task_t *tarefaAux = readyQueue;

    //verifica se a fila nao esta vazia
    if(readyQueue != NULL){
    
        tarefaAux = tarefaAux->next;

        while(tarefaAux != readyQueue){

            tarefaAux->tempoDeEspera++;

            tarefaAux = tarefaAux->next;
        }

        //para a primeira tarefa da fila
        tarefaAux->tempoDeEspera++;

    }
}


/*
a cada disparo do temporizador, o tratador decrementa o quantum e
incrementa o running_time da tarefa corrente. Além disso, incrementa
o tempo de espera das demais tarefas 
*/
void tratador(){

    //contador global do sistema é incrementado 
    systemTime++;
    
    //a rotina de tratamento de ticks de relógio deve decrementar o contador de
    //quantum da tarefa corrente, se for uma tarefa de usuário
    if(taskExec->tarefaCritica == 0){
        //se quantum esgotou, então a tarefa corrente é preemptada
        if(taskExec->quantum == 0){
            //antes da preempção, setamos o quantum da tarefa pro valor definido
            taskExec->quantum = QUANTUM;
            task_yield();
        }
        else{
            taskExec->quantum--;
        }
    }

    

    incrementaTempoDeEspera();
    //incrementando o tempo de execução no processador
    taskExec->running_time++;

}


void after_task_create (task_t *task ) {
    // put your customization here
    task->quantum = QUANTUM;
    task->running_time = 0;
    task->tempoEstimado = 99999;
    task->tempoRestante = 0;
    task->tempoDeEspera = 0;
    task->tarefaCritica = 0;
    task->ativacoes = 0;
    task->inicio = systime();

#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}


void after_ppos_init () {
    
    systemTime = 0;

    //taskMain só será mandada ao processador, caso não exista mais tarefas de usuario na fila
    task_set_eet(taskMain, 99999);

    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = tratador;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ");
        exit (1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }


#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}


void before_task_exit () {
    
    if(taskExec->running_time == taskExec->tempoEstimado)
        printf("\nTarefa %d acabou sua execução", taskExec->id);
    
    printf("\nTask %d exit: execution time %d ms, processor time %d ms, %d activations \n", 
            taskExec->id, taskExec->running_time + taskExec->tempoDeEspera, taskExec->running_time, taskExec->ativacoes);

#ifdef DEBUG
    printf("\ntask_exit - BEFORE- [%d]", taskExec->id);
#endif
}


// ****************************************************************************


void before_ppos_init () {
    // put your customization here

#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void after_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - AFTER - [%d]", taskExec->id);
#endif
}

void before_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield () {
    // put your customization here 
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}

void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

