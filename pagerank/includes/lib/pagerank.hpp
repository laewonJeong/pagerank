#include <iostream>
#include <map>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <thread>

#define df 0.85
#define buf_size1 104857

using namespace std;
class Pagerank{
    public:
       void create_graph_data(string path, int num_of_vertex);
       void initial_pagerank_value();
       static void calc_pagerank_value(int start, int end);
       static void thread_calc_pr(int index);
       static void combine_pr();
       static void update_pr();
       static void thread_update_pr(int index);
       void send_recv_pagerank_value(int start, int end);
       void change_pagerank_value();
       void run_pagerank(int iter);
       int max_pr();
       void init_connection(const char* ip, string server[], 
                            int number_of_server, int Port,
                            int number_of_vertex);
    private:
        vector<int> graph[4039];
        vector<long double> pr;
        vector<long double> new_pr;
        vector<long double> my_pr;
        int num_of_vertex;
        char send_buffer[6][buf_size1];
        char recv_buffer[6][buf_size1];
        int start1;
        int end1;
};