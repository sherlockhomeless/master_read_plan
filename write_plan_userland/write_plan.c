#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../pbs_entities.h"
#include "../prediction_failure_config.h"


void write_binary_to_file(struct PBS_Plan* plan, char* path);
struct PBS_Plan* parse_plan(char* plan_s, struct PBS_Plan* plan);

char* parse_meta(char* plan_s, char* cur_position, struct PBS_Plan* p);
void parse_tasks(char* plan_s, char* cur_position, struct PBS_Plan* p);

void parse_next_process(char** str, struct PBS_Process* process_information);
char* parse_next_task(struct PBS_Plan * , int , char* );
char parse_cur_symbol(char *str);
long parse_next_number(char** str_ptr);
long count_tasks(char* task_list);
long length_plan(struct PBS_Plan*);

// defining symbols for parsing
#define NUMBER 0b0
#define COMMA 0b1
#define SEMI 0b10
#define META_END 0b11
#define END 0b1111

#define DRY_RUN 0

int main(int argc, char const *argv[]) {
    struct PBS_Plan plan = {0};

    // if a path to a plan.log was given
    if (argc > 1){
        char ch;
        FILE* fp = fopen(argv[1], "r");
        int word_count = 0;
        char plan_str[MAX_LEN_PLAN_STR];
        while((ch = fgetc(fp)) != EOF){
            plan_str[word_count] = ch;
            word_count++;
        }
        plan_str[word_count] = '\0';
        printf("read %s\n", plan_str);
        parse_plan(plan_str, &plan);
    }


    char * char_dev_name = "/dev/pbs_plan_in";
    printf("Plan has size=%zd, num_processes=%ld, address=%p\n", sizeof(plan), plan.num_processes, &plan);
    if (!DRY_RUN)
        write_binary_to_file(&plan, char_dev_name);

    return 0;
}


void write_binary_to_file(struct PBS_Plan* plan, char* path){
    FILE *fp;
    fp = fopen(path, "wb");
    if (fp == NULL){
        printf("error opening file\n");
        return;
    }
    //https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.htm
    size_t written = fwrite(plan, sizeof(struct PBS_Plan), 1, fp);
    if (written == 1)
        printf("wrote plan to device\n length of all tasks: %ld\n", length_plan(plan));
}


struct PBS_Plan* parse_plan(char* plan_s, struct PBS_Plan* plan){
    char* cur_position = plan_s;
    // parse meta-section until we find ';;;'
    cur_position = parse_meta(plan_s, cur_position, plan);
    parse_tasks(plan_s, cur_position, plan);
    plan->state = ON_PLAN;
    plan->tasks_finished = 0;
    plan->cur_task = plan->tasks;
    plan->finished_tasks = plan->tasks;
    plan->cur_process = &plan->processes[plan->cur_task->process_id];
    plan->tick_counter = 0;
    return plan;
}


/*
 * @brief parses the meta information contained in the p
 * @param plan_s: String of the p to be parsed
 * @param cur_position: Pointer to the current position in the parsing process
 * @param p: Target data structure to insert data into
 */
char* parse_meta(char* plan_s, char* cur_position, struct PBS_Plan* p){
    /* FLAGS:
    0b0 => found nothing
    0b1 => found number of processes
    */
    char cur_symbol;
    char state = 0b0;
    char found_end = 0b0;
    int process_counter = 0;

    // indices: process_id, buffer
    long process_information [2];

    while(!found_end) {
        cur_symbol = parse_cur_symbol(cur_position);

        // parse number of processes
        if (cur_symbol == NUMBER && state == 0b0) {
            long number_processes = parse_next_number(&cur_position);
            p->num_processes = number_processes;
            state = 0b1;
            cur_position++;

            if (LOG_PBS)
                printf("number of processes found: %ld\n", p->num_processes);

        // found end of process_section in plan
        } else if (cur_symbol == SEMI && parse_cur_symbol(cur_position + 1) == SEMI) {
            cur_position += 2;
            found_end = 1;
            continue;
        } else {
            p->processes[process_counter].num_tasks_remaining = 0;
            p->processes[process_counter].lateness = 0;
            p->processes[process_counter].length_plan = 0;
            p->processes[process_counter].instructions_retired = 0;
            p->processes[process_counter].process_id = 0;
            p->processes[process_counter].buffer = 0;

            parse_next_process(&cur_position, &p->processes[process_counter]);

            if(LOG_PBS)
                printf("process %ld: buffer=%ld\n", p->processes[process_counter].process_id, p->processes[process_counter].buffer);
            process_counter++;
        }
    }

    return cur_position;
}

