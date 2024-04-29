//------------------------------------------------------------------------------
// Definição e operações em uma fila genérica.
// by Eduardo e Jader
//------------------------------------------------------------------------------

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// - **queue é um ponteiro para um outro ponteiro que aponta para a primeira posição da fila

void queue_append (queue_t **queue, queue_t *elem){

    if(queue == NULL){
        fprintf(stderr, "Fila não existe\n");
        return -1;
    }

    if(elem == NULL){
        fprintf(stderr, "Elemento não existe\n");
        return -2;
    }
    else{//verificar se elem já está na fila

        //ponteiro auxiliar recebe o primeiro elemento da fila
        queue_t *aux = *queue;

        //verificando se o primeiro elemento da fila é igual a elem
        if(aux == elem){
            printf(stderr, "Elemento já está na fila");
            return -3;
        }

        //verificando as demais posições
        while(aux->next != (*queue)){
            if(aux == elem){
                printf(stderr, "Elemento já está na fila");
                return -3;
            }
        }

        //se o elem não foi encontrado, então ele está em outra fila
        printf(stderr, "Elemento está em outra fila");

    }

    //inserindo no final da fila

    queue_t * aux = (*queue)->prev;
    aux->next = elem;
    elem->prev = aux;
    (*queue)->prev = elem;
    elem->next = *queue;


    return 0;

}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem) ;

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {

    int i = 1;
    //fila vazia 
    if(queue == NULL)
        return 0;
    else{
        //ponteiro auxiliar
        queue_t *aux = queue;
        //percorrendo a fila circular
        while(aux->next != queue){
            i++;
            aux = queue->next;
        }
    }
    return i;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){

    if(queue == NULL)
        printf("Fila Vazia");
    else{
        //ponteiro auxiliar
        queue_t *aux = queue;

        //imprimindo o primeiro da fila (em caso de fila com 1 elemento)
        print_elem(aux);

        //percorrendo a fila circular
        while(aux->next != queue){
            print_elem(aux);
            aux = queue->next;
        }
    }
}
