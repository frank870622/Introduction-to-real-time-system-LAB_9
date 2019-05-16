#include <iostream>
#include <algorithm>
#include <memory.h>
#include <vector>
#include <string>
#include <math.h>
using namespace std;

class Task //class of each task
{
  public:
    int ID;
    int release_time; //arrival time
    int execution_time;
    int deadline;
    int period = 0;
    int preemption;
    int task;
    int suspend = 0;
    int waiting_time = 0;
};

class Processor //class of each processor
{
  public:
    int id;
    int ability;
    Task *task;
};

class Constraint //class fo precedence constraint
{
  public:
    int parent;
    int child;
    int child_release_time;
};

bool sort_execution_time(Task, Task);            //sort vector's execute time from small to big
bool sort_realease_time(Task, Task);             //sort vector's release time(arrival time) from small to big
bool sort_watting_time(Task, Task);              //sort vector's waiting time from big to small
bool sort_deadline_time(Task, Task);             //sort vector's deadline time from small to big
bool sort_slack_time(Task, Task);                //sort vector's deadline time from small to big
bool sort_ID(Task, Task);                        //sort vector's ID from small to big
bool running_vector_empty(const vector<Task> *); //check all running queue is empty of not
int get_slack_time(Task);                        //get the slack time of a task

//function to find greatest common divisor
template <typename T>
T GCD(T a, T b)
{
    if (b)
        while ((a %= b) && (b %= a))
            ;
    return a + b;
}
//function to find least common multiple
template <typename T>
T LCM(T a, T b)
{
    return a * b / GCD(a, b);
}

int process_time = 0; //time of schedule