void parse_tasks(char* plan_s, char* cur_position, struct PBS_Plan* p) {
    int i;

    p->num_tasks = count_tasks(cur_position);

    for (i = 0; i < p->num_tasks; i++){
        cur_position = parse_next_task(p, i, cur_position);

        if(LOG_PBS)
            printf("task %ld: pid=%ld, length_plan=%ld @=%p %ld\n ", p->tasks[i].task_id, p->tasks[i].process_id, p->tasks[i].instructions_planned, &p->tasks[i], p->num_processes);
    }

    p->tasks[i].task_id = -2;
    p->tasks[i].process_id = -2;
}

/**
 * Reads a task from a string into the task list on index at the given plan
 * @param plan
 * @param index
 * @param cur_position: Pointing to the first character in the tasks process
 * @return
 */
char* parse_next_task(struct PBS_Plan* plan, int index, char* cur_position){

    struct PBS_Task cur_t;
    cur_t.lateness = 0;
    cur_t.instructions_retired_slot = 0;
    cur_t.instructions_retired_task = 0;
    cur_t.state = PLAN_TASK_WAITING;
    cur_t.slot_owner = SHARES_NO_SLOT;

    cur_t.process_id = parse_next_number(&cur_position);
    if(cur_t.process_id != -1)
        plan->processes[cur_t.process_id].num_tasks_remaining++;

    cur_position++;
    cur_t.task_id = parse_next_number(&cur_position);
    cur_position++;
    cur_t.instructions_planned = parse_next_number(&cur_position);
    if(cur_t.process_id != -1)
        plan->processes[cur_t.process_id].length_plan += cur_t.instructions_planned;
    cur_position++;
    cur_t.instructions_real = parse_next_number(&cur_position);
    cur_position++;
    plan->tasks[index] = cur_t;


    return cur_position;
}
/*
 * Counts the task in the plan-string by counting semicolons
 */
long count_tasks(char* task_list_s){
    long counter = 0;
    while(*task_list_s != '\0'){
        if(*task_list_s == ';'){
            counter++;
        }
        task_list_s++;
    }
    return counter;
}
char parse_cur_symbol(char* str){
    switch(*str){
        case ',':
            return COMMA;
        case ';':
            return SEMI;
        case '\n':
            return END;
        default:
            return NUMBER;
    }
}

// ! pointer at first char after last number
void parse_next_process(char** str, struct PBS_Process* process_information){
    process_information->process_id = parse_next_number(str);
      *str+=1;
    process_information->buffer = parse_next_number(str);
      *str+=1;
}

// ! pointer at last digit of number
long parse_next_number(char **str_ptr) {
    char *start = *str_ptr;
    // reads job_id
    while (parse_cur_symbol(*str_ptr) == NUMBER) {
        *str_ptr = *str_ptr + 1;
    }
    long res = strtol(start, str_ptr, 10);
    assert(**str_ptr == ';' || **str_ptr == ',');
    return res;
}


long length_plan(struct PBS_Plan* p){
    long total_length = 0;
    int i;
    for(i = 0; i < MAX_NUMBER_TASKS_IN_PLAN; i++){
        total_length += p->tasks[i].instructions_planned;
    }
    return total_length;
}