int main()
{
    float hit = 0;                     //task terminal before deadline
    float miss = 0;                    //task terminal after deadline
    float allWaitTime = 0;             //waiting time of all task
    float variation = 0;               //variation of waiting time
    float standard_deviation = 0;      //standart_derivation of waiting time
    int threshold = 10;                //any task's waiting time > threshold need get high priority
    int hyper_period = 1;              //hyper period of tasks
    int CPU_task_runtime = 0;          //task running time
    float CPU_utilization = 0;         //CPU_utilization
    int precedence_constraint_num = 0; //precedence constraint number

    int framesize = 0;  //frame size (lab9)
    int next_frame = 0; //the next frame timer
    int now_frame = 0;  //this frame timer

    freopen("input.txt", "r", stdin); //set input.txt as the stdin
    int processor_num, task_num;
    cin >> processor_num >> task_num; //input processor number & task number

    vector<Task> readyvector_low;          //vector to store ready task
    vector<Task> readyvector_high;         //vector to store ready task
    vector<Processor> provector;           //vector to store processor
    vector<Task> waitvector;               //vector to store waiting task
    vector<Task> runvector[processor_num]; //vector to store running task
    vector<Task> tervector;                //vector to store terminal task
    vector<Constraint> constraintvector;   //vector to store constraint

    for (int i = 0; i < processor_num; i++)
    {
        Processor processor;                      //initial each processor
        cin >> processor.id >> processor.ability; //input each processor's ID & ability
        provector.push_back(processor);           //push processor into vector
    }

    for (int i = 0; i < task_num; i++)
    {
        Task task;
        //initial each task
        cin >> task.ID; //input each task's parameter
        cin >> task.release_time;
        cin >> task.execution_time;
        cin >> task.deadline;
        cin >> task.period;
        cin >> task.preemption;
        cin >> task.task;

        //if it is periodic , it's deadline is its next release time
        if (task.period > 0)
            task.deadline = task.release_time + task.period;

        waitvector.push_back(task); //push task into vector
    }
    cout << "after task input" << endl;

    //input precedence constraing number
    cin >> precedence_constraint_num;

    //input the dependence
    for (int i = 0; i < precedence_constraint_num; i++)
    {
        Constraint new_constraint;
        cin >> new_constraint.parent;
        cin >> new_constraint.child;
        new_constraint.child_release_time = waitvector[new_constraint.child].release_time; //save the child's oringal release
        waitvector[new_constraint.child].release_time = 99999999;                          //befor the parent terminate, give chile a huge release time
        constraintvector.push_back(new_constraint);
    }

    //change constraing deadline, release
    for (int i = 0; i < precedence_constraint_num; i++)
    {
        waitvector[constraintvector[i].parent].deadline = min(waitvector[constraintvector[i].parent].deadline, waitvector[constraintvector[i].child].deadline);
        waitvector[constraintvector[i].child].release_time = waitvector[constraintvector[i].parent].release_time + waitvector[constraintvector[i].parent].execution_time;
    }

    //find hyper period
    for (int i = 0; i < waitvector.size(); i++)
    {
        if (waitvector[i].period == 0)
            continue;
        hyper_period = LCM(hyper_period, waitvector[i].period);
    }
    cout << "hyper period: " << hyper_period << endl;

    //count frame
    int frame_rule1 = 0, frame_rule3 = 0; //three rule to get frame size
    vector<int> frame_rule2;
    int frame_rule2_max_period = 0;
    for (int i = 0; i < waitvector.size(); ++i)
    {
        frame_rule1 = max(frame_rule1, waitvector[i].execution_time);
        frame_rule2_max_period = max(frame_rule2_max_period, waitvector[i].period);
    }
    for (int i = frame_rule1; i < frame_rule2_max_period; ++i)
    {
        if (frame_rule2_max_period % i == 0)
            frame_rule2.push_back(i);
    }
    for (int i = 0; i < frame_rule2.size(); ++i)
    {
        bool frame_flag = true;
        for (int j = 0; j < waitvector.size(); ++j)
        {
            if (waitvector[i].period > 0)
            {
                if (2 * frame_rule2[i] - GCD(waitvector[i].period, frame_rule2[i]) > waitvector[i].period)
                    frame_flag = false;
            }
        }
        if (frame_flag == true)
        {
            framesize = frame_rule2[i];
            break;
        }
    }
    cout << "framesize: " << framesize << endl;
    now_frame = 0;
    next_frame = now_frame + framesize;

    //sort readyvector_low by it's release time to get the first task
    sort(waitvector.begin(), waitvector.end(), sort_realease_time);

    //create string for every process
    string outputstring[processor_num];
    for (int i = 0; i < processor_num; i++) //initial the output string of each processor
    {
        outputstring[i] = outputstring[i] + "Processor: " + to_string(provector[i].id) + "\n";
    }

    cout << "start job schedule" << endl;
    //job schedule
    while (1)
    {
        //when have period time break at hyper period time // lab8 don't have period
        if (process_time == hyper_period)
            break;
        //if ready,wait,run queue is empty -> all task is terminate -> program end
        if (readyvector_low.size() <= 0 && readyvector_high.size() <= 0 && waitvector.size() <= 0 && running_vector_empty(runvector))
            break;

        //first sort the ready vector & waiting queue
        sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time);
        sort(waitvector.begin(), waitvector.end(), sort_realease_time);

        //change frame
        if (process_time % framesize == 0)
        {
            now_frame = next_frame;
            next_frame = now_frame + framesize;
        }

        //release time schedule
        if (waitvector.size() > 0)
        {
            //if release time < process time,is means the task is arrived ,so move the task from waitingqueue to readyqueue
            while (waitvector.begin()->release_time <= process_time && waitvector.size() > 0)
            {
                readyvector_low.push_back(waitvector[0]); //move task to ready

                //hyper period
                if (waitvector[0].period > 0)
                {
                    Task period_task = waitvector[0];                                       //create a new task for period release
                    period_task.release_time += period_task.period;                         //change the release time of the new task
                    period_task.waiting_time = 0;                                           //change the waiting time of the new task
                    period_task.deadline = period_task.release_time + period_task.deadline; //change the deadline of the new task
                    waitvector.push_back(period_task);                                      //push the period release task into waiting queue
                }

                waitvector.erase(waitvector.begin());                                     //remove task from waiting
                sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time); //sort the ready queue by deadline
                sort(waitvector.begin(), waitvector.end(), sort_realease_time);           //sort the waiting queue by release time
            }
        }

        //working for all processor
        for (int i = 0; i < processor_num; i++)
        {
            //if the run queue of a processor is empty, move a ready task to it
            if (runvector[i].size() <= 0)
            {
                //searth task from high queue first, low queue second
                if (readyvector_high.size() > 0 && 0) //this lab don't have high preemption
                {
                    runvector[i].push_back(readyvector_high[0]);                                                             //move task to run queue
                    readyvector_high.erase(readyvector_high.begin());                                                        //remove task from ready
                    outputstring[i] += "\t" + to_string(process_time) + " Task" + to_string(runvector[i].begin()->ID) + " "; //else, just move to run queue
                }
                else if (readyvector_low.size() > 0 && 0) //close at lab9
                {
                    //this scheme let ready queue sort first by deadline, second by id
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_ID);            //sort id to let lower id to be executed first
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time); //sort ready queue with deadline

                    runvector[i].push_back(readyvector_low[0]);     //move task to run queue
                    readyvector_low.erase(readyvector_low.begin()); //remove task from ready

                    if (runvector[i][0].suspend == 1 && 0 /*lab4 don't have preemption*/) //if suspend is i, it means this task has been remove from run queue, this situation need context switch
                    {                                                                     //during context switch, the task whick need to be move to run queue 's waiting time need to be added
                        runvector[i][0].waiting_time++;
                        outputstring[i] += "\t" + to_string(process_time) + " Context Switch " + to_string(process_time + 1) + "\n"; //context switch
                        outputstring[i] += "\t" + to_string(process_time + 1) + " Task" + to_string(runvector[i].begin()->ID) + " "; //the task move to run queue
                        continue;                                                                                                    //because context switch ,at this moment, this processor can't do anyting else
                    }
                    else
                    {
                        outputstring[i] += "\t" + to_string(process_time) + " Task" + to_string(runvector[i].begin()->ID) + " "; //else, just move to run queue
                    }
                }
                else if (readyvector_low.size() > 0)
                { //lab9 mode
                    //this scheme let ready queue sort first by deadline, second by id
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_ID);            //sort id to let lower id to be executed first
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time); //sort ready queue with deadline

                    for (int j = 0; j < readyvector_low.size(); ++j) //judge with period task
                    {
                        if (process_time + readyvector_low[j].execution_time < next_frame && readyvector_low[j].period > 0) //check is task not lay two frame
                        {
                            runvector[i].push_back(readyvector_low[j]);                                                              //move task to run queue
                            readyvector_low.erase(readyvector_low.begin() + j);                                                      //remove task from ready
                            outputstring[i] += "\t" + to_string(process_time) + " Task" + to_string(runvector[i].begin()->ID) + " "; //else, just move to run queue
                            break;
                        }
                    }

                    if (runvector[i].size() <= 0)
                    {                                                    //judge with non aperiodic task
                        for (int j = 0; j < readyvector_low.size(); ++j) //judge with period task
                        {
                            if (process_time + readyvector_low[j].execution_time < next_frame && readyvector_low[j].period == 0) //check is task not lay two frame
                            {
                                runvector[i].push_back(readyvector_low[j]);                                                              //move task to run queue
                                readyvector_low.erase(readyvector_low.begin() + j);                                                      //remove task from ready
                                outputstring[i] += "\t" + to_string(process_time) + " Task" + to_string(runvector[i].begin()->ID) + " "; //else, just move to run queue
                                break;
                            }
                        }
                    }
                }
            }

            //context switch
            if (runvector[i].size() > 0 && readyvector_low.size() > 0 && 0) // this scenario don't have preemption
            {
                //context switch with shortest execution time
                if (runvector[i].begin()->execution_time > readyvector_low.begin()->execution_time + 1 && 0) //lab6 use deadline
                {
                    runvector[i][0].suspend = 1;                                                                                                             //set context switch flag
                    readyvector_low.push_back(runvector[i][0]);                                                                                              //move running task to ready queue
                    runvector[i].erase(runvector[i].begin());                                                                                                //remove running task
                    outputstring[i] += to_string(process_time) + "\n\t" + to_string(process_time) + " Context Switch " + to_string(process_time + 1) + "\n"; //output context switch
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time);                                                                //sort ready queue by time

                    runvector[i].push_back(readyvector_low[0]);                                                                  //move another to run queue
                    readyvector_low.erase(readyvector_low.begin());                                                              //remove from ready queue
                    runvector[i][0].waiting_time++;                                                                              //even if the task has been move to running queue, the task has not been execute yet, so the moved task's waiting time need to be added
                    outputstring[i] += "\t" + to_string(process_time + 1) + " Task" + to_string(runvector[i].begin()->ID) + " "; //task
                    continue;                                                                                                    //because context switch ,at this moment, this processor can't do anyting else
                }
                //context switch with shortest slack time         //lab7 without context switch time
                if (get_slack_time(runvector[i][0]) > get_slack_time(readyvector_low[0]))
                {
                    runvector[i][0].suspend = 1;                //set context switch flag
                    readyvector_low.push_back(runvector[i][0]); //move running task to ready queue
                    runvector[i].erase(runvector[i].begin());   //remove running task
                    //outputstring[i] += to_string(process_time) + "\n\t" + to_string(process_time) + " Context Switch " + to_string(process_time + 1) + "\n"; //output context switch
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_slack_time); //sort ready queue by time

                    runvector[i].push_back(readyvector_low[0]);     //move another to run queue
                    readyvector_low.erase(readyvector_low.begin()); //remove from ready queue
                    //runvector[i][0].waiting_time++;                                                                              //even if the task has been move to running queue, the task has not been execute yet, so the moved task's waiting time need to be added
                    outputstring[i] += to_string(process_time /* + 1*/) + "\n\t" + to_string(process_time /* + 1*/) + " Task" + to_string(runvector[i].begin()->ID) + " "; //task
                    //continue;                                                                                                    //because context switch ,at this moment, this processor can't do anyting else
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_slack_time); //sort ready queue by time
                }
                //in lab6 when two tasks have same deadline, lower id need to be execute first, lab7 don't have this condition
                else if (runvector[i].begin()->deadline == readyvector_low.begin()->deadline && 0)
                {
                    //this scheme let ready queue sort first by deadline, second by id
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_ID);            //sort id to let lower id to be executed first
                    sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time); //sort ready queue with deadline

                    if (runvector[i].begin()->ID > readyvector_low.begin()->ID)
                    {
                        runvector[i][0].suspend = 1;                //set context switch flag
                        readyvector_low.push_back(runvector[i][0]); //move running task to ready queue
                        runvector[i].erase(runvector[i].begin());   //remove running task
                        //outputstring[i] += to_string(process_time) + "\n\t" + to_string(process_time) + " Context Switch " + to_string(process_time + 1) + "\n"; //output context switch
                        sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time); //sort ready queue by time

                        runvector[i].push_back(readyvector_low[0]);     //move another to run queue
                        readyvector_low.erase(readyvector_low.begin()); //remove from ready queue
                        //runvector[i][0].waiting_time++;                                                                              //even if the task has been move to running queue, the task has not been execute yet, so the moved task's waiting time need to be added
                        outputstring[i] += to_string(process_time /* + 1*/) + "\n\t" + to_string(process_time /* + 1*/) + " Task" + to_string(runvector[i].begin()->ID) + " "; //task
                        //continue;                                                                                                    //because context switch ,at this moment, this processor can't do anyting else
                        sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time); //sort ready queue by time
                    }
                }
            }

            //task excuting
            if (runvector[i].size() > 0)
            {
                //cout << "excute\n";
                runvector[i][0].execution_time -= 1;     //execute all task in running queue
                ++CPU_task_runtime;                      //runtime +1 , cpu isn't sleeping
                if (runvector[i][0].execution_time == 0) //task terminal
                {
                    //if the terminal task is a parent , set it's child's release time to the origin
                    for (int j = 0; j < constraintvector.size(); j++)
                        if (runvector[j][0].ID == constraintvector[j].parent)
                            for (int k = 0; k < waitvector.size(); k++)
                                if (waitvector[k].ID == constraintvector[j].child)
                                    waitvector[k].release_time = constraintvector[j].child_release_time;

                    if (runvector[i][0].deadline < process_time + 1) //check if this task terminal before deadline of not
                        ++miss;
                    else
                        ++hit;

                    //cout << "a task terminal\n";
                    tervector.push_back(runvector[i][0]);                  //move task to terminal queue
                    runvector[i].erase(runvector[i].begin());              //remove task from running queue
                    outputstring[i] += to_string(process_time + 1) + "\n"; //output task
                }
            }
        }
        //add waiting time
        for (int i = 0; i < readyvector_low.size(); i++)
        {
            ++readyvector_low[i].waiting_time;
        }
        for (int i = 0; i < readyvector_high.size(); i++)
        {
            ++readyvector_high[i].waiting_time;
        }

        //threashold schedule
        /*
        sort(readyvector_low.begin(), readyvector_low.end(), sort_watting_time);    //sort low ready queue by waiting time
        while (readyvector_low[0].waiting_time >= 10 && readyvector_low.size() > 0) //find task which waiting time >= 10 and move it to high priority
        {
            readyvector_high.push_back(readyvector_low[0]);
            readyvector_low.erase(readyvector_low.begin());
        }
        */
        sort(readyvector_low.begin(), readyvector_low.end(), sort_deadline_time); //sort low ready queue by waiting time
        //add process time
        ++process_time;
    }

    for (int i = 0; i < processor_num; i++) //print the output string
    {
        cout << outputstring[i];
    }

    //add all waiting time of all task in ready queue
    for (int i = 0; i < tervector.size(); i++)
    {
        allWaitTime += tervector[i].waiting_time;
    }
    CPU_utilization = (float)CPU_task_runtime / (float)hyper_period; //get cpu tuilization

    //calculate variation time
    for (int i = 0; i < tervector.size(); i++)
    {
        variation += (tervector[i].waiting_time - allWaitTime / (float)tervector.size()) * (tervector[i].waiting_time - allWaitTime / (float)tervector.size());
    }
    variation /= tervector.size();

    //calculate standard deviation
    standard_deviation = sqrt(variation);

    cout << "Average Waiting Time: " << allWaitTime / (float)tervector.size() << endl;
    cout << "variation of waiting time: " << variation << endl;
    cout << "standard deviation: " << standard_deviation << endl;
    cout << "Hit Rate: " << hit / (hit + miss) << endl;
    cout << "Miss Rate: " << miss / (hit + miss) << endl;
    cout << "CPU utilization: " << CPU_utilization << endl;

    return 0;
}

bool sort_execution_time(Task task1, Task task2)
{
    return task1.execution_time < task2.execution_time;
}
bool sort_realease_time(Task task1, Task task2)
{
    return task1.release_time < task2.release_time;
}
bool sort_watting_time(Task task1, Task task2)
{
    return task1.waiting_time > task2.waiting_time;
}
bool sort_deadline_time(Task task1, Task task2)
{
    return task1.deadline < task2.deadline;
}
bool sort_slack_time(Task task1, Task task2)
{
    return (task1.deadline - process_time - task1.execution_time) < (task2.deadline - process_time - task2.execution_time);
}
bool sort_ID(Task task1, Task task2)
{
    return task1.ID < task2.ID;
}
bool running_vector_empty(const vector<Task> *runvector)
{
    bool ans = true;
    for (int i = 0; i < runvector->size(); i++)
    {
        if (runvector[i].size() > 0)
            ans = false;
    }
    return ans;
}
int get_slack_time(Task task1)
{
    return task1.deadline - process_time - task1.execution_time;
}